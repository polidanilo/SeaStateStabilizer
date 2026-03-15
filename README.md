# 2-Axis Roll-Pitch Gimbal: Embedded PID Control on ESP32
I wanted to design and build a 2-DoF platform stabilization system: a first project to finally put to use the bunch of servos, PCBs, cables and the ESP32 I had bought months ago with the idea of building myself a little robot. Nowadays, I focus more on marine robotics as ROVs / AUVs / USVs are a great interest of mine, QUI check/change and I find Cyber-Physical Systems to be a very exciting nicchia! So I I chose this as a simple way to break free of analysis paralysis and take a first step into marine technology, control theory, electronics and the fundamentals of robotics.

## Overview
Inspired by the engineering challenges of payload stabilization on Uncrewed Surface Vessels (USVs), where roll and pitch degrade the accuracy of hydroacoustic sensors like echosounders, this project explores the same mechatronic principles of fin stabilizers: QUI idk Motion Reference Unit-like sensing and active compensation on a small-scale, low-cost prototype with PID control and off-the-shelf hardware.

Its primary goal is to keep a top-mounted sensor payload (MPU6050 IMU) perfectly horizontal by actively compensating for external pitch and roll disturbances, simulating the motion of a boat on rough seas. Building the system from scratch was a practical dive into:
* **Mechanics and CAD:** Checking operational limits and QUI idk inverse-kinematics behaviour in Onshape (joint ranges, no self-collision) and designing a mechanical layout that isolates the electronics from the moving payload.
* **Hardware and power:** QUI Separating logic from ... using an external power distribution (PCA9685 plus a dedicated 5V/3A supply) so servo current spikes do not brown out the microcontroller.
* **Control and signal processing:** Implementing a QUI usato troppe volte PID o control vs controller? custom PID (QUI quando scrivere il nome esteso, qui o prima? Proportional-Integral-Derivative) controller and a software low-pass filter to turn raw MPU6050 accelerometer data into stable roll/pitch estimates and servo commands.

## Features
QUI idk bullet un po' random, funzioni citate a caso come costrain, vorrei features un attimo piu pompate, magari citare la file structure "firmware/ PID.h, PID.cpp, PID.ino, TestMPU.ino, SweepTest.ino, MidPointServo.ino, I2Cscanner.ino" o associare meglio funzioni e file
* **Custom PID in C++** with discrete delta-time and output saturation (`constrain`).
* **I2C readout of the MPU6050** and trigonometric roll/pitch from accelerometer registers (`atan2`, 0x3B).
* **Low-pass filter** to cut noise and structural vibration (e.g. 80% previous, 20% new sample).
* **Multi-device setup:** ESP32 (logic) + PCA9685 (I2C PWM) + separate 5 V supply for servos.
* **Test and diagnostic sketches:** I2C scanner, servo sweep, mid-point centering before mechanical assembly.

## Limitations & Constraints
The control loop and signal chain behave as intended; I closed the project as a proof-of-concept because of physical limits that no amount of software could fully overcome.
That's because while commercial, drone-grade gimbals use direct-drive brushless motors and stiff carbon frames, my prototype uses plastic-gear servos and cardboard: I was trying to accomplish the same “keep the payload leveled” job, but with components that have inherent backlash, flex and parasitic torque. QUI fix In retrospect, keeping such a fragile balance wasn't the most suitable project for what I had! But a good learning experience nonetheless.

* **Servo backlash:** Plastic gears and deadband prevent the load from following all the micro-corrections, so the system oscillates around the setpoint.
* **Cable routing:** Jumper wires add parasitic torque that opposes the motors.
* **Structural flex:** The cardboard chassis damps fast corrections and forced very conservative PID tuning (low Kp, light damping).
* **Top-heavy layout:** The base servo fights a large moment arm (arm + upper servo + platform + IMU); inertia and backlash made the base axis prone to undesired oscillation.

BUT! On the software side I achieved:
* Interfacing the ESP32 with an external I2C device (PCA9685).
* Reading and decoding raw accelerometer registers (MPU6050) and using trigonometry (`atan2`) to obtain roll/pitch angles.
* Implementing a low-pass filter to clean sensor noise.
* Writing a PID class from scratch with discrete time step `dt` and handling derivative kick.
* QUI idk è software side? Debugging brown-outs and power conflicts (logic vs motor supply).
QUI idk... While I was quite disappointed by how the cardboard rig shooks, at least I'd say that the control and signal pipeline remain valid. Can't be too sad about that!

## Future Work
The current design relies on a **reactive PID**, which was enough for my first project, but literature on USVs highlights that purely reactive control has inherent latency in highly dynamic sea states. Future work could move toward a simplified MPC (Model Predictive Control) or complementary/Kalman filtering to anticipate tilt, instead of only reacting to it.

## Bill of Materials
* **Microcontroller:** ESP32 (38-pin)
* **IMU:** MPU6050 (6-axis accelerometer & gyroscope)
* **Actuators:** 2× SG90 micro servos (180°)
* **PWM driver:** PCA9685 (16-channel, 12-bit, I2C)
* **Power:** 5V, 3A DC supply (isolated from ESP32 logic)
* **Chassis:** Custom cardboard structure
* Breadboard, Jumper wires

---

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
