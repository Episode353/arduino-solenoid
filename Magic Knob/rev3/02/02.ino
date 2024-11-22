#include <MIDIUSB.h>

// Define the flash duration (in milliseconds)
const unsigned long flashDuration = 30;

// Array to track the last flash time for each button's LED
unsigned long ledFlashTimers[32];
bool ledFlashing[32] = {false}; // Track whether each LED is in flash-off state

#include <EEPROM.h>

// Constants for preset handling
#define PRESET1_BUTTON 31
#define PRESET1_ADDRESS 0 // Start address in EEPROM for preset1
#define NOTES_COUNT 12 // Only saving buttons 1-12

// Timing and state tracking for the preset button
unsigned long presetButtonPressTime = 0;
bool presetButtonPressed = false;
// Pin definitions (unchanged)
#define clockPin 3
#define dataPin 5
#define latchPin 4
#define muxSigPin A3
#define muxS0Pin 6
#define muxS1Pin 7
#define muxS2Pin 8
#define muxS3Pin 9

#define muxSigPin2 A2
#define muxS0Pin2 19
#define muxS1Pin2 18
#define muxS2Pin2 15
#define muxS3Pin2 14


int numOfRegisters = 4;
byte* registerState;
bool buttonStates[32];
bool prevButtonStates[32];

int currentNoteIndex = 0;   // Index to keep track of current note position in the active notes list
int octaveShift = 0;        // Variable to track the current octave shift
const uint8_t noteMap[12] = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}; // MIDI notes C4-B4
int activeNotes[12];
int activeNotesCount = 0;

// Store presets in RAM
bool preset1[NOTES_COUNT];


const int encoderPinA = 10;
const int encoderPinB = 16;
int lastEncoderState = HIGH;

// Function states
bool octave_shift_State = false;  // false = OFF (no octave shifting), true = ON (octave shifting)
bool arp_state = false;  // false = OFF (no arpeggio), true = ON (arpeggio)
bool randomizer_state = false;  // false = OFF, true = ON (random note)

unsigned long lastNoteChangeTime = 0;  // Timer for note cycling
int bpm = 120;  // Default BPM
int max_bpm = 1000;  // Max BPM
int min_bpm = 30;  // Minimum BPM
int cycleInterval;  // Interval for note cycling in milliseconds

