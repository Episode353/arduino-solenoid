#include <MIDIUSB.h>

// Define the drum pins
const int kickPin = 3;
const int snarePin = 4;
const int hhPin = 6;
const int crashPin = 9;

// Define the Buzzer Pin
const int buzzPin = 5;


// Define the analog input pins for potentiometers
const int kickPot = A0;
const int snarePot = A1;
const int hhPot = A2;
const int crashPot = A3;

// Define the MIDI notes for each drum
const int kickMidi = 36;
const int snareMidi = 38;
const int hhMidi = 42;
const int crashMidi = 49;

// Variables to store potentiometer values
float kickScale, snareScale, hhScale, crashScale;

// Variables to track note timing
unsigned long noteOnTimes[128]; // Stores the time each note was turned on
const unsigned long NOTE_DURATION = 10;

void play_jingle(){
  tone(buzzPin, 277.18, 200); // C#4
  delay(200);
  tone(buzzPin, 311.13, 400); // D#4
  delay(300);
  tone(buzzPin, 369.99, 300); // F#4
}

void setup() {
  // Play a little jingle
  play_jingle();


  // Begin the Serial Communication
  Serial.begin(115200);

  // Setup drum pins as outputs
  pinMode(kickPin, OUTPUT);
  pinMode(snarePin, OUTPUT);
  pinMode(hhPin, OUTPUT);
  pinMode(crashPin, OUTPUT);


  // Initialize noteOnTimes array
  for (int i = 0; i < 128; i++) {
    noteOnTimes[i] = 0;
  }
}

void loop() {
  // Continuously read potentiometer values
  updatePotentiometers();

  // Listen for new MIDI messages
  midiEventPacket_t rx = MidiUSB.read();
  processMidi(rx);

  // Check if any notes should be turned off manually after 100ms
  checkNoteTimeouts();
}

// Function to continuously update potentiometer values
void updatePotentiometers() {
  // Map potentiometer readings (0-1023) to a scale of 0.5 - 1.0
  kickScale = map(analogRead(kickPot), 0, 1023, 50, 100) / 100.0;
  snareScale = map(analogRead(snarePot), 0, 1023, 50, 100) / 100.0;
  hhScale = map(analogRead(hhPot), 0, 1023, 50, 100) / 100.0;
  crashScale = map(analogRead(crashPot), 0, 1023, 50, 100) / 100.0;
}

void processMidi(midiEventPacket_t rx) {
  switch (rx.header) {
    case 0x0:
      // Do nothing
      break;

    // Note on
    case 0x9:
      handleNoteOn(rx.byte1 & 0xF, rx.byte2, rx.byte3);
      break;

    // Note off
    case 0x8:
      handleNoteOn(rx.byte1 & 0xF, rx.byte2, 0);
      break;

    default:
      Serial.println(rx.header);
      break;
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  // Map the MIDI velocity (0-127) to the PWM range (160-255)
  if (velocity > 0) {
    velocity = map(velocity, 0, 127, 160, 255);
  }

  switch (pitch) {
    case kickMidi:
      velocity *= kickScale; // Scale velocity by potentiometer value
      Serial.print("Kick: ");
      analogWrite(kickPin, velocity);
      break;

    case snareMidi:
      velocity *= snareScale;
      Serial.print("Snare: ");
      analogWrite(snarePin, velocity);
      break;

    case hhMidi:
      velocity *= hhScale;
      Serial.print("Hi-hat: ");
      analogWrite(hhPin, velocity);
      break;

    case crashMidi:
      velocity *= crashScale;
      Serial.print("Crash: ");
      analogWrite(crashPin, velocity);
      break;

    default:
      Serial.print("Unknown note: ");
      Serial.println(pitch);
      break;
  }

  // If the velocity is greater than 0, the note is being turned on, so store the timestamp
  if (velocity > 0) {
    noteOnTimes[pitch] = millis(); // Store the time the note was turned on
    Serial.println("on");
  } else {
    analogWrite(pitchToPin(pitch), 0); // Manually turn off the note
    noteOnTimes[pitch] = 0; // Reset the note on time
    Serial.println("off");
  }
}

// Function to check if any notes have been on for more than 100ms and turn them off
void checkNoteTimeouts() {
  unsigned long currentTime = millis();
  for (int pitch = 0; pitch < 128; pitch++) {
    if (noteOnTimes[pitch] > 0 && (currentTime - noteOnTimes[pitch] > NOTE_DURATION)) {
      // Turn off the note
      analogWrite(pitchToPin(pitch), 0);
      noteOnTimes[pitch] = 0; // Reset the note on time
      Serial.print("Manually turning off note: ");
      Serial.println(pitch);
    }
  }
}

// Helper function to map MIDI pitch to drum pins
int pitchToPin(byte pitch) {
  switch (pitch) {
    case kickMidi:
      return kickPin;
    case snareMidi:
      return snarePin;
    case hhMidi:
      return hhPin;
    case crashMidi:
      return crashPin;
    default:
      return -1; // Unknown pitch
  }
}
