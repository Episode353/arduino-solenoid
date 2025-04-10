/*
  Arduino C++ Code for Omnichord-like Device
  - Implements chromatic scale, enharmonics, major scales, and complexity chords.
  - Interacts with hardware buttons and LEDs via multiplexers and shift registers.
  - Generates and prints chords based on button presses.
  
  Updates:
  1. Note and scale buttons now light up when pressed.
  2. Only one note or one scale can be active at a time.
  3. Complexity is hard-coded to 1.
  4. Inversion is hard-coded to 1.
*/

#include <Arduino.h>

// ===================== Constants and Definitions =====================

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

// Major scale intervals in semitones
const int majorScaleIntervals[7] = {0, 2, 4, 5, 7, 9, 11};

// Complexity chords mapping (hard-coded to complexity 1)
const int complexityChords[1][3] = {
  {0, 4, 7} // Complexity 1: Major Triad (Root, Major Third, Perfect Fifth)
};
const int chordSizes[1] = {3};

// Button Mapping
// Button 0-6: Chords I-VII (Momentary)
// Button 7-18: Notes C to B
// Button 19-24: Major Scales (for future expansion, currently only Major Scale is used)
const int NUM_BUTTONS = 31;

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
const char* currentScaleType = "Major";

// Active Note and Scale Indices
int activeNoteIndex = -1;   // -1 means no active note
int activeScaleIndex = -1;  // -1 means no active scale

// Chord Notes (Maximum size is 3 for complexity 1)
String chord[3];

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

// Generate major scale based on tonic
void generateMajorScale(String tonic, String scale[7]) {
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
    int noteIndex = (tonicIndex + majorScaleIntervals[i]) % 12;
    scale[i] = String(chromaticScale[noteIndex]);
  }
}

// Build a chord from the scale (corrected for diatonic scale mapping)
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

  // Clear shift registers
  byte initialStates[4] = { 0 };
  writeShiftRegister(initialStates);
}

void loop() {
  byte shiftRegisterStates[4] = { 0 };
  String scale[7];

  // Generate current major scale
  generateMajorScale(currentTonic, scale);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    int muxIndex = i / 16;       // 0 for MUX1, 1 for MUX2
    int muxButtonIndex = i % 16;

    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);

    if (i <= 6) {
      // Chord Buttons (0-6): Momentary behavior
      buttonStates[i] = isPressed;
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Chord Button ");
        Serial.print(numerals[i]);
        Serial.println(" pressed");

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
        }

        // Toggle current note
        if (activeNoteIndex == i) {
          buttonStates[i] = false;
          activeNoteIndex = -1;
        }
        else {
          buttonStates[i] = true;
          activeNoteIndex = i;
          // Update current tonic
          currentTonic = String(chromaticScale[i - 7]);
          Serial.print("Tonic changed to: ");
          Serial.println(currentTonic);
        }
      }
    }
    else if (i >=19 && i <=24) {
      // Scale Buttons (19-24): Toggle behavior with exclusive activation
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Scale Button ");
        Serial.print(currentScaleType); // Currently only Major Scale is implemented
        Serial.println(" pressed");

        // Deactivate previously active scale
        if (activeScaleIndex != -1 && activeScaleIndex != i) {
          buttonStates[activeScaleIndex] = false;
        }

        // Toggle current scale (since only Major Scale is implemented, no actual change)
        if (activeScaleIndex == i) {
          buttonStates[i] = false;
          activeScaleIndex = -1;
        }
        else {
          buttonStates[i] = true;
          activeScaleIndex = i;
          // Currently only Major Scale is implemented
          currentScaleType = "Major";
          Serial.println("Scale type set to Major");
        }
      }
    }
    else {
      // Other buttons: Toggle behavior or specific actions (Buttons 19-30 are handled above)
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Button ");
        Serial.print(i);
        Serial.println(" pressed");

        // Currently, only handle Major Scale (Button 19)
        if (i == 19){
          // Set scale to Major
          currentScaleType = "Major";
          Serial.println("Scale type set to Major");
          // Ensure only this scale button is active
          if (activeScaleIndex != -1 && activeScaleIndex != i) {
            buttonStates[activeScaleIndex] = false;
          }
          activeScaleIndex = i;
          buttonStates[i] = true;
        }

        // Add more button functionalities as needed
      }
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
}
