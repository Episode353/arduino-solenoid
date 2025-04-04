/*
  Arduino C++ Code for an Omnichord-like Device with MIDI USB Output
  -- Revised chord–generation functionality based on the new C++ code.
  -- Generates scales (Major/Minor) with proper note names and builds chords
     using a “complexity” offset array and an inversion parameter.
  
  Updates compared to the original:
    1. The chord–generation functions (buildChord, invertChord, etc.)
       have been replaced by new functions (generateScaleNew() and buildChordNew()).
    2. Complexity and inversion are now parameters (defaulting to 1).
    3. When a chord button (0–6) is pressed, the new chord is generated,
       debug–printed, and its notes are sent as MIDI Note On messages.
  
  (Note: Only Major and Minor scale mappings are implemented.
         You can extend generateScaleNew() to support additional keys.)
*/

#include <Arduino.h>
#include <MIDIUSB.h> // Include the MIDIUSB library

// ===================== Constants and Definitions =====================

// --- Original definitions for scales, chromatic scale, and enharmonics ---

// Chromatic scale (used to convert note names to MIDI numbers)
const char* chromaticScale[12] = {
  "C", "C#", "D", "D#", "E", "F",
  "F#", "G", "G#", "A", "A#", "B"
};

// Enharmonic mappings (for normalizing note names before sending MIDI)
const char* enharmonicsInput[] = {
  "Cb", "B#", "E#", "Fb", "C##", "D##", "F##", "G##", "A##", "B##"
};
const char* enharmonicsNormalized[] = {
  "B",  "C",  "F",  "E",  "C#", "E",  "G",  "A",  "C",  "C"
};
const int numEnharmonics = sizeof(enharmonicsInput) / sizeof(enharmonicsInput[0]);

// --------------------- New Chord Generation Definitions ---------------------

// Global parameters for chord complexity and inversion.
// (These can be later adjusted via buttons or other input methods.)
int currentComplexity = 1; // Valid levels: 1 to 10 (default = 1)
int currentInversion  = 1; // Default inversion (0 = no inversion)

// Complexity chords mapping (each complexity level is a set of scale–degree offsets)
// The new code uses scale–degree offsets (e.g. 0,2,4 for a triad built from the scale).
// (Index 0 corresponds to the chord's “I” degree.)
const int complexityOffsets[10][10] = {
  {0, 2, 4},         // Complexity 1: triad (3 notes)
  {0, 2, 4, 5},      // Complexity 2: 4-note chord
  {0, 2, 4, 6},      // Complexity 3: 4-note chord (with a different extension)
  {0, 3, 4, 6},      // Complexity 4: 4-note chord (altered third)
  {0, 3, 4},         // Complexity 5: triad with an altered root-to-third spacing
  {0, 2, 5, 6},      // Complexity 6: 4-note chord
  {0, 2, 4, 6, 8},   // Complexity 7: 5-note chord
  {0, 2, 4, 6, 5},   // Complexity 8: 5-note chord (order changed)
  {0, 2, 4, 6, 9},   // Complexity 9: 5-note chord (extension)
  {0, 2, 4, 6, 5, 10} // Complexity 10: 6-note chord
};
// Sizes for each complexity level (number of offsets)
const int complexitySizes[10] = {3, 4, 4, 4, 3, 4, 5, 5, 5, 6};

// Global array to hold the generated chord notes (max 6 notes)
String chord[6];
int chordSize = 0;

// -------------------- Hardware Button & LED Definitions --------------------

// Button Mapping
// Button 0-6: Chord Buttons (Momentary)
// Button 7-18: Note Buttons (Tonic selection)
// Button 19-24: Scale Buttons (e.g. Major, Minor – currently only Major/Minor are supported)
const int NUM_BUTTONS = 31;

// Current scale settings
String currentTonic = "C";
String currentScaleType = "Major"; // Default scale type

// State arrays for button presses
bool buttonStates[NUM_BUTTONS] = { false };
bool prevButtonStates[NUM_BUTTONS] = { false };

// Numerals for chord buttons (I, II, III, IV, V, VI, VII)
const char* numerals[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};

// -------------------- Multiplexer and Shift Register Pins --------------------

// Multiplexer 1 (buttons 0-15)
#define MUX1_SIG_PIN A3
#define MUX1_S0_PIN 2
#define MUX1_S1_PIN 3
#define MUX1_S2_PIN 4
#define MUX1_S3_PIN 5

