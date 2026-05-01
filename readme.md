# Visual Time & Distance Scheduler: The "No-Number" Clock

## 🌟 The Goal: Making Time Tangible

Time is a highly abstract concept. Traditional clocks—whether digital numbers or analog hands—require a level of literacy and mathematical understanding that young children, the elderly, or individuals with certain cognitive differences may not possess. 

Telling a toddler "we leave in 45 minutes" or showing them a timer often leads to frustration because numbers in a circle don't mean anything to them. 

**The Visual Time & Distance Scheduler solves this by making time physical.** Instead of relying on numbers, this device slowly moves a physical, tangible object (like a cardboard school bus, a toy, or a token) along a string towards a destination. 

The user instantly understands: **"When the bus reaches the house, it's time to go."** It provides a constant-motion visual countdown, making the concept of "time remaining" intuitively accessible to anyone, regardless of their ability to read.

---

## ⚙️ How the Program Works

At its core, this system is a highly precise, ultra-slow robotic winch. Here is how the software (written for the Arduino) brings this concept to life:

1. **User Input:** Using a standard infrared remote control, a parent or caregiver navigates a simple menu on an LCD screen. They input three key pieces of information:
   * **Start Time** (e.g., 7:00 AM)
   * **End Time** (e.g., 8:00 AM)
   * **Total Distance** (e.g., 150 centimeters of string)

2. **Time Math:** The system uses a highly accurate Real-Time Clock (RTC) chip to calculate the exact number of minutes between the Start and End times, even safely handling overnight schedules. 

3. **The "Fractional RPM" Magic:** This is where the core software logic shines. Standard software libraries for motors only understand speed in whole numbers (1 RPM, 2 RPM, etc.). However, moving a toy 150 centimeters over an entire hour requires an incredibly slow, *sub-integer* speed (like 0.3 RPM). 
   * Our program uses a custom algorithm that mathematically "tricks" the motor library. It calculates a fractional speed, rounds it up to a safe integer, and then scales down the *steps per revolution* to force the motor to pulse at a perfectly slow, steady, and accurate crawl. 

4. **Safe Execution:** Once started, the system locks into a dedicated running mode. It includes safety checks to ensure the math never crashes (preventing "divide-by-zero" errors if the requested speed is exceptionally slow) and steadily drives the object to its destination right on time.

---

## 🧩 The Hardware Components

This project uses readily available, inexpensive hobbyist electronics to achieve industrial-level timing precision:

* **The Brain (Arduino Mega 2560):** The main computer. We use the Mega version because it has plenty of connection pins, allowing it to talk to the screen, the remote, and the motor all at the same time without overwhelming the system.
* **The Muscle (28BYJ-48 Stepper Motor & ULN2003 Driver):** Unlike normal motors that just spin fast when given power, a *stepper* motor moves in microscopic, highly controlled "steps." This allows for smooth, incredibly slow pulling power.
* **The Metronome (DS3231 RTC):** A Real-Time Clock module with its own battery backup. This acts as the project's stopwatch, ensuring the math for the travel time is flawless.
* **The Eyes (VS1838B IR Receiver):** A tiny sensor that reads infrared signals from a standard TV-style remote control, allowing for wireless setup.
* **The Display (16x2 Character LCD):** A simple screen that guides the user through the setup steps. It is wired in "8-bit mode" to ensure the screen doesn't glitch or flicker when the motor draws power.