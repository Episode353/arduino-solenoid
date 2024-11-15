#include <MIDIUSB.h>

// Define the flash duration (in milliseconds)
const unsigned long flashDuration = 30;

// Array to track the last flash time for each button's LED
unsigned long ledFlashTimers[16];
bool ledFlashing[16] = {false}; // Track whether each LED is in flash-off state

#include <EEPROM.h>

// Constants for preset handling
#define PRESET1_BUTTON 15
#define PRESET1_ADDRESS 0 // Start address in EEPROM for preset1
#define NOTES_COUNT 12 // Only saving buttons 1-12

// Timing and state tracking for the preset button
unsigned long presetButtonPressTime = 0;
bool presetButtonPressed = false;
// Pin definitions (unchanged)
#define clockPin 3
#define dataPin 5
#define latchPin 4
#define muxSigPin A0
#define muxS0Pin 6
#define muxS1Pin 7
#define muxS2Pin 8
#define muxS3Pin 9

int numOfRegisters = 2;
byte* registerState;
bool buttonStates[16];
bool prevButtonStates[16];

int currentNoteIndex = 0;   // Index to keep track of current note position in the active notes list
int octaveShift = 0;        // Variable to track the current octave shift
const uint8_t noteMap[12] = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}; // MIDI notes C4-B4
int activeNotes[12];
int activeNotesCount = 0;

const int encoderPinA = 10;
const int encoderPinB = 16;
int lastEncoderState = HIGH;

// Function states
bool function1State = false;  // false = OFF (no octave shifting), true = ON (octave shifting)
bool function2State = false;  // false = OFF (no arpeggio), true = ON (arpeggio)
bool function3State = false;  // false = OFF, true = ON (random note)

unsigned long lastNoteChangeTime = 0;  // Timer for note cycling
int bpm = 120;  // Default BPM
int max_bpm = 1000;  // Max BPM
int min_bpm = 30;  // Minimum BPM
int cycleInterval;  // Interval for note cycling in milliseconds

// Function toggle handler
void toggleFunction(int buttonIndex, bool isPressed) {
  // Toggle function 1 when button 12 is pressed or released
  if (buttonIndex == 12) {
    if (isPressed || !isPressed) {  // Trigger on both press and release
      function1State = !function1State;  // Toggle the state
      Serial.print("Function 1 ");
      Serial.println(function1State ? "ON" : "OFF");
    }
  }

  if (buttonIndex == 13) {
    if (isPressed || !isPressed) {  // Trigger on both press and release
      function2State = !function2State;  // Toggle the state
      Serial.print("Function 2 ");
      Serial.println(function2State ? "ON" : "OFF");
    }
  }

  if (buttonIndex == 14) {  // Toggle function 3 (Random note)
    if (isPressed || !isPressed) {  
      function3State = !function3State;
      Serial.print("Function 3 ");
      Serial.println(function3State ? "ON" : "OFF");
    }
  }

   if (buttonIndex == PRESET1_BUTTON) {
    if (isPressed) {
      presetButtonPressTime = millis(); // Record press time
      presetButtonPressed = true;
    } else {
      unsigned long pressDuration = millis() - presetButtonPressTime;
      presetButtonPressed = false;

      if (pressDuration >= 2000) {
        savePreset(PRESET1_ADDRESS); // Save preset if held for >2 seconds
      } else if (pressDuration < 2000) {
        loadPreset(PRESET1_ADDRESS); // Load preset if held for <2 seconds
      }
    }
  }
}

