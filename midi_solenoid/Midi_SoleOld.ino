#include <frequencyToNote.h>
#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <pitchToFrequency.h>
#include <pitchToNote.h>

// the setup function runs once when you press reset or power the board
void setup() {
 
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(7, OUTPUT);
  Serial.begin(9600); 
}

void loop() {
 
  midiEventPacket_t rx = MidiUSB.read();  //listen for new MIDI messages all channels
 
  switch (rx.header) {
    case 0x9:            //Note On message
      handleNoteOn(
        rx.byte1 & 0xF,  //channel
        rx.byte2,        //pitch
        rx.byte3         //velocity
      );
      break;
    default:
      break;
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {

Serial.print(pitch);
Serial.println();
 
  switch (pitch) {
      case 24:      //solenoid 1 = C1/24
       digitalWrite(12, HIGH);   
       delay(100);            
  digitalWrite(12, LOW);    
  delay(10);           
      break;
      
      case 25:      //solenoid 2 = C#1/25
      digitalWrite(11, HIGH);
  delay(100);              
  digitalWrite(11, LOW);    
  delay(100);           
      break;
    
    case 26:      //solenoid 3 = D1/26
      digitalWrite(10, HIGH);
  delay(100);             
  digitalWrite(10, LOW); 
  delay(100);           
      break;
    
    case 27:      //solenoid 4 = D#1/27
      digitalWrite(9, HIGH);
  delay(100);              
  digitalWrite(9, LOW);    
  delay(100);           
      break;
    
  case 28:      //solenoid 5 = E/28
      digitalWrite(7, HIGH);
  delay(100);              
  digitalWrite(7, LOW);    
  delay(100);           
      break;
    
    default:
    break;
  }
}