// Multiplexer 2 (buttons 16-31)
#define MUX2_SIG_PIN A2
#define MUX2_S0_PIN 12
#define MUX2_S1_PIN 11
#define MUX2_S2_PIN 10
#define MUX2_S3_PIN 9

// Shift Register (for driving LEDs)
#define SR_LATCH_PIN 8
#define SR_CLOCK_PIN 7
#define SR_DATA_PIN 6

// ===================== Helper Functions =====================

// Normalize a note using enharmonic mappings
String normalizeNote(String note) {
  if (note.length() > 0) {
    note[0] = toupper(note[0]);
    for (unsigned int i = 1; i < note.length(); i++) {
      note[i] = tolower(note[i]);
    }
  }
  for (int i = 0; i < numEnharmonics; i++) {
    if (note.equalsIgnoreCase(enharmonicsInput[i])) {
      return String(enharmonicsNormalized[i]);
    }
  }
  return note;
}

// -------------------- New Scale and Chord Generation Functions --------------------

// generateScaleNew()
// Fills the provided 7-element array "scale" with note names based on the tonic and scale type.
// (Only "Major" and "Minor" are supported here.)
bool generateScaleNew(const String &tonic, const String &scaleType, String scale[7]) {
  if (scaleType.equalsIgnoreCase("Major")) {
    if (tonic.equalsIgnoreCase("C")) {
      scale[0] = "C";  scale[1] = "D";  scale[2] = "E";  scale[3] = "F";  scale[4] = "G";  scale[5] = "A";  scale[6] = "B";
      return true;
    } else if (tonic.equalsIgnoreCase("C#")) {
      scale[0] = "C#"; scale[1] = "D#"; scale[2] = "E#"; scale[3] = "F#"; scale[4] = "G#"; scale[5] = "A#"; scale[6] = "B#";
      return true;
    } else if (tonic.equalsIgnoreCase("Db")) {
      scale[0] = "Db"; scale[1] = "Eb"; scale[2] = "F";  scale[3] = "Gb"; scale[4] = "Ab"; scale[5] = "Bb"; scale[6] = "C";
      return true;
    } else if (tonic.equalsIgnoreCase("D")) {
      scale[0] = "D";  scale[1] = "E";  scale[2] = "F#"; scale[3] = "G";  scale[4] = "A";  scale[5] = "B";  scale[6] = "C#";
      return true;
    } else if (tonic.equalsIgnoreCase("D#")) {
      scale[0] = "D#"; scale[1] = "E#"; scale[2] = "F##"; scale[3] = "G#"; scale[4] = "A#"; scale[5] = "B#"; scale[6] = "C##";
      return true;
    } else if (tonic.equalsIgnoreCase("Eb")) {
      scale[0] = "Eb"; scale[1] = "F";  scale[2] = "G";  scale[3] = "Ab"; scale[4] = "Bb"; scale[5] = "C";  scale[6] = "D";
      return true;
    } else if (tonic.equalsIgnoreCase("E")) {
      scale[0] = "E";  scale[1] = "F#"; scale[2] = "G#"; scale[3] = "A";  scale[4] = "B";  scale[5] = "C#"; scale[6] = "D#";
      return true;
    } else if (tonic.equalsIgnoreCase("Fb")) {
      scale[0] = "Fb"; scale[1] = "Gb"; scale[2] = "Ab"; scale[3] = "Bbb"; scale[4] = "Cb"; scale[5] = "Db"; scale[6] = "Eb";
      return true;
    } else if (tonic.equalsIgnoreCase("F")) {
      scale[0] = "F";  scale[1] = "G";  scale[2] = "A";  scale[3] = "Bb"; scale[4] = "C";  scale[5] = "D";  scale[6] = "E";
      return true;
    } else if (tonic.equalsIgnoreCase("F#")) {
      scale[0] = "F#"; scale[1] = "G#"; scale[2] = "A#"; scale[3] = "B";  scale[4] = "C#"; scale[5] = "D#"; scale[6] = "E#";
      return true;
    } else if (tonic.equalsIgnoreCase("Gb")) {
      scale[0] = "Gb"; scale[1] = "Ab"; scale[2] = "Bb"; scale[3] = "Cb"; scale[4] = "Db"; scale[5] = "Eb"; scale[6] = "F";
      return true;
    } else if (tonic.equalsIgnoreCase("G")) {
      scale[0] = "G";  scale[1] = "A";  scale[2] = "B";  scale[3] = "C";  scale[4] = "D";  scale[5] = "E";  scale[6] = "F#";
      return true;
    } else if (tonic.equalsIgnoreCase("Ab")) {
      scale[0] = "Ab"; scale[1] = "Bb"; scale[2] = "C";  scale[3] = "Db"; scale[4] = "Eb"; scale[5] = "F";  scale[6] = "G";
      return true;
    } else if (tonic.equalsIgnoreCase("A")) {
      scale[0] = "A";  scale[1] = "B";  scale[2] = "C#"; scale[3] = "D";  scale[4] = "E";  scale[5] = "F#"; scale[6] = "G#";
      return true;
    } else if (tonic.equalsIgnoreCase("A#")) {
      scale[0] = "A#"; scale[1] = "B#"; scale[2] = "C##"; scale[3] = "D#"; scale[4] = "E#"; scale[5] = "F##"; scale[6] = "G##";
      return true;
    } else if (tonic.equalsIgnoreCase("Bb")) {
      scale[0] = "Bb"; scale[1] = "C";  scale[2] = "D";  scale[3] = "Eb"; scale[4] = "F";  scale[5] = "G";  scale[6] = "A";
      return true;
    } else if (tonic.equalsIgnoreCase("B")) {
      scale[0] = "B";  scale[1] = "C#"; scale[2] = "D#"; scale[3] = "E";  scale[4] = "F#"; scale[5] = "G#"; scale[6] = "A#";
      return true;
    } else if (tonic.equalsIgnoreCase("Cb")) {
      scale[0] = "Cb"; scale[1] = "Db"; scale[2] = "Eb"; scale[3] = "Fb"; scale[4] = "Gb"; scale[5] = "Ab"; scale[6] = "Bb";
      return true;
    }
  }
  else if (scaleType.equalsIgnoreCase("Minor")) {
    if (tonic.equalsIgnoreCase("C")) {
      scale[0] = "C";  scale[1] = "D";  scale[2] = "Eb"; scale[3] = "F";  scale[4] = "G";  scale[5] = "Ab"; scale[6] = "Bb";
      return true;
    } else if (tonic.equalsIgnoreCase("C#")) {
      scale[0] = "C#"; scale[1] = "D#"; scale[2] = "E";  scale[3] = "F#"; scale[4] = "G#"; scale[5] = "A";  scale[6] = "B";
      return true;
    } else if (tonic.equalsIgnoreCase("Db")) {
      scale[0] = "Db"; scale[1] = "Eb"; scale[2] = "Fb"; scale[3] = "Gb"; scale[4] = "Ab"; scale[5] = "Bbb"; scale[6] = "Cb";
      return true;
    } else if (tonic.equalsIgnoreCase("D")) {
      scale[0] = "D";  scale[1] = "E";  scale[2] = "F";  scale[3] = "G";  scale[4] = "A";  scale[5] = "Bb"; scale[6] = "C";
      return true;
    } else if (tonic.equalsIgnoreCase("D#")) {
      scale[0] = "D#"; scale[1] = "E#"; scale[2] = "F#"; scale[3] = "G#"; scale[4] = "A#"; scale[5] = "B";  scale[6] = "C#";
      return true;
    } else if (tonic.equalsIgnoreCase("Eb")) {
      scale[0] = "Eb"; scale[1] = "F";  scale[2] = "Gb"; scale[3] = "Ab"; scale[4] = "Bb"; scale[5] = "Cb"; scale[6] = "Db";
      return true;
    } else if (tonic.equalsIgnoreCase("E")) {
      scale[0] = "E";  scale[1] = "F#"; scale[2] = "G";  scale[3] = "A";  scale[4] = "B";  scale[5] = "C";  scale[6] = "D";
      return true;
    } else if (tonic.equalsIgnoreCase("Fb")) {
      scale[0] = "Fb"; scale[1] = "Gb"; scale[2] = "Abb"; scale[3] = "Bbb"; scale[4] = "Cb"; scale[5] = "Dbb"; scale[6] = "Ebb";
      return true;
    } else if (tonic.equalsIgnoreCase("F")) {
      scale[0] = "F";  scale[1] = "G";  scale[2] = "Ab"; scale[3] = "Bb"; scale[4] = "C";  scale[5] = "Db"; scale[6] = "Eb";
      return true;
    } else if (tonic.equalsIgnoreCase("F#")) {
      scale[0] = "F#"; scale[1] = "G#"; scale[2] = "A";  scale[3] = "B";  scale[4] = "C#"; scale[5] = "D";  scale[6] = "E";
      return true;
    } else if (tonic.equalsIgnoreCase("Gb")) {
      scale[0] = "Gb"; scale[1] = "Ab"; scale[2] = "Bbb"; scale[3] = "Cb"; scale[4] = "Db"; scale[5] = "Ebb"; scale[6] = "Fb";
      return true;
    } else if (tonic.equalsIgnoreCase("G")) {
      scale[0] = "G";  scale[1] = "A";  scale[2] = "Bb"; scale[3] = "C";  scale[4] = "D";  scale[5] = "Eb"; scale[6] = "F";
      return true;
    } else if (tonic.equalsIgnoreCase("Ab")) {
      scale[0] = "Ab"; scale[1] = "Bb"; scale[2] = "Cb"; scale[3] = "Db"; scale[4] = "Eb"; scale[5] = "Fb"; scale[6] = "Gb";
      return true;
    } else if (tonic.equalsIgnoreCase("A")) {
      scale[0] = "A";  scale[1] = "B";  scale[2] = "C";  scale[3] = "D";  scale[4] = "E";  scale[5] = "F";  scale[6] = "G";
      return true;
    } else if (tonic.equalsIgnoreCase("A#")) {
      scale[0] = "A#"; scale[1] = "C";  scale[2] = "D";  scale[3] = "D#"; scale[4] = "F";  scale[5] = "G";  scale[6] = "A";
      return true;
    } else if (tonic.equalsIgnoreCase("Bb")) {
      scale[0] = "Bb"; scale[1] = "C";  scale[2] = "Db"; scale[3] = "Eb"; scale[4] = "F";  scale[5] = "Gb"; scale[6] = "Ab";
      return true;
    } else if (tonic.equalsIgnoreCase("B")) {
      scale[0] = "B";  scale[1] = "C#"; scale[2] = "D";  scale[3] = "E";  scale[4] = "F#"; scale[5] = "G";  scale[6] = "A";
      return true;
    } else if (tonic.equalsIgnoreCase("Cb")) {
      scale[0] = "Cb"; scale[1] = "Db"; scale[2] = "Ebb"; scale[3] = "Fb"; scale[4] = "Gb"; scale[5] = "Abb"; scale[6] = "Bbb";
      return true;
    }
  }
  return false; // Unrecognized tonic or scale type.
}

