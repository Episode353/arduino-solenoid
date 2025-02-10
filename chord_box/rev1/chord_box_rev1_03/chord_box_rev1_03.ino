// This code is aimed to be an analogue of an omnichord, where when you press the one of the Seven Chord buttons, a chord is played
// The Chord buttons are mommentary buttons and the chord should only play when the chord is being pressed and when the button is realsed it should stop playing


// 01 This peice of code of mearly just a demonstration of the
// leds and lights, with no actual midi being sent or recieved.
// the arudino simply confirms a button press with the led being light up and the
// output in the console

// 02 Made the Chord buttons I-Vii (buttons 0-6) have a Mommentary press
// Meaning they do not hold or "switch"
// the led turns on and off directly with the button
// Specified the Button mapping for easy coding

// 03 incorporated the python script in, after testing the hardware


// Button Mapping
// Button 0 = Chord I
// Button 1 = Chord II
// Button 2 = Chord III
// Button 3 = Chord VI
// Button 4 = Chord V
// Button 5 = Chord VI
// Button 6 = Chord VII
// Button 7 = Note C
// Button 8 = Note C#
// Button 9 = Note D
// Button 10 = Note D#
// Button 11 = Note E
// Button 12 = Note F
// Button 13 = Note F#
// Button 14 = Note G
// Button 15 = Note G#
// Button 16 = Note A
// Button 17 = Note A#
// Button 18 = Note B
// Button 19 = Major Scale
// Button 20 = Minor Scale
// Button 21 = Dorian Scale
// Button 22 = Phrygian Scale
// Button 23 = Lydian Scale
// Button 24 = Mixolydian Scale
// Button 25 = Locrian Scale
// Button 26 = Harmonic Scale
// Button 27 = Latch (Currently not used, do not use in the code)
// Button 28 = Bass (Currently not used, do not use in the code)
// Button 29 = Chromatic (Currently not used, do not use in the code)
// Button 30 = Random (Currently not used, do not use in the code)

// Button Mapping
// Button 0 = Chord I
// Button 1 = Chord II
// Button 2 = Chord III
// Button 3 = Chord IV
// Button 4 = Chord V
// Button 5 = Chord VI
// Button 6 = Chord VII
// Button 7 = Note C
// Button 8 = Note C#
// Button 9 = Note D
// Button 10 = Note D#
// Button 11 = Note E
// Button 12 = Note F
// Button 13 = Note F#
// Button 14 = Note G
// Button 15 = Note G#
// Button 16 = Note A
// Button 17 = Note A#
// Button 18 = Note B
// Button 19 = Major Scale

// Pin Definitions for Multiplexer 1
#define MUX1_SIG_PIN A3
#define MUX1_S0_PIN 2
#define MUX1_S1_PIN 3
#define MUX1_S2_PIN 4
#define MUX1_S3_PIN 5

// Pin Definitions for Multiplexer 2
#define MUX2_SIG_PIN A2
#define MUX2_S0_PIN 12
#define MUX2_S1_PIN 11
#define MUX2_S2_PIN 10
#define MUX2_S3_PIN 9

// Pin Definitions for Shift Registers
#define SR_LATCH_PIN 8
#define SR_CLOCK_PIN 7
#define SR_DATA_PIN 6

// Number of buttons
const int NUM_BUTTONS = 32;

// State Arrays
bool buttonStates[NUM_BUTTONS] = { false };
bool prevButtonStates[NUM_BUTTONS] = { false };

// Scale Data
const char* SCALES[7][12][7] = {
  // Major Scale
  {
    { "C", "D", "E", "F", "G", "A", "B" },
    { "C#", "D#", "E#", "F#", "G#", "A#", "B#" },
    { "D", "E", "F#", "G", "A", "B", "C#" },
    { "D#", "E#", "F##", "G#", "A#", "B#", "C##" },
    { "E", "F#", "G#", "A", "B", "C#", "D#" },
    { "F", "G", "A", "Bb", "C", "D", "E" },
    { "F#", "G#", "A#", "B", "C#", "D#", "E#" },
    { "G", "A", "B", "C", "D", "E", "F#" },
    { "G#", "A#", "B#", "C#", "D#", "E#", "F##" },
    { "A", "B", "C#", "D", "E", "F#", "G#" },
    { "A#", "B#", "C##", "D#", "E#", "F##", "G##" },
    { "B", "C#", "D#", "E", "F#", "G#", "A#" } },
  // Minor Scale
  {
    { "C", "D", "Eb", "F", "G", "Ab", "Bb" },
    { "C#", "D#", "E", "F#", "G#", "A", "B" },
    { "D", "E", "F", "G", "A", "Bb", "C" },
    { "D#", "E#", "F#", "G#", "A#", "B", "C#" },
    { "E", "F#", "G", "A", "B", "C", "D" },
    { "F", "G", "Ab", "Bb", "C", "Db", "Eb" },
    { "F#", "G#", "A", "B", "C#", "D", "E" },
    { "G", "A", "Bb", "C", "D", "Eb", "F" },
    { "G#", "A#", "B", "C#", "D#", "E", "F#" },
    { "A", "B", "C", "D", "E", "F", "G" },
    { "A#", "B#", "C#", "D#", "E#", "F#", "G#" },
    { "B", "C#", "D", "E", "F#", "G", "A" } },
  // Dorian, Phrygian, Lydian, Mixolydian, Locrian, Harmonic Minor (Add full scale data similarly)
};

// Function to write to shift registers
void writeShiftRegister(byte* states) {
  digitalWrite(SR_LATCH_PIN, LOW);
  for (int i = 3; i >= 0; i--) {  // Four shift registers
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, states[i]);
  }
  digitalWrite(SR_LATCH_PIN, HIGH);
}

// Function to read a button from a multiplexer
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

// Function to build a chord from a specific scale
void playChord(int scaleIndex, int rootIndex, int numeralIndex) {
  Serial.print("Chord Notes: ");
  for (int i = 0; i < 3; i++) {
    int noteIndex = (numeralIndex + chordOffsets[i]) % 7;
    Serial.print(SCALES[scaleIndex][rootIndex][noteIndex]);
    if (i < 2) Serial.print(", ");
  }
  Serial.println();
}

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

  // Clear shift register
  byte initialStates[4] = { 0 };
  writeShiftRegister(initialStates);
}

void loop() {
  byte shiftRegisterStates[4] = { 0 };

  for (int i = 0; i < NUM_BUTTONS; i++) {
    int muxIndex = i / 16;  // 0 for MUX1, 1 for MUX2
    int muxButtonIndex = i % 16;

    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);
    if (i <= 6) {
      // Chord Buttons (0-6): Momentary behavior
      buttonStates[i] = isPressed;
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Button ");
        Serial.print(i);
        Serial.println(" pressed");
        playChord(0, 0, i);  // Default to Major scale (index 0), root C (index 0)
      }
    } else {
      // Other buttons: Toggle behavior
      if (isPressed && !prevButtonStates[i]) {
        Serial.print("Button ");
        Serial.print(i);
        Serial.println(" pressed");
        buttonStates[i] = !buttonStates[i];
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

  writeShiftRegister(shiftRegisterStates);
  delay(50);  // Debounce delay
}