// Function toggle handler
void toggleFunction(int buttonIndex, bool isPressed) {
  // Toggle function 1 when button 12 is pressed or released
  if (buttonIndex == 14) {
    if (isPressed || !isPressed) {  // Trigger on both press and release
      octave_shift_State = !octave_shift_State;  // Toggle the state
      Serial.print("Function 1 ");
      Serial.println(octave_shift_State ? "ON" : "OFF");
    }
  }

  if (buttonIndex == 15) {
    if (isPressed || !isPressed) {  // Trigger on both press and release
      arp_state = !arp_state;  // Toggle the state
      Serial.print("Function 2 ");
      Serial.println(arp_state ? "ON" : "OFF");
    }
  }

  if (buttonIndex == 16) {  // Toggle function 3 (Random note)
    if (isPressed || !isPressed) {  
      randomizer_state = !randomizer_state;
      Serial.print("Function 3 ");
      Serial.println(randomizer_state ? "ON" : "OFF");
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

void loadPresetFromEEPROM(int presetIndex) {
  // Check if preset data exists in EEPROM and load it
  for (int i = 0; i < NOTES_COUNT; i++) {
    preset1[i] = EEPROM.read(presetIndex + i);  // Read byte by byte from EEPROM
  }

  // Flash the preset button LED on successful load
  for (int i = 0; i < 3; i++) {
    regWrite(PRESET1_BUTTON, HIGH);  // Turn LED on
    regWrite(PRESET1_BUTTON, LOW);   // Turn LED off
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

  pinMode(muxS0Pin2, OUTPUT);
  pinMode(muxS1Pin2, OUTPUT);
  pinMode(muxS2Pin2, OUTPUT);
  pinMode(muxS3Pin2, OUTPUT);

  pinMode(muxSigPin, INPUT_PULLUP);
  pinMode(muxSigPin2, INPUT_PULLUP);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  Serial.begin(9600);
  clearShiftRegister();

  loadPresetFromEEPROM(PRESET1_ADDRESS);
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
    // Read from the first multiplexer
    digitalWrite(muxS0Pin, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin, (i & 8) ? HIGH : LOW);

    int buttonState = digitalRead(muxSigPin);
    bool isPressed = (buttonState == LOW);

    // Handle toggle buttons (0-11 and 14-19)
    if ((i >= 0 && i <= 11) || (i >= 14 && i <= 19)) {
      if (isPressed && !prevButtonStates[i]) { // Detect press event
        buttonStates[i] = !buttonStates[i];    // Toggle state
        Serial.print("Toggle Button ");
        Serial.print(i);
        Serial.println(buttonStates[i] ? " ON" : " OFF");
        toggleFunction(i, buttonStates[i]);    // Call function with new state
      }
    }
    // Handle momentary buttons (12 and 13)
    else if (i == 12 || i == 13) {
      if (isPressed != prevButtonStates[i]) { // Detect state change
        buttonStates[i] = isPressed;          // Set state to current press
        Serial.print("Momentary Button ");
        Serial.print(i);
        Serial.println(isPressed ? " pressed" : " released");
        toggleFunction(i, isPressed);         // Call function with current state
      }
    }
    // Handle other buttons as needed
    else {
      // Existing behavior for other buttons
    }

    // Update previous state
    prevButtonStates[i] = isPressed;

    // Read from the second multiplexer
    digitalWrite(muxS0Pin2, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin2, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin2, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin2, (i & 8) ? HIGH : LOW);

    buttonState = digitalRead(muxSigPin2);
    isPressed = (buttonState == LOW);

    int buttonIndex = i + 16;

    // Handle toggle buttons (16-27 and 30-35)
    if ((buttonIndex >= 0 && buttonIndex <= 11) || (buttonIndex >= 14 && buttonIndex <= 19)) {
      if (isPressed && !prevButtonStates[buttonIndex]) { // Detect press event
        buttonStates[buttonIndex] = !buttonStates[buttonIndex]; // Toggle state
        Serial.print("Toggle Button ");
        Serial.print(buttonIndex);
        Serial.println(buttonStates[buttonIndex] ? " ON" : " OFF");
        toggleFunction(buttonIndex, buttonStates[buttonIndex]); // Call function with new state
      }
    }
    // Handle momentary buttons (28 and 29)
    else if (buttonIndex == 12 || buttonIndex == 13) {
      if (isPressed != prevButtonStates[buttonIndex]) { // Detect state change
        buttonStates[buttonIndex] = isPressed;          // Set state to current press
        Serial.print("Momentary Button ");
        Serial.print(buttonIndex);
        Serial.println(isPressed ? " pressed" : " released");
        toggleFunction(buttonIndex, isPressed);         // Call function with current state
      }
    }
    // Handle other buttons as needed
    else {
      // Existing behavior for other buttons
    }

    // Update previous state
    prevButtonStates[buttonIndex] = isPressed;
  }
}







void regWrite(int pin, bool state) {
    int reg = pin / 8; // Determine which register the pin belongs to
    int actualPin = pin % 8; // Determine the pin within the register

    digitalWrite(latchPin, LOW);

    // Update the target register's state
    bitWrite(registerState[reg], actualPin, state);

    // Write all registers in sequence
    for (int i = numOfRegisters - 1; i >= 0; i--) {
        shiftOut(dataPin, clockPin, MSBFIRST, registerState[i]);
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
      if (randomizer_state) {
        // Random note selection when function 3 is active
        currentNoteIndex = random(0, activeNotesCount);  // Pick a random note
      } else {
        // Normal behavior
        currentNoteIndex++;
        if (currentNoteIndex >= activeNotesCount) {
          if (octave_shift_State) {  // Only shift octave if Function 1 is ON
            octaveShift++;
          }
          currentNoteIndex = 0;
        }
      }
    } else {
      // Encoder turned left
      if (randomizer_state) {
        // Random note selection when function 3 is active
        currentNoteIndex = random(0, activeNotesCount);  // Pick a random note
      } else {
        // Normal behavior
        currentNoteIndex--;
        if (currentNoteIndex < 0) {
          if (octave_shift_State) {
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
// Function to save a preset to RAM
void savePreset(int presetIndex) {
  // Flash the preset button LED while saving
  regWrite(PRESET1_BUTTON, LOW); // Turn LED off
  regWrite(PRESET1_BUTTON, HIGH); // Turn LED on

  // Save the buttonStates to preset1 in RAM
  for (int i = 0; i < NOTES_COUNT; i++) {
    preset1[i] = buttonStates[i];
    EEPROM.write(PRESET1_ADDRESS + i, preset1[i]); // Save to EEPROM
  }

  regWrite(PRESET1_BUTTON, LOW); // Turn LED off after saving
}



// Function to load a preset from EEPROM
// Function to load a preset from RAM
void loadPreset(int presetIndex) {
  bool hasData = false;
  
  // Load the preset data from RAM into buttonStates
  for (int i = 0; i < NOTES_COUNT; i++) {
    buttonStates[i] = preset1[i];
    if (preset1[i] != 0) hasData = true; // Check for valid data
  }

  if (hasData) {
    // Flash the preset button LED on successful load
    for (int i = 0; i < 3; i++) {
      regWrite(PRESET1_BUTTON, HIGH);
      regWrite(PRESET1_BUTTON, LOW);
    }
  }
}



// In the loop function, handle immediate save upon long press
void loop() {
  readMuxButtons();

  // Check if preset button is pressed long enough to save
  if (presetButtonPressed && (millis() - presetButtonPressTime) >= 2000) {
    savePreset(PRESET1_ADDRESS);  // Save to RAM instead of EEPROM
    presetButtonPressed = false;  // Prevent re-triggering
  }

  // Handle LEDs, active notes, and note cycling as usual
  for (int i = 0; i < 32; i++) {
    if (ledFlashing[i] && millis() - ledFlashTimers[i] >= flashDuration) {
      ledFlashing[i] = false;
      regWrite(i, buttonStates[i] ? HIGH : LOW);
    } else if (!ledFlashing[i]) {
      regWrite(i, buttonStates[i] ? HIGH : LOW);
    }
  }
  updateActiveNotes();
  if (arp_state) {
    cycleNotes();
    adjustBPM();
  } else {
    EncoderIntoNotes();
  }
}



void sendMidiNote() {
    int note = activeNotes[currentNoteIndex];

    // Un-Comment this to have the octave shift only work with the function 1 knob
    //if (octave_shift_State) {
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
      if (randomizer_state) {
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