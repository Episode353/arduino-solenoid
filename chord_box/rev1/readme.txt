/*
  Arduino C++ Code for Omnichord-like Device with MIDI USB Output
  - Implements chromatic scale, enharmonics, major scales, and complexity chords.
  - Interacts with hardware buttons and LEDs via multiplexers and shift registers.
  - Generates and sends MIDI chords based on button presses.
  
  Updates:
  1. Note and scale buttons now light up when pressed.
  2. Only one note or one scale can be active at a time.
  3. Complexity is hard-coded to 1.
  4. Inversion is hard-coded to 1.
  5. Sends MIDI Note On and Note Off messages over USB.
  6. Allows spanning three octaves with octave shift buttons.
*/

// This code is aimed to be an analogue of an omnichord, where when you press the one of the Seven Chord buttons, a chord is played
// The Chord buttons are mommentary buttons and the chord should only play when the chord is being pressed and when the button is realsed it should stop playing


// 01 This peice of code of mearly just a demonstration of the
// leds and lights, with no actual midi being sent or recieved.
// the arudino simply confirms a button press with the led being light up and the
// output in the console

// 02 Made the Chord buttons I-Vii (buttons 0-6) have a Mommentary press
// Meaning they do not hold or "switch"
// the led turns on and off directly with the button
// Specified the Button mapping for easy coding

// 03 incorporated the python script in, after testing the hardware


// Button Mapping
// Button 0 = Chord I
// Button 1 = Chord II
// Button 2 = Chord III
// Button 3 = Chord VI
// Button 4 = Chord V
// Button 5 = Chord VI
// Button 6 = Chord VII
// Button 7 = Note C
// Button 8 = Note C#
// Button 9 = Note D
// Button 10 = Note D#
// Button 11 = Note E
// Button 12 = Note F
// Button 13 = Note F#
// Button 14 = Note G
// Button 15 = Note G#
// Button 16 = Note A
// Button 17 = Note A#
// Button 18 = Note B
// Button 19 = Major Scale
// Button 20 = Minor Scale
// Button 21 = Dorian Scale
// Button 22 = Phrygian Scale
// Button 23 = Lydian Scale
// Button 24 = Mixolydian Scale
// Button 25 = Locrian Scale
// Button 26 = Harmonic Scale
// Button 27 = Latch (Currently not used, do not use in the code)
// Button 28 = Bass (Currently not used, do not use in the code)
// Button 29 = Chromatic (Currently not used, do not use in the code)
// Button 30 = Random (Currently not used, do not use in the code)

// Button Mapping
// Button 0 = Chord I
// Button 1 = Chord II
// Button 2 = Chord III
// Button 3 = Chord IV
// Button 4 = Chord V
// Button 5 = Chord VI
// Button 6 = Chord VII
// Button 7 = Note C
// Button 8 = Note C#
// Button 9 = Note D
// Button 10 = Note D#
// Button 11 = Note E
// Button 12 = Note F
// Button 13 = Note F#
// Button 14 = Note G
// Button 15 = Note G#
// Button 16 = Note A
// Button 17 = Note A#
// Button 18 = Note B
// Button 19 = Major Scale