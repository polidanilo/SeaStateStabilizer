# 2-Axis Roll-Pitch Gimbal
I wanted to design and build a 2-DoF platform stabilization system: a first project to finally put to use the bunch of servos, PCBs, wiring and the ESP32 I had bought months ago when I wanted to build myself a little robot. Nowadays, I'm focusing on marine robotics (and more broadly on cyber-physical systems) as ROVs / AUVs / USVs are a great interest of mine! So I chose this idea as a simple way to break free of analysis paralysis and take a first step into marine technology, control theory, electronics and the fundamentals of robotics.

For day-by-day hardware troubleshooting, notes and development details, see [LOGBOOK.md](LOGBOOK.md).

<p align="center">
  <img src="assets/cad/active_stabilization.gif" alt="Active Stabilization Inverse Kinematics" width="80%">
  <br>
  <em>Active Stabilization Simulation (Inverse Kinematics): The base (simulating a vessel) pitches and rolls, while the control loop dynamically adjusts the mechanical arms to keep the sensor payload perfectly horizontal.</em>
</p>

## Overview
Inspired by the engineering challenges of payload stabilization on Uncrewed Surface Vessels (USVs), where roll and pitch degrade the accuracy of hydroacoustic sensors like echosounders, this project explores the same mechatronic principles of fin stabilizers: Motion Reference Unit-like sensing and active compensation on a small-scale, low-cost prototype. Its primary goal is to keep a top-mounted sensor payload (MPU6050 IMU) perfectly horizontal by actively counteracting external disturbances, simulating the motion of a vessel in rough seas. 

<br>
<table width="100%" style="max-width: 800px; margin: 0 auto; border-collapse: collapse; border: 1px solid #d0d7de;">
  <tr>
    <td width="30%" align="center" valign="middle" style="padding: 10px; border: none;">
      <img src="https://upload.wikimedia.org/wikipedia/commons/a/a4/Flossenstabilisatoren_schematisch_en.png" alt="Schematic of an active fin stabilizer system" width="100%" style="max-width: 200px; height: auto;">
    </td>
    <td width="70%" align="center" valign="middle" style="padding: 10px; border: none;">
      <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/4c/Polarstern_stabilizer_hg.jpg/1280px-Polarstern_stabilizer_hg.jpg" alt="Extended fin stabilizer on research vessel POLARSTERN" width="100%" style="height: auto;">
    </td>
  </tr>
  <tr>
    <td colspan="2" align="center" style="padding: 12px; border-top: 1px solid #d0d7de; font-size: 0.85em; line-height: 1.4; color: #555;">
<em>Schematic illustrating the kinematic principle of an active fin stabilizer system (left). While macro-scale fins (right) move the entire vessel, my project explores analog mechatronic challenges such as Sensor Fusion and Discrete PID Control applied to payload stabilization on a micro-scale (Sources: Schematic by <a href="https://commons.wikimedia.org/w/index.php?curid=129736124">Lämpel</a>, CC BY-SA 4.0; Photo by <a href="https://commons.wikimedia.org/wiki/File:Polarstern_stabilizer_hg.jpg">Hannes Grobe/AWI</a>, CC BY 3.0)</em>
    </td>
  </tr>
</table>
<br>

Building the system from scratch was a practical dive into:
* **Mechanics and CAD:** Checking operational limits and kinematics behaviour in Onshape (joint ranges, no self-collision) and designing a mechanical layout that isolates the electronics from the moving payload.
* **Hardware and power distribution:** Learning the basics of IoT, electronics, and soldering to build a stable power architecture; isolating the 3.3V logic (ESP32) from the 5V actuator supply (PCA9685 + servos) to prevent motor current spikes from browning out the microcontroller.
* **Control and signal processing:** Implementing a custom Proportional-Integral-Derivative (PID) controller and a software low-pass filter to turn raw MPU6050 accelerometer data into stable roll/pitch estimates and servo commands.

<p align="center">
  <img src="assets/cad/forward_kinematics.gif" alt="Forward Kinematics Range of Motion Test" width="60%">
  <br>
  <em>Forward Kinematics: Validating the operational limits (±45°) to ensure no mechanical binding or self-collision in the structural design.</em>
</p>

## Features
The repository is organized around focused single-purpose **`.ino`** sketches that served me in this project, each targeting one layer of the system:

