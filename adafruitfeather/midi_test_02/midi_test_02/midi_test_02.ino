#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// MIDI settings
const int MIDI_CHANNEL = 0;  // MIDI channels are 0-15 in code (shown as 1-16 in MIDI software)
const int NOTE_ON = 0x90;
const int NOTE_OFF = 0x80;
const int RST_PIN = D5;

// Preset cycling
const int PRESET_COUNT = 127;
int currentPreset = 0;
const byte PRESET_UP_NOTE = 21;    // A-1
const byte PRESET_DOWN_NOTE = 22;  // A#-1


void setup() {
  // Initialize TinyUSB
  usb_midi.begin();
  
  // Wait for USB device to be mounted
  while (!TinyUSBDevice.mounted()) delay(1);
  
  // Initialize Serial1 for hardware MIDI output to module
  Serial1.begin(31250);  // VS1053 MIDI baud rate
  
  // Configure reset pin
  pinMode(RST_PIN, OUTPUT);
  
  // Reset the VS1053
  digitalWrite(RST_PIN, LOW);
  delay(100);  // Hold reset low for 100ms
  digitalWrite(RST_PIN, HIGH);  // Release reset
  delay(100);  // Allow time for the chip to wake up
}

void loop() {
  // Check if MIDI data is available
  if (usb_midi.available()) {
    // Read one byte at a time to build MIDI messages
    uint8_t byte = usb_midi.read();
    
    // MIDI status bytes have the high bit set (0x80-0xFF)
    if (byte & 0x80) {
      // This is a status byte - start of a new message
      uint8_t status = byte;
      uint8_t channel = status & 0x0F;
      uint8_t messageType = status & 0xF0;
      
      // Read data bytes based on message type
      if (messageType == NOTE_ON || messageType == NOTE_OFF) {
        // Note messages have two data bytes (note number and velocity)
        uint8_t note = usb_midi.read();
        uint8_t velocity = usb_midi.read();
        
        // Check for preset control notes
        if (channel == MIDI_CHANNEL) {
          if (note == PRESET_UP_NOTE && messageType == NOTE_ON && velocity > 0) {
            // Move to next preset
            currentPreset = (currentPreset + 1) % PRESET_COUNT;
            sendProgramChange(currentPreset);
            return; // Don't forward this note to the module
          }
          else if (note == PRESET_DOWN_NOTE && messageType == NOTE_ON && velocity > 0) {
            // Move to previous preset
            currentPreset = (currentPreset > 0) ? (currentPreset - 1) : (PRESET_COUNT - 1);
            sendProgramChange(currentPreset);
            return; // Don't forward this note to the module
          }
        }
        
        // Forward normal note messages to the module
        Serial1.write(status);
        Serial1.write(note);
        Serial1.write(velocity);
      }
      else if (messageType == 0xC0) {
        // Program Change has one data byte
        uint8_t program = usb_midi.read();
        Serial1.write(status);
        Serial1.write(program);
      }
      else if (messageType == 0xD0) {
        // Channel Pressure has one data byte
        uint8_t pressure = usb_midi.read();
        Serial1.write(status);
        Serial1.write(pressure);
      }
      else if (messageType == 0xA0 || messageType == 0xB0 || messageType == 0xE0) {
        // Control Change, Poly Aftertouch, and Pitch Bend have two data bytes
        uint8_t data1 = usb_midi.read();
        uint8_t data2 = usb_midi.read();
        Serial1.write(status);
        Serial1.write(data1);
        Serial1.write(data2);
      }
    }
  }
}

// Send Program Change message to switch presets
void sendProgramChange(uint8_t program) {
  // Program Change message (0xC0 | channel)
  Serial1.write(0xC0 | MIDI_CHANNEL);
  Serial1.write(program);
}