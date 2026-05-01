/**
 * ============================================================================
 * Project: Visual Time & Distance Scheduler (Arduino Mega)
 * ============================================================================
 * * Description:
 * This sketch controls a stepper motor to travel a precise distance over a 
 * specific duration defined by the user. It features a custom "Fractional RPM" 
 * algorithm to achieve ultra-slow, sub-integer speeds that the standard 
 * Stepper library cannot normally handle.
 * * The user interfaces with the system via an IR Remote and an 8-bit LCD.
 * * Key Features:
 * - User Input: Direction (CW/CCW), Start Time, End Time, Distance (cm).
 * - Precision Timing: Uses DS3231 RTC for time calculations.
 * - Stability: 8-bit LCD mode minimizes electrical noise during motor pulses.
 * - Smart Math: Dynamically adjusts 'steps_per_rev' to fake fractional RPMs.
 * - Safety: Includes divide-by-zero protection for extremely slow speeds.
 * * Hardware Requirements:
 * - Arduino Mega 2560
 * - 16x2 LCD Display (Parallel 8-bit wiring)
 * - DS3231 Real Time Clock (I2C)
 * - IR Receiver Module (e.g., VS1838B)
 * - 28BYJ-48 Stepper Motor + ULN2003 Driver Board
 * * Wiring (Mega 2560):
 * - LCD (8-bit): RS:13, E:12, D0:7, D1:6, D2:5, D3:4, D4:11, D5:10, D6:9, D7:8
 * - IR Receiver: Pin 38
 * - Stepper:     IN1:40, IN2:42, IN3:44, IN4:46
 * - RTC:         SDA:20, SCL:21
 * * Dependencies:
 * - LiquidCrystal.h (Standard)
 * - IRremote.h      (v3.x or higher)
 * - Stepper.h       (Standard)
 * - RTClib.h        (Adafruit or similar)
 * * ============================================================================
 */

#include <LiquidCrystal.h>
#include <IRremote.h>
#include <Stepper.h>        
#include <RTClib.h>        
#include <stdio.h>         
#include <stdlib.h>        
#include <math.h>          

// ============================================================================
// Objects & Hardware Initialization
// ============================================================================

RTC_DS3231 rtc; 

// Initialize LCD in 8-bit mode for maximum signal stability
// Pins: RS, E, D0, D1, D2, D3, D4, D5, D6, D7
LiquidCrystal lcd(13, 12, 7, 6, 5, 4, 11, 10, 9, 8);

// ============================================================================
// Constants & Definitions
// ============================================================================

const int IR_RECEIVE_PIN = 38; 

// Steps per revolution for the 28BYJ-48 motor (geared down)
const int ACTUAL_STEPS_PER_REVOLUTION = 2048; 

// Motor Pins (using ULN2003 driver sequence)
const int MOTOR_PIN_1 = 40; 
const int MOTOR_PIN_2 = 42; 
const int MOTOR_PIN_3 = 44; 
const int MOTOR_PIN_4 = 46; 

// Microsecond delay added between steps to ensure torque stability
const int MOTOR_STEP_STABILITY_DELAY_US = 100; 

// Physical properties
const float WHEEL_CIRC_M = 0.188; // Circumference of the drive wheel in meters
const int MAX_EFFECTIVE_RPM = 17; // Mechanical limit of the 28BYJ-48

// --- IR Remote Hex Codes ---
// Adjust these codes if using a different remote
#define IR_CODE_0         0xE916FF00 
#define IR_CODE_1         0xF30CFF00 
#define IR_CODE_2         0xE718FF00 
#define IR_CODE_3         0xA15EFF00 
#define IR_CODE_4         0xF708FF00 
#define IR_CODE_5         0xE31CFF00 
#define IR_CODE_6         0xA55AFF00 
#define IR_CODE_7         0xBD42FF00 
#define IR_CODE_8         0xAD52FF00 
#define IR_CODE_9         0xB54AFF00 
#define IR_CODE_PLAYPAUSE 0xBF40FF00 // Acts as "Enter/Next"
#define IR_CODE_BACKWARD  0xBC43FF00 // Acts as "Backspace"
#define IR_CODE_POWER     0xBA45FF00 // Acts as "Reset"

// ============================================================================
// Global Variables & State Machine
// ============================================================================

/**
 * State machine to track user progress through the setup menu.
 */
enum InputState {
  ENTERING_DIRECTION,    // Step 1: CW or CCW
  ENTERING_START_TIME,   // Step 2: Start Time (HHMM)
  ENTERING_END_TIME,     // Step 3: End Time (HHMM)
  ENTERING_DISTANCE,     // Step 4: Distance in cm
  CONFIRM_AND_CALCULATE, // Step 5: Review calculated RPM
  EXECUTING_MOTOR_RUN,   // Step 6: Motor is moving
  SHOW_RESULTS_IDLE,     // Step 7: Done
  INPUT_ERROR            // Error state
};

InputState currentInputState = ENTERING_DIRECTION; 

// Input buffer
char inputDigits[5] = "_"; 
int currentDigitIndex = 0;

