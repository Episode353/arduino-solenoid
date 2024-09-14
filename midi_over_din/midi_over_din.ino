#include <MIDI.h>

// Create a MIDI interface instance for serial communication
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);

// These are the Arduino pins that the solenoids are hooked up to
enum drumPins {kickPin = 3, snarePin = 5, hhPin = 6, crashPin = 9, cowbellPin = 10, openhatPin = 11};

// These are the MIDI notes that each solenoid triggers on
enum midiNotes {kickMidi = 36, snareMidi = 38, hhMidi = 42, crashMidi = 49, cowbellMidi = 39, openhatMidi = 46};

void setup() {
  // Initialize the Serial Monitor for debugging
  Serial.begin(115200);

  // Initialize MIDI communication (standard MIDI baud rate is 31250)
  MIDI.begin(MIDI_CHANNEL_OMNI); // Listen to all channels

  // Set all solenoid pins to output
  for (int i = 3; i <= 11; i++) {
    pinMode(i, OUTPUT);
  }

  Serial.println("MIDI to Solenoid Controller Initialized");
}

void loop() {
  // Check for new MIDI messages
  if (MIDI.read()) {
    handleMidiMessage();
  }
}

void handleMidiMessage() {
  byte type = MIDI.getType();
  byte pitch = MIDI.getData1();
  byte velocity = MIDI.getData2();

  switch (type) {
    case midi::NoteOn:
      handleNoteOn(pitch, velocity);
      break;
    case midi::NoteOff:
      handleNoteOn(pitch, 0);  // Treat NoteOff as velocity = 0
      break;
    case midi::ControlChange:
      Serial.print("CC: ");
      Serial.print(MIDI.getData1());
      Serial.print(":");
      Serial.println(MIDI.getData2());
      break;
    default:
      Serial.println("Other MIDI message received");
      break;
  }
}

void handleNoteOn(byte pitch, byte velocity) {
  if (velocity > 0) {
    velocity = map(velocity, 0, 127, 160, 255);  // Map MIDI velocity to PWM range
  }

  switch (pitch) {
    case kickMidi:
      Serial.print("Kick: ");
      analogWrite(kickPin, velocity);
      break;
    case snareMidi:
      Serial.print("Snare: ");
      analogWrite(snarePin, velocity);
      break;
    case hhMidi:
      Serial.print("Hi-Hat: ");
      analogWrite(hhPin, velocity);
      break;
    case crashMidi:
      Serial.print("Crash: ");
      analogWrite(crashPin, velocity);
      break;
    case cowbellMidi:
      Serial.print("Cowbell: ");
      analogWrite(cowbellPin, velocity);
      break;
    case openhatMidi:
      Serial.print("Open Hi-Hat: ");
      analogWrite(openhatPin, velocity);
      break;
    default:
      Serial.print("Unhandled Note: ");
      Serial.println(pitch);
      break;
  }

  if (velocity == 0) {
    Serial.println("Note off");
  } else {
    Serial.println("Note on");
  }
}
