# Logbook
This is a step-by-step account of how the project started, how it was built and where it stopped. Some initial choices (e.g. USB adapter for motor power) are described first; later sections reflect the final wiring and firmware state.

## First steps...
With my Software Engineering and Database Administration courses out of the way and the end of the Winter 2026 exam season, I wanted to try my hand at electronics. I jumped straight into wiring the ESP32 to the PCA9685 PWM driver; a 38-pin ESP32 turned out to be a large fella, taking up almost the entire width of a standard breadboard and leaving no room for jumper wires on its sides, so the breadboard was demoted to just an oversized power strip at the center of the system. Not very elegant already!

**Initial wiring scheme:**
* ESP32 `3V3` → PCA9685 `VCC` (logic)
* ESP32 `GND` → PCA9685 `GND`
* ESP32 `GPIO 21` → PCA9685 `SDA` (I2C data)
* ESP32 `GPIO 22` → PCA9685 `SCL` (I2C clock)
* PCA9685 channels 0 and 1 → servos (PWM, V+, GND)
* Motor power was first attempted via a USB adapter with screw terminals → PCA9685 green terminal block (note: later superseded as seen in section "Power distribution")

## ...First Obstacles
As I had my first attempt at stripping wires to connect screw terminals, power delivery bit me back immediately and the servos refused to behave. Or move. At all, for like a week. I had originally bought a QUI idk USB-to-terminals adapter to use with an existing USB extension cable I had, but the latter turned out to be faulty and nearly damaged components. That meant a bunch of tests to see who the culprit was, what had survived, and effectively a full restart. For a while losing my extension cable also meant I had to kneel by the wall for my power supply, until I decided for a solution.

Meanwhile, the MPU6050 arrived with its header pins unsoldered, so I had my first go at soldering! I enjoyed the process and the fumes and QUI idk damaging the furniture, but the results were rough — blobs and bubbles instead of the nice shiny cones the YouTube tutorials promised. And I'll blame it half on skill and half on the 13€ Amazon soldering kit. Despite the hardware gore, QUI idk gli indirizzi per mpu vs pca sono giusti? running a simple I2C scanner script and finally seeing the `0x40` address pop up on the Serial Monitor finally felt like some real IoT, and I got hold of a baseline PID draft to eventually map the MPU’s tilt angles to the servos *(I wouldn't discover until later that my rough soldering job was causing intermittent I2C failures — more on that in section "MPU6050 integration")*.

## Power distribution: from USB adapter to dedicated PSU
After ruling out other makeshift options and powerbanks, I switched to a multi-voltage AC/DC wall supply with interchangeable plugs, including one with screw terminals to reach my PCA9685. One SG90 servo under full load draws about 0.6–0.7A, so my two servos together stay around 1.2–1.5A and a 5V, 3A unit was a good fit.

**Final power and logic wiring:**
* Supply positive → double wire → PCA9685 green block `V+`
* Supply negative → double wire → PCA9685 green block `GND`
* ESP32 `3V3` → breadboard red rail (logic)
* ESP32 `GND` → breadboard blue/black rail
* PCA9685 pin `VCC` → breadboard red (logic only — not the green block)
* PCA9685 pin `GND` → breadboard blue/black
* PCA9685 `OE` (Output Enable) → `GND` (so the outputs are forced enabled; some boards disable them if external power comes up late, eliminating another potential cause of my initial servo issues)
* MPU6050 `VCC`/`GND` → same breadboard rails
* ESP32 `GPIO 21` → PCA9685 `SDA` and MPU6050 `SDA` (bus)
* ESP32 `GPIO 22` → PCA9685 `SCL` and MPU6050 `SCL` (bus)
* Servo 1 (pitch) → PCA9685 channel 0 (brown → `GND`, red → `V+`, orange → `PWM`)
* Servo 2 (roll) → PCA9685 channel 1

**Other time-consuming power distribution lessons learned:**
* Smart chargers that only supply current when they detect a data handshake are useless for powering a dumb PCA9685 board.
* Single Dupont wires per rail were insufficient for the current spikes of two servos (I moved to two wires in parallel for the main GND).
* Screw terminals must grip copper, not the plastic insulation.

## CAD simulation
To avoid frying more parts and myself while I waited for my new power supply to arrive, I stepped back and modelled the design in CAD. I learned the basics of CAD on browser-based Onshape, which was in German for some reason. I used it in German. I don't know German.
The aim was to check that the roll–pitch gimbal could reach the required angles without self-collision, to reason about stabilization and its kinematics, and to have a nice simulation to keep my morale up while I fixed the real-world complications.

**Forward kinematics:** Testing the operational range of the joints (here limited to ±45° to mimic realistic wave compensation) to ensure no mechanical binding.

**Inverse kinematics:** Anchoring the sensor platform in space while freeing the base. This validated the core stabilization logic: dynamically adjusting the arms to keep the payload perfectly horizontal despite the base simulating boat roll/pitch.

## Mechanical assembly
Assembly was honestly my favorite part! I assumed my bricolage would have turned out much uglier, but my cardboard pieces ended up connecting to the servo horns quite nicely with screws. I added fillets to the cardboard cuts to remove sharp edges and reduce stress, and implemented some basic strain relief so the electronics weren't left hanging by their Dupont wires. The kinematic chain is:

