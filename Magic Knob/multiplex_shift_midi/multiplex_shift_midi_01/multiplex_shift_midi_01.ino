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
const uint8_t noteMap[12] = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}; // MIDI notes C4-B4
int activeNotes[12];
int activeNotesCount = 0;

const int encoderPinA = 10;
const int encoderPinB = 16;
int lastEncoderState = HIGH;

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

  // Read the encoder and send MIDI note if it changes position
  handleEncoder();
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

void handleEncoder() {
  int encoderState = digitalRead(encoderPinA);
  
  if (encoderState != lastEncoderState && encoderState == LOW) {
    if (digitalRead(encoderPinB) == HIGH) {
      // Encoder turned right
      currentNoteIndex = (currentNoteIndex + 1) % activeNotesCount;
    } else {
      // Encoder turned left
      currentNoteIndex = (currentNoteIndex - 1 + activeNotesCount) % activeNotesCount;
    }
    sendMidiNote();
  }
  
  lastEncoderState = encoderState;
}

void sendMidiNote() {
  int note = activeNotes[currentNoteIndex];
  
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
