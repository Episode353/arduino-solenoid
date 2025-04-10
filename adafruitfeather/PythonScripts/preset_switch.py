#!/usr/bin/env python3
import sys
import serial
import time
import serial.tools.list_ports

def find_rp2040():
    """Find the RP2040 board's serial port."""
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        # Look for typical RP2040 identifiers
        if "RP2040" in port.description or "Adafruit" in port.description or "USB Serial Device" in port.description:
            return port.device
    
    # If no specific identifier found, look for generic USB Serial Device
    for port in ports:
        if "USB Serial" in port.description:
            return port.device
            
    return None

def send_preset_command(port, preset_number):
    """
    Send a preset change command using the simple serial protocol:
    'P' followed by the preset number as a single byte
    """
    try:
        # Command format: 'P' (marker) + preset_number (0-127 as a byte)
        command = bytes(['P'.encode()[0], preset_number])
        port.write(command)
        
        # Wait for and read the confirmation response
        time.sleep(0.2)  # Give the Arduino time to process and respond
        response = port.readline().decode('utf-8').strip()
        
        if response:
            print(f"Device response: {response}")
        else:
            print("No confirmation received from device")
            
        return True
    except Exception as e:
        print(f"Error sending preset change: {e}")
        return False

def main():
    if len(sys.argv) != 2:
        print("Usage: python preset_switcher.py PRESET_NUMBER")
        print("Example: python preset_switcher.py 27")
        sys.exit(1)
    
    try:
        preset = int(sys.argv[1])
        if preset < 0 or preset > 127:
            print("Error: Preset number must be between 0 and 127")
            sys.exit(1)
    except ValueError:
        print("Error: Preset number must be an integer")
        sys.exit(1)
    
    # Try to find the RP2040 automatically
    port_name = find_rp2040()
    
    if not port_name:
        # If automatic detection fails, suggest manual entry
        print("RP2040 not automatically detected. Available ports:")
        for port in serial.tools.list_ports.comports():
            print(f"  {port.device}: {port.description}")
        
        port_name = input("Enter port name manually (e.g., COM3 or /dev/ttyACM0): ")
    
    try:
        # Open serial connection at 9600 baud (standard for USB Serial)
        print(f"Connecting to {port_name}...")
        ser = serial.Serial(port_name, 9600, timeout=1)
        time.sleep(0.5)  # Give the serial connection time to establish
        
        # Send the preset change command
        print(f"Sending command to switch to preset {preset}...")
        success = send_preset_command(ser, preset)
        
        # Close the connection
        ser.close()
        
        if success:
            print("Command sent successfully")
        else:
            print("Failed to send command")
            sys.exit(1)
            
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()