void setup() {
  registerState = new byte[numOfRegisters];
  for (size_t i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(muxS0Pin, OUTPUT);
  pinMode(muxS1Pin, OUTPUT);
  pinMode(muxS2Pin, OUTPUT);
  pinMode(muxS3Pin, OUTPUT);
  pinMode(muxSigPin, INPUT_PULLUP);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  Serial.begin(9600);
  clearShiftRegister();
}



void clearShiftRegister() {
  for (int i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }
  digitalWrite(latchPin, LOW);
  for (int i = 0; i < numOfRegisters; i++) {
    shiftOut(dataPin, clockPin, MSBFIRST, registerState[i]);
  }
  digitalWrite(latchPin, HIGH);
}

void readMuxButtons() {
  for (int i = 0; i < 16; i++) {
    digitalWrite(muxS0Pin, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin, (i & 8) ? HIGH : LOW);

    int buttonState = digitalRead(muxSigPin);
    bool isPressed = (buttonState == LOW);

    if (isPressed != prevButtonStates[i]) {
      buttonStates[i] = isPressed;
      prevButtonStates[i] = isPressed;

      Serial.print("Button ");
      Serial.print(i);
      Serial.println(isPressed ? " pressed" : " released");

      // Call function toggle handler
      toggleFunction(i, isPressed);
    }
  }
}

void regWrite(int pin, bool state) {
  int reg = (pin / 8) == 0 ? 1 : 0;
  int actualPin = pin % 8;
  
  digitalWrite(latchPin, LOW);
  for (int i = 0; i < numOfRegisters; i++) {
    byte* states = &registerState[i];
    if (i == reg) {
      bitWrite(*states, actualPin, state);
    }
    shiftOut(dataPin, clockPin, MSBFIRST, *states);
  }
  digitalWrite(latchPin, HIGH);
}

void updateActiveNotes() {
  activeNotesCount = 0;
  for (int i = 0; i < 12; i++) {
    if (buttonStates[i]) {
      activeNotes[activeNotesCount++] = noteMap[i];
    }
  }
}


// Modify EncoderIntoNotes to send random notes when function 3 is enabled (similar logic to cycleNotes)
void EncoderIntoNotes() {
  int encoderState = digitalRead(encoderPinA);

  if (encoderState != lastEncoderState && encoderState == LOW) {
    if (digitalRead(encoderPinB) == HIGH) {
      // Encoder turned right
      if (function3State) {
        // Random note selection when function 3 is active
        currentNoteIndex = random(0, activeNotesCount);  // Pick a random note
      } else {
        // Normal behavior
        currentNoteIndex++;
        if (currentNoteIndex >= activeNotesCount) {
          if (function1State) {  // Only shift octave if Function 1 is ON
            octaveShift++;
          }
          currentNoteIndex = 0;
        }
      }
    } else {
      // Encoder turned left
      if (function3State) {
        // Random note selection when function 3 is active
        currentNoteIndex = random(0, activeNotesCount);  // Pick a random note
      } else {
        // Normal behavior
        currentNoteIndex--;
        if (currentNoteIndex < 0) {
          if (function1State) {
            octaveShift--;
          }
          currentNoteIndex = activeNotesCount - 1;
        }
      }
    }
    sendMidiNote();  // Send the selected note
  }

  lastEncoderState = encoderState;
}


// Function to save a preset to EEPROM
void savePreset(int presetAddress) {
  // Flash the preset button LED while saving
  regWrite(PRESET1_BUTTON, LOW); // Turn LED off
  delay(200);                    // Small delay for feedback
  regWrite(PRESET1_BUTTON, HIGH); // Turn LED on

  for (int i = 0; i < NOTES_COUNT; i++) {
    EEPROM.update(presetAddress + i, buttonStates[i]);
  }
  delay(1000); // LED stays on for a second to indicate save
  regWrite(PRESET1_BUTTON, LOW); // Turn LED off after saving
}

// Function to load a preset from EEPROM
void loadPreset(int presetAddress) {
  bool hasData = false;
  for (int i = 0; i < NOTES_COUNT; i++) {
    byte savedState = EEPROM.read(presetAddress + i);
    if (savedState != 0xFF) hasData = true; // Check for valid data
    buttonStates[i] = savedState;
  }

  if (hasData) {
    // Flash the preset button LED on successful load
    for (int i = 0; i < 3; i++) {
      regWrite(PRESET1_BUTTON, HIGH);
      delay(100);
      regWrite(PRESET1_BUTTON, LOW);
      delay(100);
    }
  }
}


// In the loop function, handle immediate save upon long press
void loop() {
  readMuxButtons();

  // Check if preset button is pressed long enough to save
  if (presetButtonPressed && (millis() - presetButtonPressTime) >= 2000) {
    savePreset(PRESET1_ADDRESS);
    presetButtonPressed = false; // Prevent re-triggering
  }

  // Handle LEDs, active notes, and note cycling as usual
  for (int i = 0; i < 16; i++) {
    if (ledFlashing[i] && millis() - ledFlashTimers[i] >= flashDuration) {
      ledFlashing[i] = false;
      regWrite(i, buttonStates[i] ? HIGH : LOW);
    } else if (!ledFlashing[i]) {
      regWrite(i, buttonStates[i] ? HIGH : LOW);
    }
  }
  updateActiveNotes();
  if (function2State) {
    cycleNotes();
    adjustBPM();
  } else {
    EncoderIntoNotes();
  }
}


void sendMidiNote() {
    int note = activeNotes[currentNoteIndex];

    // Un-Comment this to have the octave shift only work with the function 1 knob
    //if (function1State) {
    //    note += octaveShift * 12;  // Adjust note based on octave shift if Function 1 is ON
    //}
    note += octaveShift * 12;
    // Calculate LED index for the note and check if it's within valid range
    int ledIndex = note % 12;
    if (ledIndex < 0 || ledIndex >= 12) {
        return; // Avoid invalid LED index
    }

    // Send MIDI Note On
    midiEventPacket_t noteOn = {0x09, 0x90, note, 127}; // 127 = full velocity
    MidiUSB.sendMIDI(noteOn);
    MidiUSB.flush();

    // Flash the LED off briefly if it's not already flashing
    if (!ledFlashing[ledIndex]) {
        regWrite(ledIndex, LOW);   // Turn LED off
        ledFlashTimers[ledIndex] = millis();  // Set flash start time
        ledFlashing[ledIndex] = true;         // Mark the LED as flashing
    }

    delay(100);


    // Send MIDI Note Off
    midiEventPacket_t noteOff = {0x08, 0x80, note, 0};
    MidiUSB.sendMIDI(noteOff);
    MidiUSB.flush();
}



// Cycle notes at the set BPM
// Modify cycleNotes function to incorporate random note selection if function 3 is ON
void cycleNotes() {
  cycleInterval = 60000 / bpm;

  if (millis() - lastNoteChangeTime >= cycleInterval) {
    if (activeNotesCount > 0) {
      if (function3State) {
        // Function 3: Select a random note from active notes
        int randomIndex = random(0, activeNotesCount);  // Get a random index
        currentNoteIndex = randomIndex;  // Use that index
      } else {
        // Function 2 (arpeggio) or normal mode
        currentNoteIndex = (currentNoteIndex + 1) % activeNotesCount;
      }
      sendMidiNote();  // Send the current note
    }
    lastNoteChangeTime = millis();
  }
}


// Adjust the BPM using the encoder
void adjustBPM() {
  int encoderState = digitalRead(encoderPinA);
  if (encoderState != lastEncoderState && encoderState == LOW) {
    if (digitalRead(encoderPinB) == HIGH) {
      bpm += 5;
    } else {
      bpm -= 5;
    }
    if (bpm < min_bpm) bpm = min_bpm; // Minimum BPM
    if (bpm > max_bpm) bpm = max_bpm; // Maximum BPM
    Serial.print("BPM");
    Serial.println(bpm);
  }
  lastEncoderState = encoderState;
}
