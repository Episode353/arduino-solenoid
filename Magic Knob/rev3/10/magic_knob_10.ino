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

// Encoder pins and variables
const int encoderPinA = 10;
const int encoderPinB = 16;
volatile int encoderPos = 0;      // Current encoder position
int lastEncoderPos = 0;           // Last processed encoder position
unsigned long lastEncoderTime = 0; // For debouncing
int encoderState = 0;             // State machine for the encoder

// Function states
bool octave_shift_State = false;  // false = OFF (no octave shifting), true = ON (octave shifting)
bool arp_state = false;           // false = OFF (no arpeggio), true = ON (arpeggio)
bool randomizer_state = false;    // false = OFF, true = ON (random note)

// New variables for arpeggiator octave range
int arpOctaveRange = 0;           // How many octaves up the arpeggiator will go
int currentArpOctave = 0;         // Current octave in the arpeggiator cycle
int currentOctavePass = 0;        // Tracks which pass through the octave we're on

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

// Timing variables for optimized loop
unsigned long lastMuxReadTime = 0;
unsigned long lastLEDUpdateTime = 0;
unsigned long lastNoteUpdateTime = 0;
const unsigned long MUX_READ_INTERVAL = 10;     // Check buttons every 10ms
const unsigned long LED_UPDATE_INTERVAL = 5;    // Update LEDs every 5ms
const unsigned long ENCODER_CHECK_INTERVAL = 1; // Check encoder more frequently


// Inertia mode variables
const float maxInertia = 0.55;  // Adjust this value to change the maximum inertia
bool inertia_mode = false;           // false = OFF, true = ON
float knobInertia = 0.0;             // Current inertia value
const float inertiaDecayRate = 0.001; // Rate at which inertia decays decay per update
const float minInertiaThreshold = 0.0001; // Minimum inertia to continue movement
unsigned long lastInertiaUpdateTime = 0;
const unsigned long INERTIA_UPDATE_INTERVAL = 15; // Update inertia effect every 20ms
const float inertiaPerDetent = 0.0075; // Adjust this value to change how much inertia is added per click

struct NoteOffEvent {
  bool active;           // Whether this event is active and waiting to be processed
  unsigned long dueTime; // When to process this event (in milliseconds)
  uint8_t noteNumber;    // Which MIDI note to turn off
};


// Queue for pending note-off events (adjust size based on expected polyphony)
#define MAX_NOTE_EVENTS 10
NoteOffEvent noteOffQueue[MAX_NOTE_EVENTS];