// Operational Variables
bool spinClockwise = true; 
int startHour = -1, startMinute = -1;
int endHour = -1, endMinute = -1;
long timeDeltaMinutes = 0; 
int distanceCm = -1;

// Motor Calculation Variables
float desiredRPM_float = 0.0; 
int lib_rpm_setting = 0;      // "Fake" integer RPM sent to library
int lib_steps_setting = 2048; // "Fake" steps/rev sent to library
char floatDisplayBuffer[10]; 

// Timing & Execution
unsigned long motorRunStartTime = 0;
unsigned long motorRunDurationMillis = 0;
bool motorIsRunning = false;

// UI Blinking
unsigned long lastBlinkTime = 0;
bool cursorState = true;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Resets the input buffer string for the next stage of data entry.
 * @param maxDigits The maximum number of digits allowed for the new stage.
 */
void resetInputBuffer(int maxDigits) {
  for (int i = 0; i < 4; i++) inputDigits[i] = (i < maxDigits) ? '_' : '\0';
  inputDigits[maxDigits] = '\0';
  currentDigitIndex = 0;
}

/**
 * Updates the LCD display based on the current state of the state machine.
 * Handles string formatting for RPM and menu prompts.
 */
void updateLcd() {
  noInterrupts(); // Disable interrupts to prevent display corruption
  lcd.clear();
  delayMicroseconds(2000); 

  switch (currentInputState) {
    case ENTERING_DIRECTION:
      lcd.setCursor(0, 0); lcd.print("Set Direction:");
      lcd.setCursor(0, 1); lcd.print("0=CW,1=CCW: ");
      lcd.print(inputDigits[0]); 
      break;
      
    case ENTERING_START_TIME:
      lcd.setCursor(0, 0); lcd.print(spinClockwise ? "Dir:CW Start:" : "Dir:CCW Start:");
      lcd.setCursor(0, 1); lcd.print("Time(HHMM):"); lcd.print(inputDigits); 
      break;
      
    case ENTERING_END_TIME:
      lcd.setCursor(0, 0); lcd.print("End Time(HHMM):");
      lcd.setCursor(0, 1); lcd.print(inputDigits);
      break;
      
    case ENTERING_DISTANCE:
      lcd.setCursor(0, 0); lcd.print("Dist (cm):");
      lcd.setCursor(0, 1); lcd.print(inputDigits);  
      break;
      
    case CONFIRM_AND_CALCULATE:
      lcd.setCursor(0,0); lcd.print("Confirm Setup:");
      lcd.setCursor(0,1);
      lcd.print("RPM:");
      // Use dtostrf for float formatting (standard print() is limited on Arduino)
      if (desiredRPM_float < 0.01f) dtostrf(desiredRPM_float, 7, 5, floatDisplayBuffer);
      else dtostrf(desiredRPM_float, 5, 2, floatDisplayBuffer);
      lcd.print(floatDisplayBuffer); lcd.print(" GO?");
      break;
      
    case EXECUTING_MOTOR_RUN:
      lcd.setCursor(0,0); lcd.print("Tgt RPM:");
      dtostrf(desiredRPM_float, 8, 5, floatDisplayBuffer);
      lcd.print(floatDisplayBuffer);
      lcd.setCursor(0,1); lcd.print("Running...");
      break;
      
    case SHOW_RESULTS_IDLE:
      lcd.setCursor(0,0); lcd.print("Test Finished");
      lcd.setCursor(0,1); lcd.print("RPM:"); lcd.print(desiredRPM_float, 5);
      break;
      
    case INPUT_ERROR:
      lcd.setCursor(0, 0); lcd.print("Invalid Input!");
      break;
  }
  interrupts();
}

/**
 * Blinks the LCD cursor without blocking program execution.
 * Only active during data entry states.
 */
void blinkCursor() {
  if (millis() - lastBlinkTime > 500) {
    lastBlinkTime = millis();
    cursorState = !cursorState;
    noInterrupts();
    if (cursorState) lcd.blink(); else lcd.noBlink();
    interrupts();
  }
}

// ============================================================================
// Main Setup & Loop
// ============================================================================

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2);
  
  // Initialize RTC
  if (!rtc.begin()) { 
    // Halt if RTC is missing
    lcd.print("RTC Error!");
    while(1); 
  }
  // Uncomment the line below once to set the time, then re-comment and upload.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  
  // Initialize IR Receiver
  IrReceiver.begin(IR_RECEIVE_PIN);
  
  // Initialize UI
  resetInputBuffer(1);
  updateLcd();
}

