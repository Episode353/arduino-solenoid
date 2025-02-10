/*
  Arduino C++ Code for Omnichord-like Device with MIDI USB Output
  - Implements chromatic scale, enharmonics, major scales, and complexity chords.
  - Interacts with hardware buttons and LEDs via multiplexers and shift registers.
  - Generates and sends MIDI chords based on button presses.
  
  Updates:
  1. Note and scale buttons now light up when pressed.
  2. Only one note or one scale can be active at a time.
  3. Complexity is hard-coded to 1.
  4. Inversion is hard-coded to 1.
  5. Sends MIDI Note On and Note Off messages over USB.
  6. Allows spanning three octaves with octave shift buttons.
*/

#include <Arduino.h>
#include <MIDIUSB.h> // Include MIDIUSB library

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

// Enharmonic mappings
const char* enharmonicsInput[] = {
  "Cb", "B#", "E#", "Fb", "C##", "D##", "F##", "G##", "A##", "B##"
};
const char* enharmonicsNormalized[] = {
  "B",  "C",  "F",  "E",  "C#", "E",  "G",  "A",  "C",  "C"
};
const int numEnharmonics = sizeof(enharmonicsInput) / sizeof(enharmonicsInput[0]);

// Complexity chords mapping (hard-coded to complexity 1)
const int complexityChords[1][3] = {
  {0, 4, 7} // Complexity 1: Major Triad (Root, Major Third, Perfect Fifth)
};
const int chordSizes[1] = {3};

// Button Mapping
// Button 0-6: Chords I-VII (Momentary)
// Button 7-18: Notes C to B
// Button 19-20: Octave Shift Up and Down
// Button 21-30: Major Scales (for future expansion, currently only Major Scale is used)
const int NUM_BUTTONS = 31;

// Octave Shift Definitions
#define OCTAVE_UP_BUTTON 19
#define OCTAVE_DOWN_BUTTON 20
const int MIN_OCTAVE_SHIFT = -1; // One octave down
const int MAX_OCTAVE_SHIFT = 1;  // One octave up

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

// Numerals for Major Scale
const char* numerals[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};

// State Arrays
bool buttonStates[NUM_BUTTONS] = { false };
bool prevButtonStates[NUM_BUTTONS] = { false };

// Current Scale Information
String currentTonic = "C";
const char* currentScaleType = "Major"; // Default scale

// Active Note and Scale Indices
int activeNoteIndex = -1;   // -1 means no active note
int activeScaleIndex = -1;  // -1 means no active scale

// Chord Notes (Maximum size is 3 for complexity 1)
String chord[3];

// Octave Shift Variable
int octaveShift = 0; // 0 = default octave, -1 = one octave down, +1 = one octave up

// ===================== Helper Functions =====================

// Normalize a note using enharmonic mappings
String normalizeNote(String note) {
  // Capitalize the first letter and handle case
  if (note.length() > 0) {
    note[0] = toupper(note[0]);
    for (unsigned int i = 1; i < note.length(); i++) {
      note[i] = tolower(note[i]);
    }
  }

  // Check enharmonic mappings
  for (int i = 0; i < numEnharmonics; i++) {
    if (note.equalsIgnoreCase(enharmonicsInput[i])) {
      return String(enharmonicsNormalized[i]);
    }
  }

  return note;
}

void generateScale(String tonic, String scale[7], const char* scaleType) {
  // Find the scale intervals
  const int* intervals = nullptr;
  for (int i = 0; i < numScales; i++) {
    if (String(scales[i].name).equalsIgnoreCase(scaleType)) {
      intervals = scales[i].intervals;
      break;
    }
  }

  if (!intervals) {
    Serial.println("Invalid scale type. Defaulting to Major.");
    intervals = scales[0].intervals; // Default to Major
  }

  // Normalize tonic
  tonic = normalizeNote(tonic);

  // Find the index of the tonic in the chromatic scale
  int tonicIndex = -1;
  for (int i = 0; i < 12; i++) {
    if (String(chromaticScale[i]) == tonic) {
      tonicIndex = i;
      break;
    }
  }

  if (tonicIndex == -1) {
    Serial.println("Invalid tonic. Defaulting to C.");
    tonicIndex = 0; // Default to C
  }

  // Generate scale notes
  for (int i = 0; i < 7; i++) {
    int noteIndex = (tonicIndex + intervals[i]) % 12;
    scale[i] = String(chromaticScale[noteIndex]);
  }
}

