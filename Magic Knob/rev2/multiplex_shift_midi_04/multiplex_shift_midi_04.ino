#include <MIDIUSB.h>

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

unsigned long lastNoteChangeTime = 0;  // Timer for note cycling
int bpm = 120;  // Default BPM
int max_bpm = 500;  // Default BPM
int min_bpm = 30;  // Default BPM
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

void loop() {
  readMuxButtons();

  // Update the LED states
  for (int i = 0; i < 16; i++) {
    regWrite(i, buttonStates[i] ? HIGH : LOW);
  }

  // Build the active note list for MIDI (buttons 0-11 only)
  updateActiveNotes();

  // Handle note cycling when function2 is active
  if (function2State) {
    cycleNotes();
    adjustBPM();
  } else {
    // Read the encoder and send MIDI note if it changes position
    EncoderIntoNotes();
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


void EncoderIntoNotes() {
  int encoderState = digitalRead(encoderPinA);
  
  if (encoderState != lastEncoderState && encoderState == LOW) {
    if (digitalRead(encoderPinB) == HIGH) {
      // Encoder turned right
      currentNoteIndex++;
      if (currentNoteIndex >= activeNotesCount) {
        // If it exceeds the active notes, increase the octave
        if (function1State) {  // Only shift octave if Function 1 is ON
          octaveShift++;
        }
        currentNoteIndex = 0;  // Reset to start of the scale
      }
    } else {
      // Encoder turned left
      currentNoteIndex--;
      if (currentNoteIndex < 0) {
        // If it goes below the active notes, decrease the octave
        if (function1State) {  // Only shift octave if Function 1 is ON
          octaveShift--;
        }
        currentNoteIndex = activeNotesCount - 1;  // Reset to last note of the scale
      }
    }
    sendMidiNote();
  }
  
  lastEncoderState = encoderState;
}

void sendMidiNote() {
  int note = activeNotes[currentNoteIndex];
  if (function1State) {
    note += octaveShift * 12;  // Adjust note based on octave shift if Function 1 is ON
  }
  
  // Send Note On
  midiEventPacket_t noteOn = {0x09, 0x90, note, 127}; // 127 = full velocity
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();

  delay(100); // Short delay to allow note sustain (adjust as needed)

  // Send Note Off
  midiEventPacket_t noteOff = {0x08, 0x80, note, 0};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

// Cycle notes at the set BPM
void cycleNotes() {
  cycleInterval = 6000 / bpm;  // Calculate the interval based on BPM

  if (millis() - lastNoteChangeTime >= cycleInterval) {
    sendMidiNote();  // Send current note
    currentNoteIndex = (currentNoteIndex + 1) % activeNotesCount;  // Move to the next note
    lastNoteChangeTime = millis();  // Update the last note change time
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
