#include <MIDIUSB.h>

// these are the arduino pins that the solenoids are hooked up to
enum drumPins {kickPin = 3, snarePin = 5, hhPin = 6, crashPin = 9, cowbellPin = 10, openhatPin = 11};

// these are the midi notes that each solenoid triggers on
enum midiNotes {kickMidi = 36, snareMidi = 38, hhMidi = 42, crashMidi = 49, cowbellMidi = 39, openhatMidi = 46};

void setup() {
  // the serial port is just used as a monitor for debugging
  // it is not needed for midi
  Serial.begin(115200);

  // setup all output pins
  for(int i=3; i<=11; i++) {
    pinMode(i, OUTPUT);
  }
}

void loop() {
  //listen for new MIDI messages
  midiEventPacket_t rx = MidiUSB.read();
  processMidi(rx);
}

void processMidi(midiEventPacket_t rx) {
  switch (rx.header) {
    case 0x0:
      // do nothing
      break;

    // note on
    case 0x9:
      handleNoteOn(rx.byte1 & 0xF, rx.byte2, rx.byte3);
      break;
    
    // note off
    case 0x8:
      handleNoteOn(rx.byte1 & 0xF, rx.byte2, 0);
      break;

    // control change
    case 11:
      Serial.print("CC: ");
      Serial.print(rx.byte2);
      Serial.print(":");
      Serial.print(rx.byte3);
      Serial.print("\n");
      break;

    default:
      Serial.println(rx.header);
      break;
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity > 0) {
    // Map the MIDI velocity (0-127) to the PWM range (0-255)
    // I set the pwm range minimum to 160
    // This is because below this value the smaller solenod wont actuate
    velocity = map(velocity, 0, 127, 160, 255);
  }

  switch (pitch) {
    case kickMidi:
      Serial.print("Kick: ");
      analogWrite(kickPin, velocity);  // PWM control
      break;
    case snareMidi:
      Serial.print("Snare: ");
      analogWrite(snarePin, velocity);  // PWM control
      break;
    case hhMidi:
      Serial.print("HH: ");
      analogWrite(hhPin, velocity);  // PWM control
      break;
    case crashMidi:
      Serial.print("Crash: ");
      analogWrite(crashPin, velocity);  // PWM control
      break;
    case cowbellMidi:
      Serial.print("Cowbell: ");
      analogWrite(cowbellPin, velocity);  // PWM control
      break;
    case openhatMidi:
      Serial.print("Open hat: ");
      analogWrite(openhatPin, velocity);  // PWM control
      break;
    default:
      // print the midi note value, handy for adding new notes
      Serial.print("Note(");
      Serial.print(pitch);
      Serial.print("): ");
      break;
  }

  if (velocity == 0) {
    Serial.println("off");
  } else {
    Serial.println("on");
  }
}