// MIDI Note Management
struct MidiNote {
  uint8_t note;
  uint8_t velocity;
  bool active;
} midiNotes[3]; // Supports up to 3 simultaneous notes (triad)

// Function to send MIDI Note On
void sendMidiNoteOn(uint8_t note, uint8_t velocity = 127) {
  midiEventPacket_t noteOn = {0x09, 0x90, note, velocity}; // Note On, channel 1
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

// Function to send MIDI Note Off
void sendMidiNoteOff(uint8_t note, uint8_t velocity = 0) {
  midiEventPacket_t noteOff = {0x08, 0x80, note, velocity}; // Note Off, channel 1
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

// Function to send all chord notes as Note On
void sendChordNotes() {
  // Send Note On for each chord note
  for (int i = 0; i < chordSizes[0]; i++) {
    String noteStr = chord[i];
    // Convert note string to MIDI note number
    // Assuming base octave (C4 = 60)
    int baseNote = 60; // C4
    int semitoneOffset = 0;
    for (int j = 0; j < 12; j++) {
      if (String(chromaticScale[j]) == noteStr) {
        semitoneOffset = j;
        break;
      }
    }
    int midiNote = baseNote + semitoneOffset;
    // Apply octave shift
    midiNote += octaveShift * 12;

    // Ensure MIDI note is within valid range
    if (midiNote < 0) midiNote = 0;
    if (midiNote > 127) midiNote = 127;

    // Store active notes
    midiNotes[i].note = midiNote;
    midiNotes[i].velocity = 127;
    midiNotes[i].active = true;

    // Send Note On
    sendMidiNoteOn(midiNote);
  }
}

// Function to send all chord notes as Note Off
void sendChordNotesOff() {
  // Send Note Off for each active chord note
  for (int i = 0; i < chordSizes[0]; i++) {
    if (midiNotes[i].active) {
      sendMidiNoteOff(midiNotes[i].note);
      midiNotes[i].active = false;
    }
  }
}

// Invert a chord (corrected to rotate notes)
bool invertChord(int chordSize) {
  if (chordSize <= 0) return false;

  // Inversion is hard-coded to 1
  int inversion = 1;

  // Rotate the chord array
  for (int i = 0; i < inversion; i++) {
    String temp = chord[0];
    for (int j = 0; j < chordSize - 1; j++) {
      chord[j] = chord[j + 1];
    }
    chord[chordSize - 1] = temp;
  }

  return true;
}

// Build chord with inversion (corrected)
bool buildChordWithInversion(String scale[7], int numeralIndex, bool debug = false) {
  if (!buildChord(scale, numeralIndex)) return false;
  if (!invertChord(3)) return false; // Triad has 3 notes

  if (debug) {
    Serial.print("Inverted Chord: ");
    for (int i = 0; i < 3; i++) {
      Serial.print(chord[i] + " ");
    }
    Serial.println();
  }

  // Send MIDI Note On for the new chord
  sendChordNotes();

  // Optionally, send Note Off for the previous chord if needed
  // You can implement this based on your specific requirements

  return true;
}

// ===================== Hardware Interaction Functions =====================

// Write to shift registers to control LEDs
void writeShiftRegister(byte* states) {
  digitalWrite(SR_LATCH_PIN, LOW);
  for (int i = 3; i >= 0; i--) {  // Assuming 4 shift registers for 32 LEDs
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, states[i]);
  }
  digitalWrite(SR_LATCH_PIN, HIGH);
}

// Read a button state from a multiplexer
bool readMuxButton(int muxIndex, int buttonIndex) {
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

  // Initialize LED states to off
  byte initialStates[4] = { 0 };
  writeShiftRegister(initialStates);
}

void loop() {
  byte shiftRegisterStates[4] = { 0 };
  String scale[7];

  // Generate current scale
  generateScale(currentTonic, scale, currentScaleType);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    int muxIndex = (i < 16) ? 0 : 1;       // 0 for MUX1 (buttons 0-15), 1 for MUX2 (buttons 16-30)
    int muxButtonIndex = (i < 16) ? i : i - 16;

    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);

    if (i <= 6) {
      // Chord Buttons (0-6): Momentary behavior
      buttonStates[i] = isPressed;
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Chord Button ");
        Serial.print(numerals[i]);
        Serial.println(" pressed");

        // Send Note Off for previous chord before playing a new one
        sendChordNotesOff();

        // Complexity and inversion are hard-coded to 1
        buildChordWithInversion(scale, i, true);

        // Print generated chord
        Serial.print("Generated Chord: ");
        for(int c = 0; c < chordSizes[0]; c++) {
          Serial.print(chord[c] + " ");
        }
        Serial.println();
      }
    }
    else if (i >=7 && i <=18) {
      // Note Buttons (7-18): Toggle behavior with exclusive activation
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Note Button ");
        Serial.print(chromaticScale[i - 7]);
        Serial.println(" pressed");

        // Deactivate previously active note
        if (activeNoteIndex != -1 && activeNoteIndex != i) {
          buttonStates[activeNoteIndex] = false;
          // Send MIDI Note Off for the previously active note
          String prevNoteStr = chromaticScale[activeNoteIndex - 7];
          int baseNote = 60; // C4
          int semitoneOffset = 0;
          for (int j = 0; j < 12; j++) {
            if (String(chromaticScale[j]) == prevNoteStr) {
              semitoneOffset = j;
              break;
            }
          }
          int prevMidiNote = baseNote + semitoneOffset;
          prevMidiNote += octaveShift * 12;
          // Ensure MIDI note is within valid range
          if (prevMidiNote >= 0 && prevMidiNote <= 127) {
            sendMidiNoteOff(prevMidiNote);
          }
        }

        // Toggle current note
        if (activeNoteIndex == i) {
          buttonStates[i] = false;
          activeNoteIndex = -1;
          // Send MIDI Note Off for the current note
          String currentNoteStr = chromaticScale[i - 7];
          int baseNote = 60; // C4
          int semitoneOffset = 0;
          for (int j = 0; j < 12; j++) {
            if (String(chromaticScale[j]) == currentNoteStr) {
              semitoneOffset = j;
              break;
            }
          }
          int currentMidiNote = baseNote + semitoneOffset;
          currentMidiNote += octaveShift * 12;
          // Ensure MIDI note is within valid range
          if (currentMidiNote >= 0 && currentMidiNote <= 127) {
            sendMidiNoteOff(currentMidiNote);
          }
        }
        else {
          buttonStates[i] = true;
          activeNoteIndex = i;
          // Update current tonic
          currentTonic = String(chromaticScale[i - 7]);
          Serial.print("Tonic changed to: ");
          Serial.println(currentTonic);

          // Send MIDI Note On for the new tonic
          int baseNote = 60; // C4
          int semitoneOffset = 0;
          for (int j = 0; j < 12; j++) {
            if (String(chromaticScale[j]) == currentTonic) {
              semitoneOffset = j;
              break;
            }
          }
          int midiNote = baseNote + semitoneOffset;
          midiNote += octaveShift * 12;
          // Ensure MIDI note is within valid range
          if (midiNote >= 0 && midiNote <= 127) {
            sendMidiNoteOn(midiNote);
          }
        }
      }
    }
    else if (i == OCTAVE_UP_BUTTON || i == OCTAVE_DOWN_BUTTON) {
      // Octave Shift Buttons (19-20)
      if (isPressed && !prevButtonStates[i]) {
        if (i == OCTAVE_UP_BUTTON) {
          if (octaveShift < MAX_OCTAVE_SHIFT) {
            octaveShift++;
            Serial.print("Octave Shift increased to: ");
            Serial.println(octaveShift);
            // Update LEDs for octave shift buttons
            buttonStates[OCTAVE_UP_BUTTON] = (octaveShift > 0);
            buttonStates[OCTAVE_DOWN_BUTTON] = (octaveShift < 0);
            // Update shift register states
          }
        }
        else if (i == OCTAVE_DOWN_BUTTON) {
          if (octaveShift > MIN_OCTAVE_SHIFT) {
            octaveShift--;
            Serial.print("Octave Shift decreased to: ");
            Serial.println(octaveShift);
            // Update LEDs for octave shift buttons
            buttonStates[OCTAVE_UP_BUTTON] = (octaveShift > 0);
            buttonStates[OCTAVE_DOWN_BUTTON] = (octaveShift < 0);
          }
        }

        // After shifting octaves, update active notes
        if (activeNoteIndex != -1) {
          // Send Note Off for the previous note
          String prevNoteStr = chromaticScale[activeNoteIndex - 7];
          int baseNote = 60; // C4
          int semitoneOffset = 0;
          for (int j = 0; j < 12; j++) {
            if (String(chromaticScale[j]) == prevNoteStr) {
              semitoneOffset = j;
              break;
            }
          }
          int prevMidiNote = baseNote + semitoneOffset;
          prevMidiNote += (octaveShift - ((i == OCTAVE_UP_BUTTON) ? 1 : -1)) * 12;
          // Ensure MIDI note is within valid range
          if (prevMidiNote >= 0 && prevMidiNote <= 127) {
            sendMidiNoteOff(prevMidiNote);
          }

          // Send Note On for the new note with updated octave
          int newMidiNote = baseNote + semitoneOffset;
          newMidiNote += octaveShift * 12;
          // Ensure MIDI note is within valid range
          if (newMidiNote >= 0 && newMidiNote <= 127) {
            sendMidiNoteOn(newMidiNote);
          }
        }
      }
    }
    else if (i >=21 && i <=30) {
      // Scale Buttons (21-30): Toggle behavior with exclusive activation
      if (isPressed && !prevButtonStates[i]) {
        int scaleIndex = i - 21;
        if (scaleIndex < numScales) {
          Serial.print("Scale Button ");
          Serial.print(scales[scaleIndex].name);
          Serial.println(" pressed");

          // Deactivate previously active scale
          if (activeScaleIndex != -1 && activeScaleIndex != i) {
            buttonStates[activeScaleIndex] = false;
          }

          // Toggle current scale
          if (activeScaleIndex == i) {
            buttonStates[i] = false;
            activeScaleIndex = -1;
            currentScaleType = "Major"; // Default to Major if no scale is active
            Serial.println("Scale type reset to Major");
          } else {
            buttonStates[i] = true;
            activeScaleIndex = i;
            currentScaleType = scales[scaleIndex].name;
            Serial.print("Scale type set to ");
            Serial.println(currentScaleType);
          }
        }
      }
    }
    else {
      // Other buttons: Handle if any (buttons 21-30 are handled above)
      // Currently, no action for buttons beyond 30
    }

    prevButtonStates[i] = isPressed;

    // Update shift register state for LEDs
    if (buttonStates[i]) {
      shiftRegisterStates[i / 8] |= (1 << (i % 8));  // Set bit
    } else {
      shiftRegisterStates[i / 8] &= ~(1 << (i % 8));  // Clear bit
    }
  }

  // Write LED states
  writeShiftRegister(shiftRegisterStates);
  delay(50);  // Debounce delay

  // Handle MIDIUSB tasks
  MidiUSB.flush();
}

// Function to build chord (root, third, fifth)
bool buildChord(String scale[7], int numeralIndex) {
  // Complexity is hard-coded to 1 (Major Triad)
  const int offsets[3] = {0, 2, 4}; // Diatonic scale intervals for root, third, fifth
  const int chordSize = 3; // Major triad has 3 notes

  // Build chord notes
  for (int i = 0; i < chordSize; i++) {
    int noteIndex = (numeralIndex + offsets[i]) % 7; // Wrap within diatonic scale
    chord[i] = scale[noteIndex];
  }

  return true;
}