// Initialize the noteOffQueue in setup()
void initializeNoteOffQueue() {
  for (int i = 0; i < MAX_NOTE_EVENTS; i++) {
    noteOffQueue[i].active = false;
  }
}


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
    
    // Reset arpeggiator variables when toggling
    currentArpOctave = 0;
    currentOctavePass = 0;
  }

  // Toggle function for Randomizer (button 16)
  if (buttonIndex == 16 && isPressed) { // Detect press event only
    randomizer_state = !randomizer_state; // Toggle state
    Serial.print("Function 3 (Randomizer) ");
    Serial.println(randomizer_state ? "ON" : "OFF");
    buttonStates[buttonIndex] = randomizer_state; // Sync button state
    regWrite(buttonIndex, randomizer_state ? HIGH : LOW); // Update LED
  }

  if (buttonIndex == 17 && isPressed) { // Detect press event only
  inertia_mode = !inertia_mode; // Toggle state
  Serial.print("Function 4 (Inertia Mode) ");
  Serial.println(inertia_mode ? "ON" : "OFF");
  buttonStates[buttonIndex] = inertia_mode; // Sync button state
  regWrite(buttonIndex, inertia_mode ? HIGH : LOW); // Update LED
  
  // Reset inertia when toggling the mode
  knobInertia = 0.0;
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
  
  // Set maximum octave range for arpeggiator
  const int maxArpOctaveRange = 4;  // Maximum octave range for arpeggiator

  // Process buttons 0–15 from the first multiplexer
  for (int i = 0; i < 16; i++) {
    // Set multiplexer address
    digitalWrite(muxS0Pin, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin, (i & 8) ? HIGH : LOW);

    int buttonState = digitalRead(muxSigPin);
    bool isPressed = (buttonState == LOW);

    // Handle octave buttons with modified behavior for arpeggiator mode
    if (i == 12 || i == 13) {
      if (isPressed && !prevButtonStates[i]) { // Detect press event only
        if (arp_state) {
          // In arpeggiator mode, buttons change octave range
          if (i == 12 && arpOctaveRange < maxArpOctaveRange) {
            arpOctaveRange++; // Increase number of octaves to loop through
            Serial.print("Arp Octave Range: ");
            Serial.println(arpOctaveRange);
            handleOctaveButtonFlash(12);
          } else if (i == 13 && arpOctaveRange > 0) {
            arpOctaveRange--; // Decrease number of octaves to loop through
            Serial.print("Arp Octave Range: ");
            Serial.println(arpOctaveRange);
            handleOctaveButtonFlash(13);
          }
        } else {
          // Regular octave shifting in non-arp mode
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

// New improved encoder reading function using state machine approach with debouncing
void readEncoder() {
  // Read the encoder pins
  bool pinA = digitalRead(encoderPinA);
  bool pinB = digitalRead(encoderPinB);
  
  // Combine readings into a single state
  byte currentState = (pinA << 1) | pinB;
  
  // Only process state change if it's different
  if (currentState != encoderState) {
    // State transition table for clockwise rotation: 0-2-3-1-0
    // State transition table for counter-clockwise rotation: 0-1-3-2-0
    
    // Simple state machine approach
    switch (encoderState) {
      case 0: // Previous state was 00
        if (currentState == 1) encoderPos--; // Counter-clockwise
        if (currentState == 2) encoderPos++; // Clockwise
        break;
      case 1: // Previous state was 01
        if (currentState == 3) encoderPos--; // Counter-clockwise
        if (currentState == 0) encoderPos++; // Clockwise
        break;
      case 2: // Previous state was 10
        if (currentState == 0) encoderPos--; // Counter-clockwise
        if (currentState == 3) encoderPos++; // Clockwise
        break;
      case 3: // Previous state was 11
        if (currentState == 2) encoderPos--; // Counter-clockwise
        if (currentState == 1) encoderPos++; // Clockwise
        break;
    }
    
    // Update state
    encoderState = currentState;
  }
}

// Modify your processEncoder function to handle inertia mode
void processEncoder() {
  // Only process if there's been a change
  if (encoderPos != lastEncoderPos) {
    int change = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;
    
    // Normalize change based on the detent - this encoder generates 4 steps per click
    // Only process when we've accumulated enough steps for one full detent
    static int accumulatedSteps = 0;
    accumulatedSteps += change;
    
    // Process only when we reach a threshold of steps (approximately one detent)
    if (abs(accumulatedSteps) >= 4) {
      // Calculate how many full detents we've turned
      int detents = accumulatedSteps / 4;
      accumulatedSteps = accumulatedSteps % 4; // Keep remainder for next time
      
      // Depending on mode
      if (arp_state) {
        // Adjust BPM in arpeggio mode
        bpm += detents * 5;
        if (bpm < min_bpm) bpm = min_bpm;
        if (bpm > max_bpm) bpm = max_bpm;
        Serial.print("BPM: ");
        Serial.println(bpm);
      } else if (inertia_mode) {
        // In inertia mode, turning adds to the inertia value
        knobInertia += detents * inertiaPerDetent;
        

        // Cap maximum inertia at a reasonable value to prevent excessive speeds
        knobInertia = constrain(knobInertia, -maxInertia, maxInertia);
        
        
        Serial.print("Inertia: ");
        Serial.println(knobInertia);
      } else {
        // Handle note selection in normal mode
        if (activeNotesCount > 0) {
          if (randomizer_state) {
            // Random note selection when function 3 is active
            currentNoteIndex = random(0, activeNotesCount);
          } else {
            // Update note index based on direction
            currentNoteIndex += detents;
            
            // Handle wrapping and octave shift
            if (currentNoteIndex >= activeNotesCount) {
              if (octave_shift_State) {
                octaveShift++;
              }
              currentNoteIndex = 0;
            } else if (currentNoteIndex < 0) {
              if (octave_shift_State) {
                octaveShift--;
              }
              currentNoteIndex = activeNotesCount - 1;
            }
          }
          
          // Send the MIDI note
          sendMidiNote();
        }
      }
    }
  }
}

// Modify the processInertia function to handle decay differently based on sign
void processInertia() {
  unsigned long currentTime = millis();
  
  // Only update at specified interval
  if (currentTime - lastInertiaUpdateTime >= INERTIA_UPDATE_INTERVAL) {
    lastInertiaUpdateTime = currentTime;
    
    // Only process if inertia mode is active and there's significant inertia
    if (inertia_mode && abs(knobInertia) > minInertiaThreshold) {
      // Apply decay only to positive inertia values
      if (knobInertia > 0) {
        // Apply decay to the inertia value
        knobInertia -= inertiaDecayRate;
        
        // If inertia is below threshold after decay, stop movement
        if (knobInertia <= minInertiaThreshold) {
          knobInertia = 0.0;
          return;
        }
      }
      // For negative values, we keep them (no decay)
      // This allows the user to set negative inertia that stays consistent
      
      // Calculate how many notes to move based on inertia
      // We accumulate the fractional movement until it's enough to move a note
      static float accumulatedMovement = 0.0;
      accumulatedMovement += knobInertia;
      
      // When accumulated movement is >= 1.0 or <= -1.0, move notes
      if (abs(accumulatedMovement) >= 1.0) {
        int notesToMove = (int)accumulatedMovement; // Get whole number of notes to move
        accumulatedMovement -= notesToMove; // Keep the remainder
        
        if (activeNotesCount > 0) {
          // Update note index based on inertia direction
          currentNoteIndex += notesToMove;
          
          // Handle wrapping
          while (currentNoteIndex >= activeNotesCount) {
            currentNoteIndex -= activeNotesCount;
            if (octave_shift_State) octaveShift++;
          }
          
          while (currentNoteIndex < 0) {
            currentNoteIndex += activeNotesCount;
            if (octave_shift_State) octaveShift--;
          }
          
          // Send the MIDI note
          sendMidiNote();
        }
      }
    }
  }
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

// Update LEDs based on their current states and flashing status
void updateLEDs() {
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
      // Only update if state has changed since last time
      if (i == 14 && ledCurrentState[i] != octave_shift_State) {
        ledCurrentState[i] = octave_shift_State;
        regWrite(i, ledCurrentState[i] ? HIGH : LOW);
      } else if (i == 15 && ledCurrentState[i] != arp_state) {
        ledCurrentState[i] = arp_state;
        regWrite(i, ledCurrentState[i] ? HIGH : LOW);
      } else if (i == 16 && ledCurrentState[i] != randomizer_state) {
        ledCurrentState[i] = randomizer_state;
        regWrite(i, ledCurrentState[i] ? HIGH : LOW);
      } else if (i < 14 || i > 16) {
        if (ledCurrentState[i] != buttonStates[i]) {
          ledCurrentState[i] = buttonStates[i];
          regWrite(i, ledCurrentState[i] ? HIGH : LOW);
        }
      }
    }
  }
}

// Replace your sendMidiNote() function with this one:
void sendMidiNote() {
  if (activeNotesCount == 0) return;
  
  // Make sure currentNoteIndex is valid
  currentNoteIndex = constrain(currentNoteIndex, 0, activeNotesCount - 1);
  
  int note = activeNotes[currentNoteIndex];

  // Apply octave shift (base octave + current arpeggiator octave if in arp mode)
  if (arp_state) {
    note += (octaveShift + currentArpOctave) * 12;
  } else {
    note += octaveShift * 12;
  }
  
  // Calculate LED index for the note and check if it's within valid range
  int ledIndex = note % 12;
  if (ledIndex < 0 || ledIndex >= 12) {
    return; // Avoid invalid LED index
  }

  // Send MIDI Note On
  midiEventPacket_t noteOn = {0x09, 0x90, note, 127}; // 127 = full velocity
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();

  // Schedule Note Off to happen 45ms from now
  queueNoteOff(note, 45); // 45ms delay

  // Flash the LED off briefly if it's not already flashing
  if (!ledFlashing[ledIndex]) {
    ledFlashTimers[ledIndex] = millis();  // Set flash start time
    ledFlashing[ledIndex] = true;         // Mark the LED as flashing
    ledFlashCounts[ledIndex] = 2;         // Flash once (2 transitions)
    ledCurrentState[ledIndex] = false;    // Start with LED off
    regWrite(ledIndex, LOW);              // Turn LED off immediately
  }
}

// New function to queue a note-off event
void queueNoteOff(uint8_t noteNumber, unsigned long delayMs) {
  // Find an empty slot in the queue
  for (int i = 0; i < MAX_NOTE_EVENTS; i++) {
    if (!noteOffQueue[i].active) {
      // Found an empty slot, schedule the note-off
      noteOffQueue[i].active = true;
      noteOffQueue[i].dueTime = millis() + delayMs;
      noteOffQueue[i].noteNumber = noteNumber;
      return;
    }
  }
  // If we get here, the queue is full, so we'll handle it immediately
  // (this should be rare if MAX_NOTE_EVENTS is sized appropriately)
  midiEventPacket_t noteOff = {0x08, 0x80, noteNumber, 0};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}


// Add this function to process any pending note-off events
void processNoteOffQueue() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < MAX_NOTE_EVENTS; i++) {
    if (noteOffQueue[i].active && currentTime >= noteOffQueue[i].dueTime) {
      // Time to process this note-off event
      midiEventPacket_t noteOff = {0x08, 0x80, noteOffQueue[i].noteNumber, 0};
      MidiUSB.sendMIDI(noteOff);
      MidiUSB.flush();
      
      // Mark the slot as available
      noteOffQueue[i].active = false;
    }
  }
}

// Cycle notes at the set BPM with octave range feature
void cycleNotes() {
  cycleInterval = 60000 / bpm;

  if (millis() - lastNoteChangeTime >= cycleInterval) {
    if (activeNotesCount > 0) {
      // Track note progression for octave changing, regardless of randomization
      static int noteCounter = 0;
      noteCounter++;
      
      if (randomizer_state) {
        // Function 3: Select a random note from active notes
        currentNoteIndex = random(0, activeNotesCount);  // Get a random index
      } else {
        // Function 2 (arpeggio) mode without randomization
        currentNoteIndex++;
        // When we reach the end of the note sequence, wrap around
        if (currentNoteIndex >= activeNotesCount) {
          currentNoteIndex = 0;
        }
      }
      
      // Check if we've played through all notes in the current sequence
      // This happens regardless of whether we're in random mode or not
      if (noteCounter >= activeNotesCount) {
        noteCounter = 0; // Reset note counter
        
        // Move to next octave/pass
        currentOctavePass++;
        
        // Calculate which octave we're on based on the pass count
        if (arpOctaveRange == 0) {
          // If range is 0, just stay in the base octave
          currentArpOctave = 0;
        } else {
          // Otherwise, calculate current octave (0 to arpOctaveRange, then back to 0)
          currentArpOctave = currentOctavePass % (arpOctaveRange + 1);
          
          // If we've completed a full cycle through all octaves, reset the pass counter
          if (currentOctavePass > arpOctaveRange) {
            currentOctavePass = 0;
          }
        }
      }
      
      sendMidiNote();  // Send the current note
    }
    lastNoteChangeTime = millis();
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Always read the encoder - this is the highest priority
  readEncoder();
  
  // Process encoder with minimal delay - multiple times per loop
  if (currentTime - lastEncoderTime >= ENCODER_CHECK_INTERVAL) {
    processEncoder();
    lastEncoderTime = currentTime;
  }

  // Process inertia if the mode is active
  if (inertia_mode) {
    processInertia();
  }
  
  // Read buttons at a reasonable rate
  if (currentTime - lastMuxReadTime >= MUX_READ_INTERVAL) {
    readMuxButtons();
    updateActiveNotes();
    lastMuxReadTime = currentTime;
  }
  
  // Update LEDs with moderate frequency
  if (currentTime - lastLEDUpdateTime >= LED_UPDATE_INTERVAL) {
    updateLEDs();
    lastLEDUpdateTime = currentTime;
  }
  
  // Process arpeggiator (lowest priority)
  if (arp_state) {
    cycleNotes();
  }
  
  // Process any pending MIDI note-off events
  processNoteOffQueue();
}