// buildChordNew()
// Builds a chord (an array of note names) from the generated scale. The chord is
// built by taking offsets from the scale (given in the complexity array) starting at
// the numeral index, and then the chord is inverted by moving the first 'inversion'
// notes to the end.
//
// Parameters:
//   scaleType        – e.g. "Major" or "Minor"
//   tonic            – e.g. "C", "C#", etc.
//   numeralIndex     – an integer 0–6 representing the chord degree (I = 0, II = 1, …)
//   complexityOffsets – an array of offsets (e.g. {0,2,4})
//   complexityLength  – number of elements in the complexityOffsets array
//   inversion        – inversion number (0 = no inversion)
//   debug            – if true, prints debugging information
//   chordOut         – output array (should have space for at least 6 notes)
//   chordSize        – (output) number of notes in the chord
void buildChordNew(const String &scaleType, const String &tonic, int numeralIndex,
                   const int offsets[], int offsetsLength, int inversion,
                   bool debug, String chordOut[], int &chordSize) {
  String scale[7];
  if (!generateScaleNew(tonic, scaleType, scale)) {
    // If the scale is not found, default to C Major.
    generateScaleNew("C", "Major", scale);
  }
  
  if (debug) {
    Serial.print("Scale Notes: ");
    for (int i = 0; i < 7; i++) {
      Serial.print(scale[i]); Serial.print(" ");
    }
    Serial.println();
  }
  
  // Build the chord notes using the offsets.
  // (Here we simply compute an index into the scale for each offset.
  // For offsets greater than or equal to 7, we add an extra shift.)
  int indices[10];  // Temporary storage (max chord length assumed = 10)
  chordSize = offsetsLength;
  for (int i = 0; i < offsetsLength; i++) {
    int offset = offsets[i];
    // Compute base index plus an additional shift for each full 7 in the offset.
    int baseIndex = numeralIndex + (offset % 7);
    int octaveShift = offset / 7;
    int current_index = (baseIndex + octaveShift) % 7;
    indices[i] = current_index;
    if (debug) {
      Serial.print("Offset "); Serial.print(offset);
      Serial.print(" -> index: "); Serial.println(current_index);
    }
  }
  
  // Fill the chordOut array using the computed indices.
  for (int i = 0; i < chordSize; i++) {
    chordOut[i] = scale[indices[i]];
  }
  
  // Apply inversion: move the first 'inversion' notes to the end.
  inversion = inversion % chordSize;
  if (inversion > 0) {
    String temp[10];
    for (int i = 0; i < chordSize; i++) {
      temp[i] = chordOut[i];
    }
    int pos = 0;
    for (int i = inversion; i < chordSize; i++) {
      chordOut[pos++] = temp[i];
    }
    for (int i = 0; i < inversion; i++) {
      chordOut[pos++] = temp[i];
    }
  }
  
  if (debug) {
    Serial.print("Final Chord: ");
    for (int i = 0; i < chordSize; i++) {
      Serial.print(chordOut[i]); Serial.print(" ");
    }
    Serial.println();
  }
}

