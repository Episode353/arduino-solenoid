#include <MIDIUSB.h>
#include <EEPROM.h>

// Define the flash duration (in milliseconds)
const unsigned long flashDuration = 30;

// Arrays to track LED flashing
unsigned long ledFlashTimers[32];
bool ledFlashing[32] = {false}; // Track whether each LED is in flash-off state
int ledFlashCounts[32];         // Number of flashes remaining for each LED
bool ledCurrentState[32];       // Current state of each LED

// Constants for preset handling
#define PRESET1_BUTTON 31
#define PRESET1_ADDRESS 0 // Start address in EEPROM for preset1
#define NOTES_COUNT 12    // Only saving buttons 1-12

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

int currentNoteIndex = 0; // Index to keep track of current note position in the active notes list
int octaveShift = 0;      // Variable to track the current octave shift
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
bool arp_state = false;           // false = OFF (no arpeggio), true = ON (arpeggio)
bool randomizer_state = false;    // false = OFF, true = ON (random note)

#define FIRST_PRESET_BUTTON 20
#define LAST_PRESET_BUTTON 31
#define EEPROM_PRESET_BASE_ADDRESS 0 // Base address in EEPROM for presets
#define NOTES_COUNT 12               // Only saving buttons 1-12

// Adjust for multiple presets in RAM
bool presets[LAST_PRESET_BUTTON - FIRST_PRESET_BUTTON + 1][NOTES_COUNT];

unsigned long lastNoteChangeTime = 0; // Timer for note cycling
int bpm = 120;                        // Default BPM
int max_bpm = 1000;                   // Max BPM
int min_bpm = 30;                     // Minimum BPM
int cycleInterval;                    // Interval for note cycling in milliseconds

