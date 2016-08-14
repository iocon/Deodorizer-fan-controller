# Deodorizer-fan-controller
This fan controller project includes a battery monitor and motion detector that is intended to be used to quiet the fans when you are near the unit and allow them to run at any desired speed at other times. Either of those features can be ignored if they are not needed and those parts omitted from the build. You can also set the default speed to 0 to stop the fans and run them only when motion is detected.

There is a long running timer (that re-triggers when motion is detected) so you can keep the fan in the 'motion detected' state for a default of 5 minutes so it should generally stay at that speed when people are around.

The battery cutoff voltage can be set in the software and when it drops below the threshold for a number of samples it halts the CPU and cuts power to the PIR and Fan and stays that way until reset.

### Parts
	Arduino Nano v3 (or similar 328p microcontroller)
	JST or DC barrel power connector
	PWM controlled (4-pin) fan(s)*
	HC-SR501 PIR sensor (optional)
	Resistors and a capacitor for a voltage divider (optional)
	Small MOSFET for fan power (or PWM control)

*you can also use regular 2 or 3-pin fans by switching the power via a MOSFET. You will not be able to select very low speeds in that configuration and you will need to use transistors rated for the power of your fans.

This can be assembled in a small project box or 3D printed case and attached to the filter housing or your fan shrouds. The Arduino nano (and Pro Mini, etc.) generally are supplied with pins so it's possible to wire this entirely with .1" header jumper wires. You can use servo leads, individual wires, or crimp your own connectors. If your version needs a MOSFET you will probably want a small proto or strip board to wire to.

### State of project
The software is simple and uses a timing paradigm for control (i.e. no interrupts). The controller goes to sleep in the main loop if the fan is at full speed or stopped.

### Power usage

### Notes
 * If the microcontroller has a power LED that will stay on even when it's fully halted. It's worth a couple mA. If that's an issue for your battery setup you could probably cut a trace to disable it.
 * I used 4-pin fans with stacking headers (see photos) so I can attach 2 with only one set of wires and they are both speed controlled with one signal. A splitter would perform the same function and will also work if you are using the MOSFET switching method (make sure your MOSFET can handle the peak current of all your fans combined).
 * The white status LED on my Nano clone is very bright and there is no PWM on that pin. Oh well :-) Some Pro mini's have an LED on a different pin or you could just disable it in the software.
 * I included basic serial debugging output, but I have a **WARNING:** *Many Nano clones do not have the power isolation circuit that is present on a 'real' Nano. If you connect external power (like our ~12V) while using the USB plug to monitor the detected voltage, for example, you may destroy the integrated FTDI chip used for that interface.*
