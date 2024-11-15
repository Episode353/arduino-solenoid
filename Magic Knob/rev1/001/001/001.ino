#define P_SERIAL 5   // Pin connected to DS of 74HC595 (data)
#define P_LATCH  4   // Pin connected to ST_CP of 74HC595 (latch)
#define P_CLOCK  3   // Pin connected to SH_CP of 74HC595 (clock)

void setup() {
  // Set pin modes for the shift register
  pinMode(P_SERIAL, OUTPUT);
  pinMode(P_LATCH, OUTPUT);
  pinMode(P_CLOCK, OUTPUT);
  
  // Initialize serial communication for debugging
  Serial.begin(9600);
  Serial.println("Setup complete.");
}
void loop() {
  // Count from 0 to 15 (for 16 LEDs)
  for (int numberToDisplay = 0; numberToDisplay < 256; numberToDisplay++) {
    // Reverse bits
    uint8_t reversed = 0;
    for (int i = 0; i < 8; i++) {
      reversed |= ((numberToDisplay >> i) & 1) << (7 - i);
    }

    // Take the latchPin low so the LEDs don't change while you're sending in bits
    digitalWrite(P_LATCH, LOW);
    
    // Shift out the reversed bits
    shiftOut(P_SERIAL, P_CLOCK, MSBFIRST, reversed);
    
    // Take the latch pin high so the LEDs will light up
    digitalWrite(P_LATCH, HIGH);
    
    // Print which LEDs are on
    Serial.print("Displaying: ");
    Serial.println(reversed, BIN); // Print the number in binary
    
    // Pause before the next value
    delay(500);
  }
}



