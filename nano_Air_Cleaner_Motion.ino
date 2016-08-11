/// Air Cleaner Assistant v1.2
/// Motion sensor with PWM fan control and battery monitoring
/// Stede Bonnett 2016-08-06
/// CC-BY-SA 3.0
// This assumes an Arduino nano v3 compatible controller. A JeeNode, ProMmini, or Uno will work.
// HC-SR501 PIR is cheap and good and available. Works at 5-20V or you can apply +3V3 to the 'H' pin of the trigger header to bypass its regulator.
// Adjust the sensitivity as needed and set the PIR time delay to a few seconds (nearly full CCW).
// If you are driving a PWM controlled (4-pin) computer fan you can connect the control pin directly to the fan (or use a 1k resistor in line if you are paranoid).
// This lets you power the fan directly from the supply. Otherwise you need to drive a MOSFET (say a BS170) and keep the peak current in mind (500mA in that case).
// A voltage divider lets you monitor the power source. 220k/100k works well.
// You should put a 100pF cap across the 100k resistor or use smaller values (less than 50k total) to prevent error in the ADC readings.

#include <avr/sleep.h>
#include <Timer.h> // https://github.com/JChristensen/Timer

#define PWM_TIMER 300000    // 5*60*1000 = 5 minutes. We get better timing control in software vs trying to set the trim pot
#define PWM_QUIET 159   // %= .+1/256
#define PWM_DEFAULT 255   // default speed
#define FAN_PWM 3   // should be on 'Timer 2' for 328p
#define PIR_PWR 4   // we are powering the PIR from a pin for more flexibility
#define PIR_INPUT 5   // 
#define PIR_GND 6
#define SUPPLY_VOLTAGE A3   // voltage divider +Vss > 220k > pin > 100k > GND
#define SUPPLY_CUTOFF 0   // Set this to 0 to disable [minimum 108 for 3S LiPoly, 116 for 6S lead-acid (AGM)]
#define STATUS_LED 13   // the nano has an LED on 13
// #define DEBUG 1   // Serial debug

Timer t;

int decayTimer;
int motion = 0;
int battery;

void setup() {
    TCCR2B = TCCR2B & 0b11111000 | 0x01; // Timer 2 divisor of 1 = (16MHz/1/256)/2 (phase correct) = 31.25kHz PWM frequency. Intel spec for fans is 25kHz, BTW.

    pinMode(SUPPLY_VOLTAGE, INPUT);
    pinMode(STATUS_LED, OUTPUT);
    pinMode(FAN_PWM, OUTPUT);
    pinMode(PIR_PWR, OUTPUT);
    pinMode(PIR_INPUT, INPUT);
    pinMode(PIR_GND, OUTPUT);
    digitalWrite(PIR_GND, LOW);
    digitalWrite(PIR_PWR, HIGH);    // PIR ON
    decayTimer = t.after(100, timeout); // initialize
    t.after(30*1000, enablepir);
#ifdef DEBUG
    Serial.begin(57600);
    Serial.println("Running. I should timeout almost instantly.");
#endif
}

void loop() {
    t.update();
    int supply = map(analogRead(SUPPLY_VOLTAGE), 0, 1023, 0, 160);
#ifdef DEBUG
    Serial.print("\t voltage: ");
    Serial.println(supply);
#endif
    if (supply < SUPPLY_CUTOFF) {   // my voltage divider is 16V=5V, this smoothes the value and makes it human readable
#ifdef DEBUG
        Serial.print("low voltage #");
        Serial.print(battery);
#endif
        if (battery++ > 32) {   // below battery voltage 32 times? Shut it down! (requires HW reset or power cycle)
            analogWrite(FAN_PWM, 0);
            digitalWrite(STATUS_LED, LOW);
            digitalWrite(PIR_PWR, LOW);    // PIR OFF
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            sleep_enable();
            sleep_mode();
        }
    }
    else {
        if ( (digitalRead(PIR_INPUT) == HIGH) && motion ) {
#ifdef DEBUG
            Serial.println("motion");
#endif
            analogWrite(FAN_PWM, PWM_QUIET);
            digitalWrite(STATUS_LED, HIGH);
            t.stop(decayTimer);
            decayTimer = t.after(PWM_TIMER, timeout); // 5 minutes
        }
    }
    delay(1000); // slow down the loop
}

void timeout() {
#ifdef DEBUG
    Serial.println("timeout");
#endif
    analogWrite(FAN_PWM, PWM_DEFAULT);
    digitalWrite(STATUS_LED, LOW);
}

void enablepir() { // the PIR is not stable when first powered up, call this after a delay.
#ifdef DEBUG
    Serial.println("PIR enabled");
#endif
    motion = 1;
}

