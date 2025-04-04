/*
 * USB MIDI to VS1053B Bridge for RP2040 with explicit USB descriptor
 */

#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// USB MIDI object with explicit descriptor
Adafruit_USBD_MIDI usb_midi(1);  // 1 = number of cables

// Create a MIDI interface using the USB MIDI object
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// VS1053 pin definitions
#define VS1053_CS      A0   // GPIO pin for VS1053 Chip Select
#define VS1053_RESET   A1   // GPIO pin for VS1053 Reset
#define VS1053_DREQ    A2   // GPIO pin for VS1053 Data Request
#define VS1053_DCS     A3   // Data Chip Select (for MIDI and audio data)

void setup() {
  Serial.begin(115200);
  
  // Ensure TinyUSB MIDI is enabled before calling begin
  TinyUSBDevice.setID(0x239A, 0x811E);  // Adafruit's MIDI-compliant PID
  TinyUSBDevice.setManufacturerDescriptor("Adafruit");
  TinyUSBDevice.setProductDescriptor("Feather RP2040 MIDI");

  usb_midi.begin();  // Ensure USB MIDI is started before TinyUSB

  // Start TinyUSB
  if (!TinyUSBDevice.begin()) {
    Serial.println("Failed to initialize TinyUSB!");
    while (1) delay(1);  // Halt if USB initialization fails
  }

  // Wait for the USB device to mount
  while (!TinyUSBDevice.mounted()) delay(10);

  // Initialize MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Set up MIDI callbacks
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);

  Serial.println("Feather RP2040 MIDI Device Ready!");
}

void loop() {
  // Read incoming MIDI messages
  MIDI.read();
}

// MIDI callback functions
void handleNoteOn(byte channel, byte note, byte velocity) {
  Serial.printf("Note On: ch=%d, note=%d, velocity=%d\n", channel, note, velocity);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  Serial.printf("Note Off: ch=%d, note=%d, velocity=%d\n", channel, note, velocity);
}

void handlePitchBend(byte channel, int bend) {
  Serial.printf("Pitch Bend: ch=%d, bend=%d\n", channel, bend);
}

void handleControlChange(byte channel, byte number, byte value) {
  Serial.printf("Control Change: ch=%d, number=%d, value=%d\n", channel, number, value);
}

void handleProgramChange(byte channel, byte number) {
  Serial.printf("Program Change: ch=%d, number=%d\n", channel, number);
}
