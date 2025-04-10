#include <Arduino.h>
#include <MIDIUSB.h>

// ===================== Constants and Definitions =====================

struct Scale {
  const char* name;
  const int intervals[7];
};

const Scale scales[] = {
  {"Major", {0, 2, 4, 5, 7, 9, 11}},
  {"Minor", {0, 2, 3, 5, 7, 8, 10}},
  {"Dorian", {0, 2, 3, 5, 7, 9, 10}},
  {"Phrygian", {0, 1, 3, 5, 7, 8, 10}},
  {"Lydian", {0, 2, 4, 6, 7, 9, 11}},
  {"Mixolydian", {0, 2, 4, 5, 7, 9, 10}},
  {"Locrian", {0, 1, 3, 5, 6, 8, 10}},
  {"Harmonic Minor", {0, 2, 3, 5, 7, 8, 11}}
};
const int numScales = sizeof(scales) / sizeof(scales[0]);

// Chromatic scale
const char* chromaticScale[12] = {
  "C", "C#", "D", "D#", "E", "F",
  "F#", "G", "G#", "A", "A#", "B"
};

// Button Mapping
// Button 0-6: Chords I-VII (Momentary)
// Button 7-18: Notes C to B
// Button 19-26: Scales
// Button 27: Latch
// Button 28: Bass (not used)
// Button 29: Octave Up
// Button 30: Octave Down
const int NUM_BUTTONS = 31;

// Button indices
#define LATCH_BUTTON 27
#define OCTAVE_UP_PIN A5     // Analog pin 5 for octave up
#define OCTAVE_DOWN_PIN 13   // Digital pin 13 for octave down
#define STRUM_POT_PIN A4     // Analog pin 4 for strum speed potentiometer

// Multiplexer Pin Definitions
// MUX1
#define MUX1_SIG_PIN A3
#define MUX1_S0_PIN 2
#define MUX1_S1_PIN 3
#define MUX1_S2_PIN 4
#define MUX1_S3_PIN 5

// MUX2
#define MUX2_SIG_PIN A2
#define MUX2_S0_PIN 12
#define MUX2_S1_PIN 11
#define MUX2_S2_PIN 10
#define MUX2_S3_PIN 9

// Shift Register Pin Definitions
#define SR_LATCH_PIN 8
#define SR_CLOCK_PIN 7
#define SR_DATA_PIN 6

// Numerals for Scales
const char* numerals[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};

// State Arrays
bool buttonStates[NUM_BUTTONS] = { false };
bool prevButtonStates[NUM_BUTTONS] = { false };
bool currentlyHeldButtons[7] = { false }; // Track which numeral buttons are held

// Current Scale Information
int currentTonicIndex = 0; // C
int currentScaleIndex = 19; // Major (corresponds to button 19)

// Active Note and Scale Indices
int activeNoteIndex = 7;   // Default to C (button 7)
int activeScaleIndex = 19; // Default to Major (button 19)

// Octave tracking
int octaveShift = 0;       // Default octave (0), can be -1 or +1

// Strum effect
int strumDelay = 0;        // Delay between notes when playing a chord (0-100ms)

// Latch state
bool latchEnabled = false;
int latchedNumeralIndex = -1; // -1 means no chord is latched

// MIDI Note Management
struct MidiNote {
  uint8_t note;
  uint8_t velocity;
  bool active;
} activeNotes[3]; // Supports up to 3 notes (triad)

// Last pressed button tracking for replay functionality
unsigned long lastNoteChangeTime = 0;
unsigned long lastScaleChangeTime = 0;

// ===================== Helper Functions =====================