// Function toggle handler
void toggleFunction(int buttonIndex, bool isPressed) {
  // Toggle function for Octave Shift (button 14)
  if (buttonIndex == 14 && isPressed) { // Detect press event only
    octave_shift_State = !octave_shift_State; // Toggle state
    Serial.print("Function 1 (Octave Shift) ");
    Serial.println(octave_shift_State ? "ON" : "OFF");
    buttonStates[buttonIndex] = octave_shift_State; // Sync button state
    regWrite(buttonIndex, octave_shift_State ? HIGH : LOW); // Update LED
  }

  // Toggle function for Arpeggio (button 15)
  if (buttonIndex == 15 && isPressed) { // Detect press event only
    arp_state = !arp_state; // Toggle state
    Serial.print("Function 2 (Arpeggio) ");
    Serial.println(arp_state ? "ON" : "OFF");
    buttonStates[buttonIndex] = arp_state; // Sync button state
    regWrite(buttonIndex, arp_state ? HIGH : LOW); // Update LED
  }

  // Toggle function for Randomizer (button 16)
  if (buttonIndex == 16 && isPressed) { // Detect press event only
    randomizer_state = !randomizer_state; // Toggle state
    Serial.print("Function 3 (Randomizer) ");
    Serial.println(randomizer_state ? "ON" : "OFF");
    buttonStates[buttonIndex] = randomizer_state; // Sync button state
    regWrite(buttonIndex, randomizer_state ? HIGH : LOW); // Update LED
  }

  // Handle preset buttons (20–31)
  if (buttonIndex >= FIRST_PRESET_BUTTON && buttonIndex <= LAST_PRESET_BUTTON) {
    handlePresetButton(buttonIndex, isPressed);
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

  // Initialize LED current states
  for (int i = 0; i < 32; i++) {
    ledCurrentState[i] = buttonStates[i];
  }
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

// Handle flashing for the octave up and down buttons
void handleOctaveButtonFlash(int buttonIndex) {
  ledFlashCounts[buttonIndex] = 3;        // 3 flashes (6 transitions)
  ledFlashTimers[buttonIndex] = millis(); // Set flash start time
  ledFlashing[buttonIndex] = true;        // Mark the LED as flashing
  ledCurrentState[buttonIndex] = false;   // Start with LED off
  regWrite(buttonIndex, LOW);
}

void readMuxButtons() {
  const int octaveShiftMin = -5; // Minimum octave shift
  const int octaveShiftMax = 4;  // Maximum octave shift

  // Process buttons 0–15 from the first multiplexer
  for (int i = 0; i < 16; i++) {
    // Set multiplexer address
    digitalWrite(muxS0Pin, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin, (i & 8) ? HIGH : LOW);

    int buttonState = digitalRead(muxSigPin);
    bool isPressed = (buttonState == LOW);

    // Handle momentary octave shift for buttons 12 and 13
    if (i == 12 || i == 13) {
      if (isPressed && !prevButtonStates[i]) { // Detect press event only
        if (i == 12 && octaveShift < octaveShiftMax) {
          octaveShift++; // Increment octave shift
          Serial.print("Octave Shift: ");
          Serial.println(octaveShift);
          handleOctaveButtonFlash(12);
        } else if (i == 13 && octaveShift > octaveShiftMin) {
          octaveShift--; // Decrement octave shift
          Serial.print("Octave Shift: ");
          Serial.println(octaveShift);
          handleOctaveButtonFlash(13);
        }
      }
    }

    // Process toggle buttons (0–11)
    if (i >= 0 && i <= 11) {
      if (isPressed && !prevButtonStates[i]) { // Press event
        buttonStates[i] = !buttonStates[i];    // Toggle state
        Serial.print("Toggle Button ");
        Serial.print(i);
        Serial.println(buttonStates[i] ? " ON" : " OFF");
        toggleFunction(i, buttonStates[i]);    // Call toggle function
      }
    }
    // Process toggle function buttons (14–16)
    else if (i >= 14 && i <= 16) {
      if (isPressed && !prevButtonStates[i]) { // Detect press event only
        toggleFunction(i, true);              // Call toggle function on press
      }
    }

    // Update previous state
    prevButtonStates[i] = isPressed;
  }

  // Process buttons 16–31 from the second multiplexer
  for (int i = 0; i < 16; i++) {
    // Set multiplexer address for the second multiplexer
    digitalWrite(muxS0Pin2, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin2, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin2, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin2, (i & 8) ? HIGH : LOW);

    int buttonState = digitalRead(muxSigPin2);
    bool isPressed = (buttonState == LOW);

    int buttonIndex = i + 16;

    // Handle function buttons (button indices 16–19)
    if (buttonIndex >= 16 && buttonIndex <= 19) {
      if (isPressed && !prevButtonStates[buttonIndex]) { // Detect press event only
        toggleFunction(buttonIndex, true);              // Call toggle function on press
      }
    }

    // Handle preset buttons (20–31)
    else if (buttonIndex >= FIRST_PRESET_BUTTON && buttonIndex <= LAST_PRESET_BUTTON) {
      if (isPressed != prevButtonStates[buttonIndex]) { // Detect state change
        toggleFunction(buttonIndex, isPressed);         // Call toggle function
      }
    }

    // Update previous state
    prevButtonStates[buttonIndex] = isPressed;
  }
}



void regWrite(int pin, bool state) {
  int reg = pin / 8;    // Determine which register the pin belongs to
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

void handlePresetButton(int buttonIndex, bool isPressed) {
  static unsigned long presetPressTimes[LAST_PRESET_BUTTON - FIRST_PRESET_BUTTON + 1] = {0};
  int presetIndex = buttonIndex - FIRST_PRESET_BUTTON;

  if (isPressed) {
    if (presetPressTimes[presetIndex] == 0) {
      presetPressTimes[presetIndex] = millis(); // Start the timer for the button
    }
  } else {
    if (presetPressTimes[presetIndex] != 0) {
      unsigned long pressDuration = millis() - presetPressTimes[presetIndex];
      presetPressTimes[presetIndex] = 0; // Reset the timer

      if (pressDuration >= 1000) {
        savePreset(presetIndex); // Save preset if held for >= 1 seconds
      } else {
        loadPreset(presetIndex); // Load preset if held for < 1 seconds
      }
    }
  }
}




void savePreset(int presetIndex) {
  // Save the buttonStates to the preset in RAM and EEPROM
  for (int i = 0; i < NOTES_COUNT; i++) {
    presets[presetIndex][i] = buttonStates[i];
    EEPROM.write(EEPROM_PRESET_BASE_ADDRESS + presetIndex * NOTES_COUNT + i, buttonStates[i]);
  }

  // Flash the corresponding preset button LED on successful save
  int buttonIndex = FIRST_PRESET_BUTTON + presetIndex;

  // Start non-blocking LED flashing
  ledFlashCounts[buttonIndex] = 12;        // 6 transitions (3 flashes)
  ledFlashTimers[buttonIndex] = millis(); // Set flash start time
  ledFlashing[buttonIndex] = true;        // Mark the LED as flashing
  ledCurrentState[buttonIndex] = false;   // Start with LED off
  regWrite(buttonIndex, ledCurrentState[buttonIndex] ? HIGH : LOW);

  Serial.print("Saved preset ");
  Serial.println(presetIndex);
}


void loadPreset(int presetIndex) {
  bool hasData = false;

  // Load the preset data from EEPROM into RAM and buttonStates
  for (int i = 0; i < NOTES_COUNT; i++) {
    presets[presetIndex][i] = EEPROM.read(EEPROM_PRESET_BASE_ADDRESS + presetIndex * NOTES_COUNT + i);
    buttonStates[i] = presets[presetIndex][i];
    if (presets[presetIndex][i] != 0) hasData = true; // Check for valid data
  }

  if (hasData) {
    // Flash the corresponding preset button LED on successful load
    int buttonIndex = FIRST_PRESET_BUTTON + presetIndex;

    // Start non-blocking LED flashing
    ledFlashCounts[buttonIndex] = 2;        // 6 transitions (3 flashes)
    ledFlashTimers[buttonIndex] = millis(); // Set flash start time
    ledFlashing[buttonIndex] = true;        // Mark the LED as flashing
    ledCurrentState[buttonIndex] = false;   // Start with LED off
    regWrite(buttonIndex, ledCurrentState[buttonIndex] ? HIGH : LOW);
  }

  Serial.print("Loaded preset ");
  Serial.println(presetIndex);
}

void loop() {
  readMuxButtons();

  // Ensure LEDs for buttons 14, 15, and 16 stay on/off based on their states
  regWrite(14, octave_shift_State ? HIGH : LOW);
  regWrite(15, arp_state ? HIGH : LOW);
  regWrite(16, randomizer_state ? HIGH : LOW);

  // Handle LEDs, active notes, and note cycling as usual
  for (int i = 0; i < 32; i++) {
    if (ledFlashing[i]) {
      if (millis() - ledFlashTimers[i] >= flashDuration) {
        ledFlashTimers[i] = millis();
        ledCurrentState[i] = !ledCurrentState[i]; // Toggle LED state
        regWrite(i, ledCurrentState[i] ? HIGH : LOW);
        ledFlashCounts[i]--;
        if (ledFlashCounts[i] <= 0) {
          ledFlashing[i] = false;
          ledCurrentState[i] = buttonStates[i]; // Return to normal state
          regWrite(i, ledCurrentState[i] ? HIGH : LOW);
        }
      }
    } else {
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
    ledFlashCounts[ledIndex] = 2;         // Flash once (2 transitions)
    ledCurrentState[ledIndex] = false;    // Start with LED off
  }



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