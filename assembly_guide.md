# Visual Scheduler Assembly Guide

This guide provides step-by-step instructions for wiring the Visual Time & Distance Scheduler using standard prototyping practices (breadboards and jumper wires).

# Purpose

This project is designed as a **no-number clock** for humans who cannot read yet, are elderly, or have cognitive differences that make abstract concepts—like numbers in circles or digital readouts—difficult to understand.

It provides a sense of time conveyed through a **completely visual and tangible medium**. By physically moving an object from Point A to Point B over a set duration, the device makes the concept of "time remaining" instantly intuitively accessible without requiring literacy or numeracy.

## 🛠 Prerequisites

* **Arduino Mega 2560**
* **Breadboard** (Full size recommended)
* **Jumper Wires** (Male-to-Male for breadboard, Male-to-Female for modules)
* **10kΩ Potentiometer** (Essential for LCD contrast)
* **External 5V Power Supply** (For the motor)

## ⚠️ Standard Protocols & Safety

1. **Color Coding:**
   * **Red Wires:** Always use for **+5V** (VCC).
   * **Black Wires:** Always use for **Ground** (GND).
   * *Why?* This prevents accidental short circuits that can fry components.

2. **Common Ground (The Golden Rule):**
   * All components must share a single ground reference.
   * **Action:** Connect the **GND** of your external motor power supply to a **GND** pin on the Arduino Mega.

3. **Power Rails:**
   * Use the long +/- rails on the side of your breadboard.
   * Connect Arduino **5V** to the Red Rail (+).
   * Connect Arduino **GND** to the Blue/Black Rail (-).

## 🔌 Step-by-Step Wiring

### Step 1: The LCD Screen (The "Parallel" Method)

*Standard 16x2 LCDs require a lot of wires. Take your time here.*

**A. Power & Contrast**

1. **VSS (Pin 1):** Connect to Breadboard **GND**.
2. **VDD (Pin 2):** Connect to Breadboard **5V**.
3. **V0 (Pin 3 - Contrast):**
   * Place a **10kΩ Potentiometer** on the breadboard.
   * Connect one side leg to **5V**, the other side leg to **GND**.
   * Connect the **Middle Pin (Wiper)** to LCD **Pin 3 (V0)**.
   * *Tip:* You will turn this knob later to make the text visible.
4. **RW (Pin 5):** Connect to Breadboard **GND** (We only write to the screen, never read).
5. **A / LED+ (Pin 15):** Connect to **5V** (usually needs a 220Ω resistor if not built-in).
6. **K / LED- (Pin 16):** Connect to **GND**.

**B. Data Connections (8-Bit Mode)**
Connect these LCD pins directly to the Arduino Mega Digital Pins:
* **RS (Pin 4)** → Arduino **13**
* **E (Pin 6)** → Arduino **12**
* **D0 (Pin 7)** → Arduino **7**
* **D1 (Pin 8)** → Arduino **6**
* **D2 (Pin 9)** → Arduino **5**
* **D3 (Pin 10)** → Arduino **4**
* **D4 (Pin 11)** → Arduino **11**
* **D5 (Pin 12)** → Arduino **10**
* **D6 (Pin 13)** → Arduino **9**
* **D7 (Pin 14)** → Arduino **8**

### Step 2: The Stepper Motor (High Current)

*Never power the motor directly from the Arduino's 5V pin. It draws too much current and can reset the board.*

1. **Driver Board Input:**
   * **IN1** → Arduino **40**
   * **IN2** → Arduino **42**
   * **IN3** → Arduino **44**
   * **IN4** → Arduino **46**

2. **Driver Power:**
   * Connect the Driver **- (GND)** to your **External Power Supply GND**.
   * Connect the Driver **+ (5V)** to your **External Power Supply 5V**.

3. **Link Grounds:**
   * Run a wire from **External Power Supply GND** to the **Arduino GND**.

### Step 3: The RTC Module (I2C)

*The DS3231 uses the I2C protocol, which has dedicated pins on the Mega.*

1. **VCC** → Breadboard **5V**
2. **GND** → Breadboard **GND**
3. **SDA** → Arduino **20** (labeled SDA on some Megas)
4. **SCL** → Arduino **21** (labeled SCL on some Megas)

### Step 4: The IR Receiver

*Check your specific sensor's datasheet, but most (like VS1838B) follow this order facing the mesh:*

1. **Left Pin (Signal/Out)** → Arduino **38**
2. **Middle Pin (GND)** → Breadboard **GND**
3. **Right Pin (VCC)** → Breadboard **5V**

## ✅ Final Check

Before plugging in USB:

1. Are the **GNDs** tied together?
2. Is the Motor powered by the **external** supply?
3. Are the LCD data pins exactly matching the map? (Swapping D4/D5 is a common error).

**First Power Up:**
If the LCD lights up but shows no text, **turn the potentiometer knob** until white blocks or text appear.