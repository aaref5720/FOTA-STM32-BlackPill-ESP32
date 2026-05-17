"""
Direct UART diagnostic: sends 0xAB to the STM32 and prints everything received.
Run with: python3 uart_test.py
"""
import serial
import time

PORT = "/dev/ttyUSB1"
BAUD = 230400

print(f"Opening {PORT} at {BAUD} baud...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=2)
except serial.SerialException as e:
    print(f"ERROR: {e}")
    print("Is the port already open by another program (e.g. serial monitor)?")
    raise

time.sleep(0.1)          # let the port settle
ser.reset_input_buffer() # flush any leftover bytes

for attempt in range(4):
    print(f"\n[Attempt {attempt+1}] Sending 0xAB...")
    ser.write(bytes([0xAB]))
    ser.flush()

    start = time.time()
    received = bytearray()
    while (time.time() - start) < 2.0:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            received.extend(chunk)
            if len(received) >= 4:   # STM32 old code sent 4 bytes
                break

    if received:
        hex_bytes = " ".join(f"0x{b:02X}" for b in received)
        print(f"  Got {len(received)} byte(s): {hex_bytes}")
    else:
        print("  No response (timeout)")

    time.sleep(2)

ser.close()
print("\nDone.")
