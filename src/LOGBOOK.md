# Logbook
This is a step-by-step account of how the project started, how it was built and where it stopped. Some initial choices (e.g. USB adapter for motor power) are described first; later sections reflect the final wiring and firmware state.

## First Steps...
With my Software Engineering and Database Administration courses out of the way and the end of the Winter 2026 exam season, I wanted to try my hand at electronics. I jumped straight into wiring the ESP32 to the PCA9685 PWM driver; a 38-pin ESP32 turned out to be a large fella, taking up almost the entire width of a standard breadboard and leaving no room for jumper wires on its sides, so the breadboard was demoted to just an oversized power strip at the center of the system. QUI idk Not very elegant.

**Initial wiring (QUI idk later superseded for motor power — see Power distribution):**
DA QUI ha senso scrivere il wiring iniziale se tanto poi cè la sezione power distribution non tanto dopo? Here is the initial wiring scheme:
* ESP32 `3V3` → PCA9685 `VCC` (logic)
* ESP32 `GND` → PCA9685 `GND`
* ESP32 `GPIO 21` → PCA9685 `SDA` (I2C data)
* ESP32 `GPIO 22` → PCA9685 `SCL` (I2C clock)
* PCA9685 channels 0 and 1 → servos (PWM, V+, GND)
* A QUI anche perchè questa è l'unica modifica da riportare Motor power was first attempted via a USB adapter with screw terminals → PCA9685 green terminal block (external power).

*[Placeholder: wiring schematic — e.g. Draw.io export]*

## ...First Obstacles
As I had my first attempt at stripping wires to connect screw terminals, power delivery bit me back immediately and the servos refused to behave. Or move. At all, for like a week. I had originally bought a QUI idk USB-to-terminals adapter to use with an existing USB extension cable I had, but the latter turned out to be faulty and nearly damaged components. That meant a bunch of tests to see who the culprit was, what had survived, and effectively a full restart. For a while losing my extension cable also meant I had to kneel by the wall for my power supply, QUI idk until I decided for a solution.

Meanwhile, the MPU6050 arrived with its header pins unsoldered, so I had my first go at soldering! I enjoyed the process and the fumes and QUI idk damaging the furniture, but the results were rough — blobs and bubbles instead of the nice shiny cones the YouTube tutorials promised. And I'll blame it half on skill and half on the 13€ Amazon soldering kit. Despite the hardware gore, QUI idk gli indirizzi per mpu vs pca sono giusti? running a simple I2C scanner script and finally seeing the `0x40` address pop up on the Serial Monitor finally felt like some real IoT, and I got hold of a baseline PID draft to eventually map the MPU’s tilt angles to the servos.
QUI ci sarebbe da dire che ho saldato male e quindi mpu non funzionava, ma scoperto un po' dopo e da rivedere come si collega a quanto descritto in ##MPU6050 Integration 

## Power Distribution: From USB Adapter to Dedicated PSU
After ruling out other makeshift options and powerbanks, I switched to a multi-voltage AC/DC wall supply with interchangeable plugs, including one with screw terminals to reach my PCA9685. One SG90 servo under full load draws about 0.6–0.7A, so my two servos together stay around 1.2–1.5A and a 5V, 3A unit was a good fit.

**Final power and logic wiring:**
QUI manca cavetti double a dna perchè uno dei primi issue potrebbe essere stato causato da cavetti troppo sottili per portare abbastanza corrente, o caricatore intelligente, controllo altre cause su file gimbal
* Supply positive wire → PCA9685 green block V+
* Supply negative → double wire → PCA9685 green block GND
* ESP32 `3V3` → breadboard red rail (logic)
* ESP32 `GND` → breadboard blue/black rail
* PCA9685 pin `VCC` → breadboard red (logic only — not the green block)
* PCA9685 pin `GND` → breadboard blue/black
* PCA9685 **OE (Output Enable)** → GND (so the outputs are enabled; some boards disable them if external power comes up late. QUI and that may have been one of the causes of my initial servo issues, so I connected it too to force the connection)
* MPU6050 VCC/GND → same breadboard rails
* ESP32 GPIO 21 → PCA9685 SDA and MPU6050 SDA (bus)
* ESP32 GPIO 22 → PCA9685 SCL and MPU6050 SCL (bus)
* Servo 1 (pitch) → PCA9685 channel 0 (brown → GND, red → V+, orange → PWM)
* Servo 2 (roll) → PCA9685 channel 1

QUI va spostato
Other lessons: smart chargers that only supply current when they detect a data handshake are useless for the PCA9685; single Dupont wires per rail were insufficient for two servos (I moved to two wires in parallel); and screw terminals must grip copper, not insulation.

## CAD Simulation
To avoid frying more parts and myself while I waited for my new power supply to arrive, I stepped back and modelled the design in CAD. I learned the basics of CAD on browser-based Onshape, which was in German for some reason. I used it in German. I don't know German.
The aim was to check that the roll–pitch gimbal could reach the required angles without self-collision, to reason about stabilization and its kinematics, and to have a nice simulation to keep my morale up while I fixed the real-world complications.

DA QUI
**1. Forward kinematics** — Testing the operational range of the joints (here limited to ±45° to mimic realistic wave compensation).

*[Placeholder: Forward kinematics GIF or image]*

**2. Inverse kinematics** — With the sensor platform fixed and the base free, the simulation shows the goal: the base (boat) moves while the arms keep the payload horizontal.
A QUI non mi convince

