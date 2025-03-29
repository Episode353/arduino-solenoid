// 01 This peice of code of mearly just a demonstration of the
// leds and lights, with no actual midi being sent or recieved.
// the arudino simply confirms a button press with the led being light up and the 
// output in the console



// Pin Definitions for Multiplexer 1
#define MUX1_SIG_PIN A3
#define MUX1_S0_PIN 2
#define MUX1_S1_PIN 3
#define MUX1_S2_PIN 4
#define MUX1_S3_PIN 5

// Pin Definitions for Multiplexer 2
#define MUX2_SIG_PIN A2
#define MUX2_S0_PIN 12
#define MUX2_S1_PIN 11
#define MUX2_S2_PIN 10
#define MUX2_S3_PIN 9

// Pin Definitions for Shift Registers
#define SR_LATCH_PIN 8
#define SR_CLOCK_PIN 7
#define SR_DATA_PIN 6

// Number of buttons
const int NUM_BUTTONS = 32;

// State Arrays
bool buttonStates[NUM_BUTTONS] = {false};
bool prevButtonStates[NUM_BUTTONS] = {false};

// Function to write to shift registers
void writeShiftRegister(byte* states) {
  digitalWrite(SR_LATCH_PIN, LOW);
  for (int i = 3; i >= 0; i--) { // Four shift registers
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, states[i]);
  }
  digitalWrite(SR_LATCH_PIN, HIGH);
}

// Function to read a button from a multiplexer
bool readMuxButton(int muxIndex, int buttonIndex) {
  int sigPin, s0Pin, s1Pin, s2Pin, s3Pin;
  if (muxIndex == 0) {
    sigPin = MUX1_SIG_PIN;
    s0Pin = MUX1_S0_PIN;
    s1Pin = MUX1_S1_PIN;
    s2Pin = MUX1_S2_PIN;
    s3Pin = MUX1_S3_PIN;
  } else {
    sigPin = MUX2_SIG_PIN;
    s0Pin = MUX2_S0_PIN;
    s1Pin = MUX2_S1_PIN;
    s2Pin = MUX2_S2_PIN;
    s3Pin = MUX2_S3_PIN;
  }

  // Set multiplexer address
  digitalWrite(s0Pin, (buttonIndex & 1) ? HIGH : LOW);
  digitalWrite(s1Pin, (buttonIndex & 2) ? HIGH : LOW);
  digitalWrite(s2Pin, (buttonIndex & 4) ? HIGH : LOW);
  digitalWrite(s3Pin, (buttonIndex & 8) ? HIGH : LOW);

  delayMicroseconds(10); // Allow settling time
  return digitalRead(sigPin) == LOW; // Button pressed is LOW
}

void setup() {
  Serial.begin(9600);

  // Initialize multiplexer control pins
  pinMode(MUX1_S0_PIN, OUTPUT);
  pinMode(MUX1_S1_PIN, OUTPUT);
  pinMode(MUX1_S2_PIN, OUTPUT);
  pinMode(MUX1_S3_PIN, OUTPUT);
  pinMode(MUX1_SIG_PIN, INPUT_PULLUP);

  pinMode(MUX2_S0_PIN, OUTPUT);
  pinMode(MUX2_S1_PIN, OUTPUT);
  pinMode(MUX2_S2_PIN, OUTPUT);
  pinMode(MUX2_S3_PIN, OUTPUT);
  pinMode(MUX2_SIG_PIN, INPUT_PULLUP);

  // Initialize shift register control pins
  pinMode(SR_LATCH_PIN, OUTPUT);
  pinMode(SR_CLOCK_PIN, OUTPUT);
  pinMode(SR_DATA_PIN, OUTPUT);

  // Clear shift register
  byte initialStates[4] = {0};
  writeShiftRegister(initialStates);
}

void loop() {
  byte shiftRegisterStates[4] = {0};

  for (int i = 0; i < NUM_BUTTONS; i++) {
    int muxIndex = i / 16; // 0 for MUX1, 1 for MUX2
    int muxButtonIndex = i % 16;

    bool isPressed = readMuxButton(muxIndex, muxButtonIndex);
    if (isPressed && !prevButtonStates[i]) {
      Serial.print("Button ");
      Serial.print(i);
      Serial.println(" pressed");
      buttonStates[i] = !buttonStates[i]; // Toggle button state
    }
    prevButtonStates[i] = isPressed;

    // Update shift register state for LEDs
    if (buttonStates[i]) {
      shiftRegisterStates[i / 8] |= (1 << (i % 8)); // Set bit
    } else {
      shiftRegisterStates[i / 8] &= ~(1 << (i % 8)); // Clear bit
    }
  }

  writeShiftRegister(shiftRegisterStates);
  delay(50); // Debounce delay
}
