import time
import audiocore
import audiopwmio
import board
import digitalio

def main():
    try:
        print("Starting audio playback script...")
        
        # Initialize the button
        button = digitalio.DigitalInOut(board.GP1)  # Replace with your button pin
        button.switch_to_input(pull=digitalio.Pull.UP)  # Use pull-up resistor
        
        # Initialize the LED
        led = digitalio.DigitalInOut(board.GP2)  # Replace with your LED pin
        led.switch_to_output(value=False)  # Start with LED off
        
        # Initialize audio
        audio = audiopwmio.PWMAudioOut(board.GP0)  # Replace with a valid PWM pin
        
        # Open the WAV file
        with open("sonic.wav", "rb") as f:
            wav = audiocore.WaveFile(f)
            
            while True:
                if not button.value:  # Button is pressed (logic low)
                    # Light up the LED
                    led.value = True
                    
                    # Start playing audio if not already playing
                    if not audio.playing:
                        print("Button pressed, starting audio...")
                        audio.play(wav, loop=True)
                else:
                    # Turn off the LED
                    led.value = False
                    
                    # Stop audio if it is playing
                    if audio.playing:
                        print("Button released, stopping audio...")
                        audio.stop()
                
                time.sleep(0.05)  # Small delay to reduce CPU usage and debounce
                
    except Exception as e:
        print(f"Error: {e}")
    finally:
        print("Deinitializing...")
        if audio:
            audio.deinit()
        # Ensure LED is turned off on exit
        led.value = False

if __name__ == "__main__":
    main()
