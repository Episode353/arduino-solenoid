#include <SPI.h>
#include <MIDIUSB.h>

// VS1053 pin definitions
#define VS1053_CS      4    // GPIO pin for VS1053 Chip Select
#define VS1053_RESET   2    // GPIO pin for VS1053 Reset
#define VS1053_DREQ    3    // GPIO pin for VS1053 Data Request

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for Serial to be ready
  Serial.println("Initializing VS1053 MIDI Synth");

  // Initialize SPI for VS1053 communication
  SPI.begin();
  Serial.println("SPI Initialized");

  // Initialize VS1053 pins
  pinMode(VS1053_CS, OUTPUT);
  pinMode(VS1053_RESET, OUTPUT);
  pinMode(VS1053_DREQ, INPUT);
  Serial.println("VS1053 Pins Set");

  // Reset VS1053
  digitalWrite(VS1053_RESET, LOW);
  delay(10);
  digitalWrite(VS1053_RESET, HIGH);
  delay(10);
  Serial.println("VS1053 Reset Complete");

  // Additional initialization steps can be added here
}

void loop() {
  // Send a 'Note On' message for Middle C (MIDI note 60) with velocity 100
  sendNoteOn(0, 60, 100);
  delay(500); // Hold the note for 500ms

  // Send a 'Note Off' message for Middle C
  sendNoteOff(0, 60, 0);
  delay(500); // Wait before sending the next note
}

// Function to send a 'Note On' message
void sendNoteOn(byte channel, byte note, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | (channel & 0x0F), note, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
  Serial.print("Note On: Channel ");
  Serial.print(channel);
  Serial.print(", Note ");
  Serial.print(note);
  Serial.print(", Velocity ");
  Serial.println(velocity);
}

// Function to send a 'Note Off' message
void sendNoteOff(byte channel, byte note, byte velocity) {
  midiEventPacket_t noteOff = {0x09, 0x80 | (channel & 0x0F), note, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
  Serial.print("Note Off: Channel ");
  Serial.print(channel);
  Serial.print(", Note ");
  Serial.print(note);
  Serial.print(", Velocity ");
  Serial.println(velocity);
}
