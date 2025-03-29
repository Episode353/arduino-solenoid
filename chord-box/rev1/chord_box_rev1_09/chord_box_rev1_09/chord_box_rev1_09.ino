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
const int NUM_BUTTONS = 27;

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
int currentScaleIndex = 0; // Major

// Active Note and Scale Indices
int activeNoteIndex = 7;   // Default to C (button 7)
int activeScaleIndex = 19; // Default to Major (button 19)

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
  
  // Base MIDI note (C4 = 60)
  int baseNote = 60 + (currentTonicIndex); 
  
  // Calculate and play chord notes
  for (int i = 0; i < 3; i++) {
    // Find the scale degree
    int scaleDegree = (numeralIndex + chordIntervals[i]) % 7;
    
    // Get the interval from the scale
    int interval = intervals[scaleDegree];
    
    // Calculate actual MIDI note
    int midiNote = baseNote + interval;
    
    // Ensure proper octave (if needed)
    if (i > 0 && midiNote <= activeNotes[i-1].note) {
      midiNote += 12;
    }
    
    // Store and play the note
    activeNotes[i].note = midiNote;
    activeNotes[i].velocity = 100;
    activeNotes[i].active = true;
    
    sendMidiNoteOn(midiNote);
  }
  
  // Print chord information
  Serial.print(chromaticScale[currentTonicIndex]);
  Serial.print(" ");
  Serial.print(scales[currentScaleIndex - 19].name);
  Serial.print(" ");
  Serial.print(numerals[numeralIndex]);
  Serial.print(": ");
  
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
  
  Serial.println("Omnichord-like MIDI controller started");
  Serial.println("Default: C Major");
}

void loop() {
  byte shiftRegisterStates[4] = { 0 };
  
  // Fast reading of button states
  for (int i = 0; i < NUM_BUTTONS; i++) {
    // Skip disabled buttons (latch, bass, octave, chromatic, random, time, complexity, inversion)
    if (i == 27 || i == 28 || i == 29 || i == 30) continue;
    
    int muxIndex = (i < 16) ? 0 : 1;
    int muxButtonIndex = (i < 16) ? i : i - 16;

    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);
    bool stateChanged = (isPressed != prevButtonStates[i]);
    prevButtonStates[i] = isPressed;
    
    if (stateChanged) {
      if (i <= 6) {
        // Numeral Buttons (I-VII): Momentary behavior
        if (isPressed) {
          // Button pressed
          currentlyHeldButtons[i] = true;
          stopAllNotes();
          playChord(i);
        } else {
          // Button released
          currentlyHeldButtons[i] = false;
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
          
          // If any numeral buttons are held, play new chord
          for (int j = 0; j <= 6; j++) {
            if (currentlyHeldButtons[j]) {
              stopAllNotes();
              playChord(j);
              break;
            }
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
          
          // If any numeral buttons are held, play new chord
          for (int j = 0; j <= 6; j++) {
            if (currentlyHeldButtons[j]) {
              stopAllNotes();
              playChord(j);
              break;
            }
          }
        }
      }
    }
    
    // Update shift register state for LEDs for this button
    if (buttonStates[i]) {
      shiftRegisterStates[i / 8] |= (1 << (i % 8));
    }
  }
  
  // Handle repeated presses (same note or scale) for replay functionality
  for (int i = 0; i <= 6; i++) {
    if (currentlyHeldButtons[i]) {
      bool notePressed = readMuxButton(0, activeNoteIndex);
      bool scalePressed = readMuxButton((activeScaleIndex < 16) ? 0 : 1, 
                                       (activeScaleIndex < 16) ? activeScaleIndex : activeScaleIndex - 16);
      
      // If user presses the currently active note or scale again while holding a numeral
      if ((notePressed && (millis() - lastNoteChangeTime > 200)) || 
          (scalePressed && (millis() - lastScaleChangeTime > 200))) {
        // Replay the chord
        stopAllNotes();
        playChord(i);
        
        // Update timestamps
        if (notePressed) lastNoteChangeTime = millis();
        if (scalePressed) lastScaleChangeTime = millis();
      }
    }
  }

  // Write LED states
  writeShiftRegister(shiftRegisterStates);
  
  // Small delay for stability
  delay(10);
}