* **Base servo (pitch):** Fixed to the static base; arm in the roll axis.
* **Straight cardboard arm:** Mounted on the base servo horn.
* **Upper servo (roll):** Mounted on top of the arm, rotated 90°.
* **Top platform:** Fixed to the upper servo horn, reinforced with a small folded cardboard tab for rigidity.

Before locking the horns and platform, the servos were centered at 90° using a small sketch that drives both to PWM 375 and holds them there before assembly. The base servo had noticeable play (backlash); that was identified early as the main threat to stable PID behaviour, since a control loop mathematically expects an immediate physical response to every single micro-command it sends.

## Sweep test and PCA9685 troubleshooting
Once assembled, I ran a sweep sketch to move both servos back and forth between minimum and maximum PWM to verify the mechanical chain. It was here that I learned that dealing with Chinese clone hardware requires some specific firmware workarounds. Three fixes were absolutely essential for me to get the ESP32 talking to the PCA9685:

1. **Explicit I2C address and bus:** Using `Adafruit_PWMServoDriver(0x40, Wire)` instead of the default empty constructor. This forced the ESP32 to talk to the specific 0x40 address on the standard I2C pins, rather than guessing.
2. **I2C before PCA init:** Calling `Wire.begin(21, 22)` before `pwm.begin()`, otherwise the library might initialize using the wrong default pins and the PCA never sees the commands.
3. **Oscillator frequency for clones:** Many clone boards use a cheaper 27 MHz crystal instead of the standard 25 MHz. Calling `pwm.setOscillatorFrequency(27000000)` so that the PWM timing matches the hardware; before I added this line, the SG90 servos received garbled signals and often refused to move.

## MPU6050 integration
With the motors moving it was time to tackle the MPU6050, which shares the same I2C bus with the PCA. Raw acceleration data was pulled from register `0x3B` (reading six consecutive bytes for X, Y, and Z axes) and combined using bitwise shift and OR operations (`Wire.read()<<8 | Wire.read()`) to reconstruct the 16-bit values. Finally the `atan2` trigonometric function on the gravity vector converted those raw numbers into human-readable roll and pitch degrees.
Obviously my cardboard base was far from perfectly level, so the resting angles were hovering around 5° off-center. Instead of rebuilding the chassis, I just implemented a software tare by subtracting the resting offset from every reading.

My first usable run was spoiled by intermittent I2C failures: the Serial Monitor showed garbage values (like a constant `-129`) and then froze. A quick bypass test by pressing the Dupont tips directly onto the solder pads confirmed the board wasn't dead, just poorly connected on the `GND` and `SCL` pins because of my terrible soldering. Redoing the solder joints properly resolved the freezing issue and the onboard green LED finally told me the IMU was happy and powered!

## PID and filter
With both the IMU and the servos finally behaving, the last step was closing the loop: `Read Angle → Calculate PID → Output PWM`. I wrote a custom PID class from scratch (see `PID.h` / `PID.cpp`) that calculates the required correction based on the setpoint (0°), the measured angle, and the discrete delta-time (`dt`) calculated using `millis()`. I then added this output correction to my servo center (375) and clamped it with a `constrain()` function to protect the cardboard mechanics from ripping themselves apart.

Initially, I fired up both axes at once. The upper (roll) servo responded beautifully. The base (pitch) servo however kept going crazy, driving the whole tower into violent oscillations. Here is what I tried to tame it:
* **Fighting the deadband:** For tiny errors (like 1 degree), the PID correctly calculated a single step correction (e.g., PWM 376 instead of 375), but the cheap plastic gears couldn't physically resolve a 1-step difference. The mechanical deadband swallowed these small corrections whole, making the software's precision useless.
* **Adding a low-pass filter:** Implementing a simple software filter (taking 80% of the previous value and 20% of the new reading) which successfully smoothed out the angle readings and reduced jitter caused by inherent sensor noise and the vibrations of the motors themselves.
* **The derivative kick:** To damp the swinging base, I introduced the Derivative term (`Kd`). But with my main loop running at ~15 ms, dividing a normal `Kd` value (like 0.8) by a tiny `dt` produced mathematically massive spikes. It essentially kicked the servo to its maximum limit instantly, locking it in place. Dropping `Kd` to a micro-value (0.05) and softening `Kp` (0.5) yielded a gentler response, but it couldn't mask the underlying physics: the base servo was fighting a huge moment arm and top-heavy inertia, and the gear backlash made overshooting inevitable.

## Physical limits and closure
So, the software pipeline was a success: `IMU → Filter → PID → PWM`. The upper axis stabilized the roll acceptably well. However, the physical limits of the hardware (severe backlash, structural flex, elastic cable torque, and a top-heavy layout) meant that the full stable two-axis stabilization I hoped for was unattainable from my prototype. No amount of software tuning can magically fix mechanical compliance :(

I decided to close the project here as a somewhat successful proof-of-concept. My cardboard structure and cheap servos were the bottleneck, but the control and signal-processing goals I set out to learn were met. From a software and systems integration standpoint, I successfully:

* Interfaced an ESP32 with an external I2C module (PCA9685).
* Read, decoded, and applied trigonometry to raw accelerometer registers (MPU6050) to obtain spatial orientation.
* Implemented a digital low-pass filter to clean up noisy sensor data.
* Wrote a custom PID class handling discrete time steps (`dt`) and output saturation.
* Diagnosed and resolved bare-metal and power distribution issues.
