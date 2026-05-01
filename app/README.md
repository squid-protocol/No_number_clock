# Visual Time & Distance Scheduler

## Purpose & Overview
This project is designed as a **no-number clock** for individuals who cannot read yet, are elderly, or have cognitive differences that make abstract concepts—like numbers in circles or digital readouts—difficult to understand. 

It conveys a sense of time through a completely visual and tangible medium. By physically moving an object (such as a "school bus" cutout) along a string from Point A to Point B over a user-defined duration, the device makes the concept of "time remaining" intuitively accessible without requiring literacy or numeracy.

## Core Features
* **Fractional RPM Algorithm:** Standard stepper libraries typically only support integer RPMs (1, 2, 3...). This project features a custom algorithm to achieve ultra-slow, sub-integer speeds by dynamically scaling down the `steps_per_rev` setting to trick the library into pulsing at a precise, slower frequency.
* **Crash Protection:** Includes critical safety logic to prevent divide-by-zero crashes that can occur when calculating extremely slow target speeds.
* **Precision Timekeeping:** Utilizes a DS3231 Real-Time Clock (RTC) for accurate duration calculations, including logic to handle overnight schedules seamlessly.
* **High-Stability Interface:** The primary build utilizes an 8-bit parallel LCD mode to minimize electrical noise and prevent display corruption during high-current stepper motor pulses.

## System Architectures
This project supports two different hardware frameworks depending on your deployment needs:

### 1. Primary Architecture (Arduino Mega 2560 - IR/LCD Control)
This is the standalone version controlled via a physical remote. The Arduino Mega 2560 was chosen specifically for its high pin count, allowing it to drive the LCD in parallel while simultaneously handling motor and IR receiver interrupts.
* **Microcontroller:** Arduino Mega 2560
* **Motor Control:** 28BYJ-48 Stepper Motor + ULN2003 Driver Board
* **Timekeeping:** DS3231 RTC Module
* **User Interface:** 16x2 Character LCD and an IR Receiver (e.g., VS1838B) with a standard NEC protocol remote

### 2. Alternate Architecture (ESP32 - Web Control)
This configuration ports the hardware logic to an ESP32 for web-controlled environments. 
* **Microcontroller:** ESP32 Development Board (3.3V Logic)
* **Stability Upgrades:** This setup highly recommends decoupling capacitors to manage the 3.3V logic: a 100nF ceramic capacitor for the RTC and a 10µF to 100µF electrolytic capacitor for the motor driver's power input.
* **Power & Grounding:** It is critical that the ESP32 (powered via USB) and the ULN2003 driver (powered via an external 5V supply) share a common ground reference. 

## Software Dependencies
To compile the primary Arduino Mega sketch (`app.ino`), install the following libraries via the Arduino Library Manager:
* `LiquidCrystal.h` (Standard)
* `Stepper.h` (Standard)
* `IRremote.h` (v3.x or higher)
* `RTClib.h` (Adafruit or similar)

## Standard Usage Workflow (Arduino Mega Build)
The state machine guides the user through five setup steps using the IR Remote:
1. **Direction:** Press `0` for Clockwise (CW) or `1` for Counter-Clockwise (CCW). Press `Play/Pause` to confirm.
2. **Start Time:** Enter the starting time in HHMM format (e.g., 0700). 
3. **End Time:** Enter the target arrival time in HHMM format.
4. **Distance:** Input the physical length of the travel path in centimeters.
5. **Execute:** The system will display the calculated target RPM. Confirm the execution to begin the blocking motor run.