**`firmware/PID.h`** and **`firmware/PID.cpp`**

Custom PID controller written in C++ with discrete time step `dt`  computed from `millis()`, integral windup cap, derivative kick handling, and output saturation via `constrain()`.

**`firmware/PID.ino`**

Main control loop: reads roll/pitch from MPU6050, feeds both axes through independent PID instances, and writes corrected PWM values to the PCA9685.

**`firmware/tests/TestMPU.ino`**

IMU integration: wakes MPU6050 from sleep (register `0x6B`), reads six raw accelerometer bytes from `0x3B`, reconstructs 16-bit values with shift-and-OR, computes roll and pitch via `atan2`, and applies a software low-pass filter (80 % previous sample, 20 % new) to suppress noise and structural vibration.

**`firmware/tests/SweepTest.ino`**

Mechanical validation: sweeps both servos through min / center / max PWM to verify the full kinematic chain before closing the control loop. Identified the PCA9685 clone oscillator mismatch (27 MHz instead of 25 MHz) that caused silent servo failures; fix: `pwm.setOscillatorFrequency(27000000)`.

**`firmware/tests/MidPointServo.ino`**

Centers both servos at 90° (PWM 375) before locking horns and platform to the chassis — essential for symmetric range of motion.

**`firmware/tests/I2Cscanner.ino`**

Scans the I2C bus and prints detected addresses; used to confirm PCA9685 (`0x40`) and MPU6050 (`0x68`) are visible before any integration work.

### Hardware integration notes 
* Isolated servo and logic rails: SG90 servos draw 0.6–0.7 A each (~1.5 A peak combined), which would brown out the ESP32 if shared. The PCA9685 green terminal block carries motor power independently from the 3.3 V logic supply.
* My PCA9685 required an explicit I2C initialization order in `Wire.begin(21, 22)` before `pwm.begin()` and explicit oscillator calibration in `setOscillatorFrequency(27000000)` — undocumented gotchas that cause silent servo failures on most Chinese clones.
* the MPU6050 arrived to me with unsoldered header pins; after my first try hand-soldering them, I encountered intermittent I2C failures caused by cold joints on the GND and SCL pins, which I quickly diagnosed via direct pad bypass and resolved by re-soldering them more carefully.

### Bill of materials
| Component | Part |
|-----------|------|
| Microcontroller | ESP32 (38-pin) |
| IMU | MPU6050 (6-axis accelerometer + gyroscope) |
| Actuators | 2× SG90 micro servo (180°) |
| PWM driver | PCA9685 (16-ch, 12-bit, I2C) |
| Power supply | 5V/3A DC (isolated from logic) |
| Chassis | Custom cardboard structure |
| Other | Breadboard, jumper wires |

<p align="center">
  <img src="assets/photos/prototype_lab.jpg" alt="Hardware Prototype" width="80%">
  <br>
  <em>The assembled hardware testbench, highlighting the isolated power distribution and I2C wiring.</em>
</p>

## Limitations
The control loop and signal chain behave as intended; I closed the project as a proof-of-concept due to physical constraints inherent to the hardware. Commercial drone-grade gimbals use direct-drive brushless motors and rigid carbon frames — this prototype uses plastic-gear servos and cardboard, which introduce backlash, flex and parasitic torque that a software PID alone cannot fully compensate.
* **Servo backlash:** Plastic gears and deadband prevent the load from following all the micro-corrections, so the system oscillates around the setpoint.
* **Cable routing:** Jumper wires add parasitic torque that opposes the actuators.
* **Structural flex:** The cardboard chassis damps fast corrections and forced very conservative PID tuning (low Kp, light damping).
* **Top-heavy layout:** The base servo fights a large moment arm (arm + upper servo + platform + IMU); inertia and backlash made the base axis prone to undesired oscillation.

### Future work
The current design relies on a reactive PID, which was enough for my first project, but literature on USVs highlights that purely reactive control has inherent latency in highly dynamic sea states. Future work could explore:
* **Complementary or Kalman filter** on the IMU to fuse accelerometer and gyroscope data for more stable angle estimates.
* **Model Predictive Control (MPC)** or other intelligent solutions, to anticipate tilt rather than only react to it — relevant for USV applications where sea-state prediction is feasible.
* **Rigid chassis** (laser-cut acrylic or 3D-printed frame) and metal-gear or brushless actuators to eliminate backlash and structural flex.
