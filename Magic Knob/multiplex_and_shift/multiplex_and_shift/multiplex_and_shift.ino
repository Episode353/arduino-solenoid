#define clockPin 3 // Clock pin of 74HC595 is connected to Digital pin 10
#define dataPin  5  // Data  pin of 74HC595 is connected to Digital pin 9 
#define latchPin 4  // Latch pin of 74HC595 is connected to Digital pin 8

#define muxSigPin A0  // Multiplexer common input
#define muxS0Pin 6    // MUX control pin S0
#define muxS1Pin 7    // MUX control pin S1
#define muxS2Pin 8    // MUX control pin S2
#define muxS3Pin 9    // MUX control pin S3

int numOfRegisters = 2;
byte* registerState;

long effectId = 0;
long prevEffect = 0;
long effectRepeat = 0;
long effectSpeed = 30;

void setup() {
  // Initialize array
  registerState = new byte[numOfRegisters];
  for (size_t i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }

  // Set pins for shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Set pins for multiplexer control
  pinMode(muxS0Pin, OUTPUT);
  pinMode(muxS1Pin, OUTPUT);
  pinMode(muxS2Pin, OUTPUT);
  pinMode(muxS3Pin, OUTPUT);
  pinMode(muxSigPin, INPUT_PULLUP);  // Use internal pull-up resistor

  // Initialize serial for debugging
  Serial.begin(9600);
}

void loop() {
  int buttonState = readMuxButton();
  if (buttonState != -1) {
    Serial.print("Button ");
    Serial.print(buttonState);
    Serial.println(" pressed");
    regWrite(buttonState, HIGH);  // Activate the LED for the pressed button
    delay(100);  // Debounce delay
    regWrite(buttonState, LOW);   // Deactivate the LED
  }
}


void regWrite(int pin, bool state) {
  int reg = (pin / 8) == 0 ? 1 : 0;  // Now register 0 is for pins 9-16 and 1 for pins 1-8
  int actualPin = pin % 8;  // Determines the pin on the actual register (0-7)
  
  digitalWrite(latchPin, LOW);  // Begin session
  
  // Loop through each shift register and set the corresponding pin
  for (int i = 0; i < numOfRegisters; i++) {
    byte* states = &registerState[i];
    if (i == reg) {
      bitWrite(*states, actualPin, state);  // Update the correct register's pin
    }
    shiftOut(dataPin, clockPin, MSBFIRST, *states);  // Shift out the byte to the shift register
  }

  digitalWrite(latchPin, HIGH);  // End session
}


// Adjusted to map button index to correct LED index
int readMuxButton() {
  for (int i = 0; i < 16; i++) {
    // Select MUX channel (buttons)
    digitalWrite(muxS0Pin, (i & 1) ? HIGH : LOW);
    digitalWrite(muxS1Pin, (i & 2) ? HIGH : LOW);
    digitalWrite(muxS2Pin, (i & 4) ? HIGH : LOW);
    digitalWrite(muxS3Pin, (i & 8) ? HIGH : LOW);

    // Read the signal pin (A0)
    int buttonState = digitalRead(muxSigPin);
    if (buttonState == LOW) {  // Button is pressed (connected to ground)
      return i;  // Return the index of the pressed button (0 to 15)
    }
  }

  return -1;  // No button pressed
}


// Effect functions (same as before)
void effectA(int speed) { /* Same as your previous effectA implementation */ }
void effectB(int speed) { /* Same as your previous effectB implementation */ }
void effectC(int speed) { /* Same as your previous effectC implementation */ }
void effectD(int speed) { /* Same as your previous effectD implementation */ }
void effectE(int speed) { /* Same as your previous effectE implementation */ }
