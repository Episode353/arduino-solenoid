#include <MIDIUSB.h>

// these are the arduino pins that the solenoids are hooked up to
enum drumPins {kickPin = 0, snarePin = 1, hhPin = 2, crashPin = 3, cowbellPin = 4 , openhatPin = 5};

// these are the midi notes that each solenoid triggers on, as well as an alternate for each
enum midiNotes {kickMidi = 36, snareMidi = 38, hhMidi = 42, crashMidi = 49, cowbellMidi = 39, openhatMidi = 46};
enum midiNoteAlts {kickMidiAlt = 44, snareMidiAlt = 48, hhMidiAlt = 45, crashMidiAlt = 149, cowbellMidiAlt = 47, openhatMidiAlt = 146};

void setup() {
  // the serial port is just used as a monitor for debugging
  // it is not needed for midi
  Serial.begin(115200);

  // setup all output pins
  for(int i=0; i<=8; i++) {
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
  // it is possible to use the actual midi velocity here, just be sure to
  // double to value because midi is 0-127
  // and then change digitalWrite to analogWrite
  if(velocity > 0) {
    velocity = HIGH;
  }

  switch (pitch) {
    case kickMidi:
    case kickMidiAlt:
      Serial.print("Kick: ");
      digitalWrite(kickPin, velocity);
      break;
    case snareMidi:
    case snareMidiAlt:
      Serial.print("Snare: ");
      digitalWrite(snarePin, velocity);
      break;
    case hhMidi:
    case hhMidiAlt:
      Serial.print("HH: ");
      digitalWrite(hhPin, velocity);
      break;
    case crashMidi:
    case crashMidiAlt:
      Serial.print("Crash: ");
      digitalWrite(crashPin, velocity);
      break;
    case cowbellMidi:
    case cowbellMidiAlt:
      Serial.print("Cowbell: ");
      digitalWrite(cowbellPin, velocity);
      break;
    case openhatMidi:
    case openhatMidiAlt:
      Serial.print("Open hat: ");
      digitalWrite(openhatPin, velocity);
      break;
    default:
      // print the midi note value, handy for adding new notes
      Serial.print("Note(");
      Serial.print(pitch);
      Serial.print("): ");
      break;
  }

  if(velocity == 0) {
    Serial.println("off");
  } else {
    Serial.println("on");
  }
}