// Function to send MIDI Note On
inline void sendMidiNoteOn(uint8_t note, uint8_t velocity = 100) {
  midiEventPacket_t noteOn = {0x09, 0x90, note, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

// Function to send MIDI Note Off
inline void sendMidiNoteOff(uint8_t note) {
  midiEventPacket_t noteOff = {0x08, 0x80, note, 0};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

// Stop all currently playing notes
void stopAllNotes() {
  for (int i = 0; i < 3; i++) {
    if (activeNotes[i].active) {
      sendMidiNoteOff(activeNotes[i].note);
      activeNotes[i].active = false;
    }
  }
}

// Generate and play a chord based on the current note, scale, and numeral
void playChord(int numeralIndex) {
  // Calculate the actual notes based on scale intervals
  const int* intervals = scales[currentScaleIndex - 19].intervals;
  
  // Diatonic chord intervals (root, third, fifth)
  const int chordIntervals[3] = {0, 2, 4};
  
  // Base MIDI note (C4 = 60) with octave shift applied
  int baseNote = 60 + (currentTonicIndex) + (octaveShift * 12); 
  
  // Calculate and prepare chord notes (without playing them yet)
  uint8_t midiNotes[3];
  
  for (int i = 0; i < 3; i++) {
    // Find the scale degree
    int scaleDegree = (numeralIndex + chordIntervals[i]) % 7;
    
    // Get the interval from the scale
    int interval = intervals[scaleDegree];
    
    // Calculate actual MIDI note
    int midiNote = baseNote + interval;
    
    // Ensure proper octave (if needed)
    if (i > 0 && midiNote <= midiNotes[i-1]) {
      midiNote += 12;
    }
    
    // Check MIDI note is in valid range (0-127)
    if (midiNote < 0) midiNote = 0;
    if (midiNote > 127) midiNote = 127;
    
    // Store the note
    midiNotes[i] = midiNote;
    activeNotes[i].note = midiNote;
    activeNotes[i].velocity = 100;
    activeNotes[i].active = true;
  }
  
  // Now play the notes with strum effect if enabled
  for (int i = 0; i < 3; i++) {
    sendMidiNoteOn(midiNotes[i]);
    
    // Add delay between notes for strum effect (if potentiometer is turned up)
    if (i < 2 && strumDelay > 0) {
      delay(strumDelay);
    }
  }
  
  // Print chord information
  Serial.print(chromaticScale[currentTonicIndex]);
  Serial.print(" ");
  Serial.print(scales[currentScaleIndex - 19].name);
  Serial.print(" ");
  Serial.print(numerals[numeralIndex]);
  Serial.print(" (Octave ");
  Serial.print(octaveShift);
  Serial.print(", Strum ");
  Serial.print(strumDelay);
  Serial.print("ms): ");
  
  // Get note names for the chord
  for (int i = 0; i < 3; i++) {
    int noteIndex = (activeNotes[i].note - 60) % 12;
    if (noteIndex < 0) noteIndex += 12;
    Serial.print(chromaticScale[noteIndex]);
    Serial.print(" ");
  }
  Serial.println();
}

// ===================== Hardware Interaction Functions =====================

// Write to shift registers to control LEDs
inline void writeShiftRegister(byte* states) {
  digitalWrite(SR_LATCH_PIN, LOW);
  for (int i = 3; i >= 0; i--) {
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, states[i]);
  }
  digitalWrite(SR_LATCH_PIN, HIGH);
}

// Read a button state from a multiplexer
inline bool readMuxButton(int muxIndex, int buttonIndex) {
  int sigPin, s0Pin, s1Pin, s2Pin, s3Pin;
  
  if (muxIndex == 0) {
    sigPin = MUX1_SIG_PIN;
    s0Pin = MUX1_S0_PIN;
    s1Pin = MUX1_S1_PIN;
    s2Pin = MUX1_S2_PIN;
    s3Pin = MUX1_S3_PIN;
  } else {
    sigPin = MUX2_SIG_PIN;
    s0Pin = MUX2_S0_PIN;
    s1Pin = MUX2_S1_PIN;
    s2Pin = MUX2_S2_PIN;
    s3Pin = MUX2_S3_PIN;
  }

  // Set multiplexer address
  digitalWrite(s0Pin, (buttonIndex & 1) ? HIGH : LOW);
  digitalWrite(s1Pin, (buttonIndex & 2) ? HIGH : LOW);
  digitalWrite(s2Pin, (buttonIndex & 4) ? HIGH : LOW);
  digitalWrite(s3Pin, (buttonIndex & 8) ? HIGH : LOW);

  delayMicroseconds(10);              // Allow settling time
  return digitalRead(sigPin) == LOW;  // Button pressed is LOW
}

// ===================== Setup and Loop =====================

void setup() {
  Serial.begin(9600);

  // Initialize multiplexer control pins
  pinMode(MUX1_S0_PIN, OUTPUT);
  pinMode(MUX1_S1_PIN, OUTPUT);
  pinMode(MUX1_S2_PIN, OUTPUT);
  pinMode(MUX1_S3_PIN, OUTPUT);
  pinMode(MUX1_SIG_PIN, INPUT_PULLUP);

  pinMode(MUX2_S0_PIN, OUTPUT);
  pinMode(MUX2_S1_PIN, OUTPUT);
  pinMode(MUX2_S2_PIN, OUTPUT);
  pinMode(MUX2_S3_PIN, OUTPUT);
  pinMode(MUX2_SIG_PIN, INPUT_PULLUP);
  
  // Initialize octave control pins
  pinMode(OCTAVE_UP_PIN, INPUT_PULLUP);
  pinMode(OCTAVE_DOWN_PIN, INPUT_PULLUP);
  
  // Initialize potentiometer
  pinMode(STRUM_POT_PIN, INPUT);

  // Initialize shift register control pins
  pinMode(SR_LATCH_PIN, OUTPUT);
  pinMode(SR_CLOCK_PIN, OUTPUT);
  pinMode(SR_DATA_PIN, OUTPUT);

  // Initialize LED states - set C note and Major scale as defaults
  byte initialStates[4] = { 0 };
  initialStates[0] |= (1 << 7); // C note
  initialStates[2] |= (1 << 3); // Major scale
  writeShiftRegister(initialStates);
  
  // Set default states
  buttonStates[7] = true;  // C note
  buttonStates[19] = true; // Major scale
  
  // Print initial state
  Serial.println("Omnichord-like MIDI controller started");
  Serial.println("Default: C Major, Octave 0");
  
  // Explicitly initialize all state variables to ensure consistent startup state
  currentTonicIndex = 0;  // C
  currentScaleIndex = 19; // Major
  activeNoteIndex = 7;    // C button
  activeScaleIndex = 19;  // Major button
  octaveShift = 0;
  latchEnabled = false;
  latchedNumeralIndex = -1;

  // Log initial chord information for verification
  Serial.print("Initial chord mode: ");
  Serial.print(chromaticScale[currentTonicIndex]);
  Serial.print(" ");
  Serial.println(scales[currentScaleIndex - 19].name);
}

void loop() {
  byte shiftRegisterStates[4] = { 0 };
  
  // Fast reading of button states
  for (int i = 0; i < NUM_BUTTONS; i++) {
    // Skip unused buttons (bass = 28)
    if (i == 28) continue;
    
    int muxIndex = (i < 16) ? 0 : 1;
    int muxButtonIndex = (i < 16) ? i : i - 16;

    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);
    bool stateChanged = (isPressed != prevButtonStates[i]);
    prevButtonStates[i] = isPressed;
    
    if (stateChanged) {
      if (i <= 6) {
        // Numeral Buttons (I-VII)
        if (isPressed) {
          // Button pressed
          if (latchEnabled) {
            // If another chord was latched, stop it and turn off its LED
            if (latchedNumeralIndex != -1) {
              buttonStates[latchedNumeralIndex] = false;
              stopAllNotes();
            }
            // Latch this new chord
            latchedNumeralIndex = i;
            buttonStates[i] = true;
          } else {
            // Normal momentary behavior
            currentlyHeldButtons[i] = true;
            buttonStates[i] = true;
          }
          
          stopAllNotes();
          playChord(i);
        } else if (!latchEnabled) {
          // Button released (only respond if not in latch mode)
          currentlyHeldButtons[i] = false;
          buttonStates[i] = false;
          stopAllNotes();
        }
      } else if (i >= 7 && i <= 18) {
        // Note Buttons (C-B): Toggle with exclusive activation
        if (isPressed) {
          // Turn off previous note LED
          buttonStates[activeNoteIndex] = false;
          
          // Set new note
          activeNoteIndex = i;
          buttonStates[i] = true;
          currentTonicIndex = i - 7;
          
          Serial.print("Note changed to: ");
          Serial.println(chromaticScale[currentTonicIndex]);
          
          // Record timestamp for replay functionality
          lastNoteChangeTime = millis();
          
          // If any numeral buttons are physically held, play new chord with strumming
          if (!latchEnabled) {
            for (int j = 0; j <= 6; j++) {
              if (currentlyHeldButtons[j]) {
                stopAllNotes();
                playChord(j);
                break;
              }
            }
          } else if (latchedNumeralIndex != -1) {
            // If in latch mode and a chord is latched, play the new chord with strumming
            stopAllNotes();
            playChord(latchedNumeralIndex);
          }
        }
      } else if (i >= 19 && i <= 26) {
        // Scale Buttons: Toggle with exclusive activation
        if (isPressed) {
          // Turn off previous scale LED
          buttonStates[activeScaleIndex] = false;
          
          // Set new scale
          activeScaleIndex = i;
          buttonStates[i] = true;
          currentScaleIndex = i;
          
          Serial.print("Scale changed to: ");
          Serial.println(scales[currentScaleIndex - 19].name);
          
          // Record timestamp for replay functionality
          lastScaleChangeTime = millis();
          
          // If any numeral buttons are physically held, play new chord with strumming
          if (!latchEnabled) {
            for (int j = 0; j <= 6; j++) {
              if (currentlyHeldButtons[j]) {
                stopAllNotes();
                playChord(j);
                break;
              }
            }
          } else if (latchedNumeralIndex != -1) {
            // If in latch mode and a chord is latched, play the new chord with strumming
            stopAllNotes();
            playChord(latchedNumeralIndex);
          }
        }
      } else if (i == LATCH_BUTTON) {
        // Latch button - Toggle
        if (isPressed) {
          latchEnabled = !latchEnabled;
          buttonStates[LATCH_BUTTON] = latchEnabled;
          
          Serial.print("Latch mode: ");
          Serial.println(latchEnabled ? "ON" : "OFF");
          
          if (!latchEnabled) {
            // When disabling latch, release any latched chord and turn off its LED
            if (latchedNumeralIndex != -1) {
              buttonStates[latchedNumeralIndex] = false;
              stopAllNotes();
              latchedNumeralIndex = -1;
              
              // Make sure all numeral LEDs are off when exiting latch mode
              for (int j = 0; j <= 6; j++) {
                if (!currentlyHeldButtons[j]) {
                  buttonStates[j] = false;
                }
              }
            }
          }
        }

      }
    }
  }
  
  // Update shift register state for LEDs
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonStates[i]) {
      shiftRegisterStates[i / 8] |= (1 << (i % 8));
    }
  }
  
  // Handle repeated presses (same note or scale) for replay functionality
  for (int j = 0; j <= 6; j++) {
    // Check both physically held buttons and latched button
    bool buttonActive = (!latchEnabled && currentlyHeldButtons[j]) || 
                        (latchEnabled && latchedNumeralIndex == j);
    
    if (buttonActive) {
      bool notePressed = readMuxButton(0, activeNoteIndex);
      bool scalePressed = readMuxButton((activeScaleIndex < 16) ? 0 : 1, 
                                       (activeScaleIndex < 16) ? activeScaleIndex : activeScaleIndex - 16);
      
      // If user presses the currently active note or scale again
      if ((notePressed && (millis() - lastNoteChangeTime > 200)) || 
          (scalePressed && (millis() - lastScaleChangeTime > 200))) {
        // Replay the chord
        stopAllNotes();
        if (!latchEnabled) {
          playChord(j);
        } else {
          playChord(latchedNumeralIndex);
        }
        
        // Update timestamps
        if (notePressed) lastNoteChangeTime = millis();
        if (scalePressed) lastScaleChangeTime = millis();
      }
    }
  }
  
  // Check dedicated octave up/down pins
  static bool prevOctaveUpState = HIGH;
  static bool prevOctaveDownState = HIGH;
  bool currentOctaveUpState = digitalRead(OCTAVE_UP_PIN);
  bool currentOctaveDownState = digitalRead(OCTAVE_DOWN_PIN);
  
  // Octave Up button (detect falling edge - button press)
  if (currentOctaveDownState == LOW && prevOctaveDownState == HIGH) {
    if (octaveShift < 2) {  // Allow up to +2 octaves
      octaveShift++;
      
      Serial.print("Octave shifted up to: ");
      Serial.println(octaveShift);
      
      // Update any playing notes with new octave
      if (!latchEnabled) {
        for (int j = 0; j <= 6; j++) {
          if (currentlyHeldButtons[j]) {
            stopAllNotes();
            playChord(j);
            break;
          }
        }
      } else if (latchedNumeralIndex != -1) {
        stopAllNotes();
        playChord(latchedNumeralIndex);
      }
    }
  }
  prevOctaveDownState = currentOctaveDownState;
  
  // Octave Down button (detect falling edge - button press)
  if (currentOctaveUpState == LOW && prevOctaveUpState == HIGH) {
    if (octaveShift > -2) {  // Allow down to -2 octaves
      octaveShift--;
      
      Serial.print("Octave shifted down to: ");
      Serial.println(octaveShift);
      
      // Update any playing notes with new octave
      if (!latchEnabled) {
        for (int j = 0; j <= 6; j++) {
          if (currentlyHeldButtons[j]) {
            stopAllNotes();
            playChord(j);
            break;
          }
        }
      } else if (latchedNumeralIndex != -1) {
        stopAllNotes();
        playChord(latchedNumeralIndex);
      }
    }
  }
  prevOctaveUpState = currentOctaveUpState;
  prevOctaveDownState = currentOctaveDownState;

  // Write LED states
  writeShiftRegister(shiftRegisterStates);
  
  // Small delay for stability
  delay(10);
  
  // Read potentiometer for strum effect
  int potValue = analogRead(STRUM_POT_PIN); // 0-1023
  
  // Map potentiometer value to a delay time (0-150ms)
  // When pot is at 0, strumDelay = 0 (no delay, all notes play together)
  // When pot is at max, strumDelay = 400ms (significant strum effect)
  strumDelay = map(potValue, 0, 1023, 0, 400);
}