void loop() {
  // Only blink cursor if we are in an input state
  if (currentInputState < CONFIRM_AND_CALCULATE) blinkCursor();

  // Check for IR Signal
  if (IrReceiver.decode()) {
    unsigned long raw = IrReceiver.decodedIRData.decodedRawData;
    
    // Filter out repeat codes (0) and errors (0xFFFFFFFF)
    if (raw != 0 && raw != 0xFFFFFFFF) {
      int num = -1;
      
      // Map IR codes to digits
      switch(raw) {
        case IR_CODE_0: num = 0; break; case IR_CODE_1: num = 1; break;
        case IR_CODE_2: num = 2; break; case IR_CODE_3: num = 3; break;
        case IR_CODE_4: num = 4; break; case IR_CODE_5: num = 5; break;
        case IR_CODE_6: num = 6; break; case IR_CODE_7: num = 7; break;
        case IR_CODE_8: num = 8; break; case IR_CODE_9: num = 9; break;
      }

      // --- Digit Entry Handling ---
      if (num != -1 && currentDigitIndex < 4) {
        inputDigits[currentDigitIndex++] = (char)('0' + num);
        updateLcd();
        
      // --- Backspace Handling ---
      } else if (raw == IR_CODE_BACKWARD && currentDigitIndex > 0) {
        inputDigits[--currentDigitIndex] = '_';
        updateLcd();
        
      // --- "Enter" / Next Step Handling ---
      } else if (raw == IR_CODE_PLAYPAUSE) {
        
        // State 1: Direction
        if (currentInputState == ENTERING_DIRECTION) {
          spinClockwise = (inputDigits[0] == '0');
          currentInputState = ENTERING_START_TIME; resetInputBuffer(4);
        
        // State 2: Start Time
        } else if (currentInputState == ENTERING_START_TIME) {
          startHour = atoi(inputDigits); 
          currentInputState = ENTERING_END_TIME; resetInputBuffer(4);
        
        // State 3: End Time
        } else if (currentInputState == ENTERING_END_TIME) {
          endHour = atoi(inputDigits);
          currentInputState = ENTERING_DISTANCE; resetInputBuffer(4);
        
        // State 4: Distance & CALCULATIONS
        } else if (currentInputState == ENTERING_DISTANCE) {
          distanceCm = atoi(inputDigits);
          
          // --- TIME CALCULATION LOGIC ---
          // 1. Parse HHMM integers into Hours and Minutes
          int sH = startHour / 100; 
          int sM = startHour % 100;
          int eH = endHour / 100; 
          int eM = endHour % 100;
          
          long startTotal = (sH * 60) + sM;
          long endTotal = (eH * 60) + eM;
          
          timeDeltaMinutes = endTotal - startTotal;
          
          // 2. Handle overnight case (e.g. 2300 to 0100)
          if (timeDeltaMinutes <= 0) timeDeltaMinutes += 1440;
          
          float targetRevs = ((float)distanceCm / 100.0) / WHEEL_CIRC_M;
          
          // --- FRACTIONAL RPM ALGORITHM ---
          // The standard Stepper library only supports integer RPMs (1, 2, 3...).
          // To achieve slower speeds (e.g., 0.5 RPM), we:
          // 1. Round the desired RPM UP to the nearest integer.
          // 2. Scale the 'steps_per_rev' setting DOWN by the same ratio.
          // This tricks the library into pulsing at the correct, slower frequency.
          
          desiredRPM_float = targetRevs / (float)timeDeltaMinutes;
          
          // Step A: Determine "fake" integer RPM
          lib_rpm_setting = ceil(desiredRPM_float);
          if (lib_rpm_setting < 1) lib_rpm_setting = 1;

          // Step B: Calculate reduction ratio
          float ratio = desiredRPM_float / (float)lib_rpm_setting;
          
          // Step C: Calculate "fake" steps 
          lib_steps_setting = round(ratio * ACTUAL_STEPS_PER_REVOLUTION);

          // CRITICAL SAFETY CHECK: 
          // If speed is extremely slow, steps may round to 0. 
          // Sending 0 to Stepper library causes a divide-by-zero crash.
          if (lib_steps_setting < 1) {
             lib_steps_setting = 1; 
          }

          currentInputState = CONFIRM_AND_CALCULATE;
          
        // State 5: Confirm & Execute
        } else if (currentInputState == CONFIRM_AND_CALCULATE) {
          motorRunDurationMillis = (unsigned long)timeDeltaMinutes * 60000L;
          motorRunStartTime = millis();
          currentInputState = EXECUTING_MOTOR_RUN;
          motorIsRunning = true;
          updateLcd();
          
          // --- BLOCKING MOTOR RUN ---
          // Initialize stepper with the calculated "fake" settings
          Stepper motorRunner(lib_steps_setting, MOTOR_PIN_1, MOTOR_PIN_3, MOTOR_PIN_2, MOTOR_PIN_4);
          motorRunner.setSpeed(lib_rpm_setting);
          
          // Run loop until duration expires
          while (millis() - motorRunStartTime < motorRunDurationMillis) {
            motorRunner.step(spinClockwise ? 1 : -1);
            delayMicroseconds(MOTOR_STEP_STABILITY_DELAY_US);
          }
          
          motorIsRunning = false;
          currentInputState = SHOW_RESULTS_IDLE;
        }
        updateLcd();
        
      // --- Reset Handling ---
      } else if (raw == IR_CODE_POWER) {
        currentInputState = ENTERING_DIRECTION; resetInputBuffer(1); updateLcd();
      }
    }
    // Resume IR receiver to listen for next signal
    IrReceiver.resume();
  }
}
