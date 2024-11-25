import time
import audiocore
import audiopwmio
import board
import digitalio

# GPIO Pin Definitions
BUTTON_PIN = board.GP1
LED_PIN = board.GP2
FLASHLIGHT_LED_PIN = board.GP3

# Mode Definitions
FX_MODE = 1
FLASHLIGHT_MODE = 2

# Button Press Timings
SHORT_PRESS_THRESHOLD = 0.3  # Time in seconds for a short press
BUTTON_TIMEOUT = 0.3  # Maximum time between presses to count as consecutive

def initialize_button():
    button = digitalio.DigitalInOut(BUTTON_PIN)
    button.switch_to_input(pull=digitalio.Pull.UP)
    return button

def initialize_led(pin):
    led = digitalio.DigitalInOut(pin)
    led.switch_to_output(value=False)
    return led

def initialize_audio():
    return audiopwmio.PWMAudioOut(board.GP0)  # Replace with a valid PWM pin

def main():
    try:
        print("Starting multi-mode LED control script...")

        # Initialize components
        button = initialize_button()
        fx_led = initialize_led(LED_PIN)
        flashlight_led = initialize_led(FLASHLIGHT_LED_PIN)
        audio = initialize_audio()

        # Open the WAV file
        with open("sonic.wav", "rb") as f:
            wav = audiocore.WaveFile(f)

            # Initialize variables
            current_mode = FX_MODE  # Boot into FX Mode by default
            print("Booting into FX Mode by default")  # Optional: For debugging
            button_press_count = 0
            last_press_time = 0
            previous_button_value = button.value
            press_start_time = None  # Initialize press start time

            # Ensure FX Mode is properly initialized
            flashlight_led.value = False
            fx_led.value = False  # Turn off FX LED initially
            audio.stop()  # Ensure audio is off at startup

            while True:
                current_button_value = button.value

                # Edge detection
                if previous_button_value != current_button_value:
                    if not current_button_value:  # Button pressed (logic low)
                        press_start_time = time.monotonic()
                    else:  # Button released
                        if press_start_time is not None:
                            press_duration = time.monotonic() - press_start_time
                            if press_duration < SHORT_PRESS_THRESHOLD:
                                button_press_count += 1
                                last_press_time = time.monotonic()
                            press_start_time = None  # Reset press start time

                # Handle mode change
                if button_press_count > 0 and (time.monotonic() - last_press_time > BUTTON_TIMEOUT):
                    if button_press_count == 2:
                        current_mode = FX_MODE
                        print("Entering FX Mode")
                        flashlight_led.value = False
                        fx_led.value = False  # Turn off FX LED initially
                        audio.stop()  # Ensure audio is off at mode entry
                    elif button_press_count == 3:
                        current_mode = FLASHLIGHT_MODE
                        print("Entering Flashlight Mode")
                        fx_led.value = False
                        audio.stop()
                        flashlight_led.value = True
                    button_press_count = 0  # Reset count after mode change

                # Handle FX_MODE operation
                if current_mode == FX_MODE:
                    if not button.value:  # Button is pressed
                        fx_led.value = True
                        if not audio.playing:
                            audio.play(wav, loop=True)
                    else:  # Button is released
                        fx_led.value = False
                        if audio.playing:
                            audio.stop()
                elif current_mode == FLASHLIGHT_MODE:
                    flashlight_led.value = True  # Keep flashlight LED on
                else:
                    # Ensure everything is off in an undefined state
                    fx_led.value = False
                    flashlight_led.value = False
                    if audio.playing:
                        audio.stop()

                previous_button_value = current_button_value
                time.sleep(0.01)  # Small delay to reduce CPU usage

    except Exception as e:
        print(f"Error: {e}")
    finally:
        print("Deinitializing...")
        if audio:
            audio.deinit()
        fx_led.value = False
        flashlight_led.value = False

if __name__ == "__main__":
    main()