// numeralToIndex()
// Converts a Roman numeral (I, II, …, VII) into an index (0–6)
int numeralToIndex(const String &numeral) {
  if (numeral.equalsIgnoreCase("I"))    return 0;
  if (numeral.equalsIgnoreCase("II"))   return 1;
  if (numeral.equalsIgnoreCase("III"))  return 2;
  if (numeral.equalsIgnoreCase("IV"))   return 3;
  if (numeral.equalsIgnoreCase("V"))    return 4;
  if (numeral.equalsIgnoreCase("VI"))   return 5;
  if (numeral.equalsIgnoreCase("VII"))  return 6;
  return -1;
}

// -------------------- MIDI Functions --------------------

void sendMidiNoteOn(uint8_t note, uint8_t velocity = 127) {
  midiEventPacket_t noteOn = {0x09, 0x90, note, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void sendMidiNoteOff(uint8_t note, uint8_t velocity = 0) {
  midiEventPacket_t noteOff = {0x08, 0x80, note, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

void sendChordNotes() {
  // Send Note On for each chord note in the global chord[] array.
  for (int i = 0; i < chordSize; i++) {
    String noteStr = chord[i];
    // Convert the note string to a MIDI note number.
    // We assume C4 = 60.
    int baseNote = 60;
    int semitoneOffset = 0;
    String normNote = normalizeNote(noteStr);
    for (int j = 0; j < 12; j++) {
      if (String(chromaticScale[j]) == normNote) {
        semitoneOffset = j;
        break;
      }
    }
    int midiNote = baseNote + semitoneOffset;
    // (Apply octave shift if needed – currently handled via currentTonic selection)
    sendMidiNoteOn(midiNote);
  }
}

void sendChordNotesOff() {
  // For simplicity, send Note Off for a fixed set of potential notes.
  // (In practice you might track which notes are active.)
  for (int i = 0; i < chordSize; i++) {
    String noteStr = chord[i];
    int baseNote = 60;
    int semitoneOffset = 0;
    String normNote = normalizeNote(noteStr);
    for (int j = 0; j < 12; j++) {
      if (String(chromaticScale[j]) == normNote) {
        semitoneOffset = j;
        break;
      }
    }
    int midiNote = baseNote + semitoneOffset;
    sendMidiNoteOff(midiNote);
  }
}

// -------------------- Hardware Interaction Functions --------------------

// Write LED states to shift registers.
void writeShiftRegister(byte* states) {
  digitalWrite(SR_LATCH_PIN, LOW);
  for (int i = 3; i >= 0; i--) {  // Assuming 4 bytes for up to 32 LEDs
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, states[i]);
  }
  digitalWrite(SR_LATCH_PIN, HIGH);
}

// Read a button state from a multiplexer.
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
  digitalWrite(s0Pin, (buttonIndex & 1) ? HIGH : LOW);
  digitalWrite(s1Pin, (buttonIndex & 2) ? HIGH : LOW);
  digitalWrite(s2Pin, (buttonIndex & 4) ? HIGH : LOW);
  digitalWrite(s3Pin, (buttonIndex & 8) ? HIGH : LOW);
  delayMicroseconds(10);
  return digitalRead(sigPin) == LOW;  // LOW means button pressed
}

// -------------------- Setup and Loop --------------------

void setup() {
  Serial.begin(9600);
  
  // Initialize multiplexer control pins.
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
  
  // Initialize shift register pins.
  pinMode(SR_LATCH_PIN, OUTPUT);
  pinMode(SR_CLOCK_PIN, OUTPUT);
  pinMode(SR_DATA_PIN, OUTPUT);
  
  // Clear shift registers.
  byte initialStates[4] = { 0 };
  writeShiftRegister(initialStates);
}

void loop() {
  byte shiftRegisterStates[4] = { 0 };
  
  // -------------------- Process Buttons --------------------
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int muxIndex = i / 16;       // MUX1 for buttons 0–15, MUX2 for 16–31
    int muxButtonIndex = i % 16;
    
    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);
    
    if (i <= 6) {
      // Chord Buttons (0-6): Momentary behavior.
      buttonStates[i] = isPressed;
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Chord Button ");
        Serial.print(numerals[i]);
        Serial.println(" pressed");
        
        // Turn off any previous chord.
        sendChordNotesOff();
        
        // --- Call the new chord–generation function ---
        // Use the current scale type, tonic, and numeral (0-based: I = 0, II = 1, …).
        // For complexity, use the offsets for currentComplexity (index = currentComplexity - 1).
        buildChordNew(currentScaleType, currentTonic, i,
                      complexityOffsets[currentComplexity - 1],
                      complexitySizes[currentComplexity - 1],
                      currentInversion, true, chord, chordSize);
        
        // Print the generated chord.
        Serial.print("Generated Chord: ");
        for (int c = 0; c < chordSize; c++) {
          Serial.print(chord[c]); Serial.print(" ");
        }
        Serial.println();
        
        // Send MIDI Note On for each note in the chord.
        sendChordNotes();
      }
    }
    else if (i >= 7 && i <= 18) {
      // Note Buttons (7-18): Tonic selection.
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Note Button ");
        Serial.print(chromaticScale[i - 7]);
        Serial.println(" pressed");
        
        // Deactivate previous tonic note if any.
        static int activeNoteIndex = -1;
        if (activeNoteIndex != -1 && activeNoteIndex != i) {
          // Send MIDI Note Off for previous tonic.
          int baseNote = 60;
          int semitoneOffset = 0;
          String prevNoteStr = chromaticScale[activeNoteIndex - 7];
          for (int j = 0; j < 12; j++) {
            if (String(chromaticScale[j]) == prevNoteStr) {
              semitoneOffset = j;
              break;
            }
          }
          int prevMidiNote = baseNote + semitoneOffset;
          sendMidiNoteOff(prevMidiNote);
        }
        // Toggle current note.
        static int activeNoteIndex = i; // Update active note index.
        activeNoteIndex = i;
        currentTonic = String(chromaticScale[i - 7]);
        Serial.print("Tonic changed to: ");
        Serial.println(currentTonic);
        // Send MIDI Note On for the new tonic.
        int baseNote = 60;
        int semitoneOffset = 0;
        for (int j = 0; j < 12; j++) {
          if (String(chromaticScale[j]) == currentTonic) {
            semitoneOffset = j;
            break;
          }
        }
        int midiNote = baseNote + semitoneOffset;
        sendMidiNoteOn(midiNote);
      }
    }
    if (i >= 19 && i <= 19 + 1) { // Buttons 19-20 for scale selection (Major/Minor)
      if (isPressed && !prevButtonStates[i]) {
        if (i == 19) {
          currentScaleType = "Major";
          Serial.println("Scale type set to Major");
        } else if (i == 20) {
          currentScaleType = "Minor";
          Serial.println("Scale type set to Minor");
        }
        // Ensure only the selected scale button is active.
        for (int k = 19; k <= 20; k++) {
          buttonStates[k] = (k == i);
        }
      }
    }
    
    prevButtonStates[i] = isPressed;
    
    // Update LED states (each bit in the corresponding byte)
    if (buttonStates[i]) {
      shiftRegisterStates[i / 8] |= (1 << (i % 8));
    } else {
      shiftRegisterStates[i / 8] &= ~(1 << (i % 8));
    }
  }
  
  // Write LED states via shift registers.
  writeShiftRegister(shiftRegisterStates);
  delay(50); // Debounce delay
  
  MidiUSB.flush();
}