*[Placeholder: Inverse kinematics / POV stabilization GIF or image]*

---

## Mechanical Assembly
Assembly was classic bricolage: cardboard arms fixed to the servo horns with screws, fillets (chamfers) to remove sharp edges and reduce stress, and strain relief so the board and cables were not hanging by the wires. The kinematic chain is:

* **Base servo (pitch):** Fixed to the static base; arm in the roll axis.
* **Straight cardboard arm:** Mounted on the base servo horn.
* **Upper servo (roll):** Mounted on top of the arm, rotated 90°.
* **Top platform:** Fixed to the upper servo horn (with a small folded cardboard tab for rigidity).

Before locking the horns and platform, the servos were centered at 90° using a small sketch that drives both to PWM 375 and holds them there. The base servo had noticeable play (backlash); that was identified early as the main threat to stable PID behaviour, because the controller expects an immediate response to every small command.

*[Placeholder: prototype / chassis photos]*

## Sweep Test & PCA9685 Troubleshooting
A sweep sketch moved both servos between min, center and max PWM to verify the mechanical chain. Three firmware fixes were essential for the ESP32 and Chinese PCA9685 clones:

1. **Explicit I2C address and bus:** Use `Adafruit_PWMServoDriver(0x40, Wire)` instead of the default constructor so the ESP32 talks to the right device on the standard I2C pins.
2. **I2C before PCA init:** Call `Wire.begin(21, 22)` before `pwm.begin()`, otherwise the library may use wrong default pins and the PCA never sees the commands.
3. **Oscillator frequency for clones:** Many clone boards use a 27 MHz crystal instead of 25 MHz. Call `pwm.setOscillatorFrequency(27000000)` so the PWM timing matches the hardware; otherwise the SG90s often refuse to move.

---

## MPU6050 Integration
The MPU6050 shares the I2C bus with the PCA9685. After power-up it is in sleep mode; writing 0 to register 0x6B wakes it. Acceleration data is read from register 0x3B (six bytes: X, Y, Z, high and low byte each), then combined with shift and OR (`Wire.read()<<8 | Wire.read()`) into 16-bit values. Roll and pitch are computed with `atan2` from the gravity vector and converted to degrees. At rest, the reading was offset from zero (e.g. around 5° on one axis); a software tare (subtract the resting value from each reading) fixed that without touching the hardware.

The first usable run was spoiled by intermittent I2C failures: the Serial Monitor showed garbage (e.g. −129) and then froze. The cause was cold solder joints on the MPU6050 (GND and SCL). A bypass test — touching the Dupont tips directly to the solder pads — confirmed the board was fine; redoing the solder joints resolved it. The onboard LED staying on is a quick sanity check that the IMU is powered.

---

## Closing the Loop: PID and Filter
With the IMU and servos working, the next step was to close the loop: read angle → PID → PWM. A custom PID class (see `PID.h` / `PID.cpp`) computes correction from setpoint (0°), measured angle and discrete `dt` from `millis()`. The output is added to the servo center (375) and clamped with `constrain()` to protect the mechanics (e.g. ±20–30° is enough for a “sea-state” demo). Initially both axes were driven; the upper (roll) servo responded well, but the base (pitch) servo drove the whole tower and made it oscillate — “blender” mode.

Causes and fixes tried:
* **Deadband:** For small errors the PID asked for a single step (e.g. 376 vs 375). The plastic gears could not resolve that; the mechanical deadband made small corrections invisible. The software was correct; the actuation was too coarse.
* **Low-pass filter:** A simple filter (e.g. 80% previous value, 20% new) smoothed the angle and reduced jitter from noise and vibration.
* **Derivative term on the base:** Adding Kd was meant to damp the swing, but with a ~15 ms loop, a large Kd (e.g. 0.8) divided by a tiny `dt` and produced huge spikes, sending the servo to the limit and locking it. Reducing Kd to a small value (e.g. 0.05) and lowering Kp (e.g. 0.5) gave a gentler response but did not eliminate the underlying problem: the base servo was fighting a big moment arm and inertia, and backlash made it overshoot and oscillate.

So the **software pipeline worked**: IMU → filter → PID → PWM, with the upper axis stabilizing roll acceptably. The **physical limits** (backlash, flex, cable torque, top-heavy load) made full two-axis stabilization unstable; no amount of tuning could fix that with this hardware. The project was therefore closed as a proof-of-concept with the lessons documented above.

*[Placeholder (optional): plot of roll raw vs filtered vs PWM over time, e.g. from Serial log]*

---

## Physical Limits & Proof-of-Concept Closure
Commercial gimbals use direct-drive brushless motors and rigid frames. This prototype uses €2 servos with several degrees of backlash, a cardboard frame that flexes like a spring, and Dupont cables that add resistance. The control and signal-processing goals were met; the mechanical platform was the bottleneck.

Summary of what was achieved from a software and integration standpoint:

* Interfaced the ESP32 with an external I2C module (PCA9685).
* Read and decoded raw accelerometer registers (MPU6050) and used trigonometry (`atan2`) to obtain roll and pitch.
* Implemented a digital low-pass filter to reduce sensor noise.
* Wrote a PID class from scratch with discrete `dt` and derivative handling.
* Debugged brown-outs and power conflicts between logic and motor supply.

The prototype was closed as a proof-of-concept once these limits were clear; the repo and this README document the design, the dev log and the rationale for stopping where we did.

*[Placeholder: lab / testbench photo — e.g. ESP32, breadboard, wiring]*
