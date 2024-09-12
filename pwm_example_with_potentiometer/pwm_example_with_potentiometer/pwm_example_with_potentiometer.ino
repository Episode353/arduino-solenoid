#include <MIDIUSB.h>

// Define the drum pins
const int kickPin = 3;
const int snarePin = 4;
const int hhPin = 6;
const int crashPin = 9;

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

void setup() {
  Serial.begin(115200);

  // Setup drum pins as outputs
  pinMode(kickPin, OUTPUT);
  pinMode(snarePin, OUTPUT);
  pinMode(hhPin, OUTPUT);
  pinMode(crashPin, OUTPUT);
}

void loop() {
  // Continuously read potentiometer values
  updatePotentiometers();

  // Listen for new MIDI messages
  midiEventPacket_t rx = MidiUSB.read();
  processMidi(rx);
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

  // Print whether the note is on or off
  if (velocity == 0) {
    Serial.println("off");
  } else {
    Serial.println("on");
  }
}
