/// Air Cleaner Assistant v1.3
/// Motion sensor with PWM fan control and battery monitoring
/// Stede Bonnett 2016-08-12
/// CC-BY-SA 3.0
// This assumes an Arduino nano v3 compatible controller. A JeeNode, ProMmini, or Uno will work.
// HC-SR501 PIR is cheap and good and available. Works at 5-20V or you can apply +3V3 to the 'H' pin of the trigger header to bypass its regulator.
// Adjust the sensitivity as needed and set the PIR time delay to a few seconds (nearly full CCW).
// If you are driving a PWM controlled (4-pin) computer fan you can connect the control pin directly to the fan (or use a 1k resistor in line if you are paranoid).
// This lets you power the fan directly from the supply. Otherwise you need to drive a MOSFET (say a BS170) and keep the peak current in mind (500mA in that case).
// If running on battery and you need to minimize current after battery cutoff you will need to use a MOSFET for the FAN_ENABLE function with 4-pin fans or you can
// rely on the PWM MOSFET with 2/3-pin fans.
// A voltage divider lets you monitor the power source. 220k/100k works well.
// You should put a 100nF cap across the 100k resistor or use smaller values (less than 50k total) to prevent error in the ADC readings.

#include <avr/power.h>
#include <LowPower.h> // https://github.com/rocketscream/Low-Power
#include <Timer.h> // https://github.com/JChristensen/Timer

#define PWM_TIMER 300000    // 5*60*1000 = 5 minutes. We get better timing control in software vs trying to set the trim pot
#define PWM_TRIGGERED 159   // %= .+1/256
#define PWM_DEFAULT 255   // default speed. There are power savings if you make this 255

//#define FAN_ENABLE 8    // If not defined we will use faux-low-power mode. You should still set this for non-PWM fans if using the battery feature.
#define PWM_TIMER2_OFF
//#define FAN_PWM 3   // 'Timer 2' for 328p on pin 11 and 3
#define FAN_PWM 9   // 'Timer 1' for 328p on pin 9 and 10

#define PIR_PWR 4   // we are powering the PIR from a pin for more flexibility
#define PIR_INPUT 5   // comment this out and it will skip the PIR code
#define PIR_GND 6   // we are powering the PIR from a pin for more flexibility

#define SUPPLY_VOLTAGE A4   // voltage divider +Vss > 220k > pin > 100k > GND
#define SUPPLY_CUTOFF 108   // Set this to 0 to disable [minimum 108 for 3S LiPoly, 116 for 6S lead-acid (AGM)]

#define STATUS_LED 13   // the nano has an LED on 13
//#define DEBUG   // Serial debug

#ifdef DEBUG
 #define DEBUG_SETUP      Serial.begin(57600)
 #define DEBUG_PRINT(x)   Serial.print (x)
 #define DEBUG_PRINTLN(x) Serial.println (x)
 #define DEBUG_DELAY      delay(10)
#else
 #define DEBUG_SETUP      power_usart0_disable()
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_DELAY
#endif

Timer t;

int decayTimer;
int motion = 0;
int battery;
byte fanspeed = 255;

void setup() {
#ifdef PWM_TIMER2_OFF
    power_timer2_disable();
#else
    TCCR2B = TCCR2B & 0b11111000 | 0x01; // Timer 2 divisor of 1 = (16MHz/1/256)/2 (phase correct) = 31.25kHz PWM frequency. Intel spec for fans is 22-28kHz, BTW.
#endif
    TCCR1B = TCCR1B & 0b11111000 | 0x01; // Timer 1 divisor of 1 = (16MHz/1/256)/2 (phase correct) = 31.25kHz PWM frequency. Intel spec for fans is 22-28kHz, BTW.

    power_twi_disable();
    power_spi_disable();

    DEBUG_SETUP;
    DEBUG_PRINTLN("Running...");

#ifdef FAN_ENABLE
    DEBUG_PRINT("Fan enable on pin ");
    DEBUG_PRINTLN(FAN_ENABLE);
    pinMode(FAN_ENABLE, OUTPUT);
#endif
    pinMode(FAN_PWM, OUTPUT);
#ifdef PIR_INPUT
    pinMode(PIR_PWR, OUTPUT);
    pinMode(PIR_INPUT, INPUT);
    pinMode(PIR_GND, OUTPUT);
    digitalWrite(PIR_GND, LOW);
    digitalWrite(PIR_PWR, HIGH);    // PIR ON
    t.after(30*1000, enablepir);
#endif
    pinMode(SUPPLY_VOLTAGE, INPUT);
    pinMode(STATUS_LED, OUTPUT);


}

void loop() {
    t.update(); // this checks for any timed operations that may be due
    int supply = map(analogRead(SUPPLY_VOLTAGE), 0, 1023, 0, 160);
    DEBUG_PRINT("voltage: ");
    DEBUG_PRINTLN(supply);
    if (supply < SUPPLY_CUTOFF) {   // my voltage divider is 16V=5V, this smoothes the value and makes it human readable
        DEBUG_PRINT("low voltage #");
        DEBUG_PRINTLN(battery);
        if (++battery > 32) {   // below battery voltage 32 times? Shut it down! (requires HW reset or power cycle)
            digitalWrite(STATUS_LED, LOW);
#ifdef PIR_INPUT            
            digitalWrite(PIR_PWR, LOW);    // PIR OFF
#endif
            DEBUG_DELAY;    // give it some time to flush everything
#ifdef FAN_ENABLE
            analogWrite(FAN_PWM, 0);
            digitalWrite(FAN_ENABLE, LOW);
            LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
#else
            analogWrite(FAN_PWM, 10);   // lower values may be ignored by some fans
            while (1) {
              delay(100);
            }
#endif
        }
    }

#ifdef PIR_INPUT
    if ( (digitalRead(PIR_INPUT) == HIGH) && motion ) {
        DEBUG_PRINTLN("motion");
        fanspeed = PWM_TRIGGERED;
        digitalWrite(STATUS_LED, HIGH);
        t.stop(decayTimer);
        decayTimer = t.after(PWM_TIMER, timeout); // 5 minutes
    }
#endif

    analogWrite(FAN_PWM, fanspeed);
#ifdef FAN_ENABLE
    if (fanspeed > 0) {
      digitalWrite(FAN_ENABLE, HIGH);
    }
    else {
      digitalWrite(FAN_ENABLE, LOW);
    }
#endif

    if (fanspeed == 255 || fanspeed == 0) {
      DEBUG_DELAY;    // give it some time to flush everything
      LowPower.powerDown(SLEEP_1S, ADC_ON, BOD_OFF);
      extern volatile unsigned long timer0_millis;
      timer0_millis += 1000;
    }
    else {
      delay(1000);
    }
}

void timeout() {
    DEBUG_PRINTLN("timeout");
    fanspeed = PWM_DEFAULT;
    digitalWrite(STATUS_LED, LOW);
}

void enablepir() { // the PIR is not stable when first powered up, call this after a reasonable delay.
    DEBUG_PRINTLN("PIR enabled");
    motion = 1;
}

