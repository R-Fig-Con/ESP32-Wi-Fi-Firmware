import serial
import time
import subprocess

# COM ports for each device
DEVICE1_PORT = "COM3"   # Change as needed
DEVICE2_PORT = "COM6"

# Upload firmware to both devices
#ensure they are not blocking, but still wait for upload to start serial connection
print("Before first sub process run\n")
subprocess.run(["pio", "test", "-e", "esp32_multi_testing", "--upload-port", DEVICE1_PORT])
print("After first sub process run\n")
subprocess.run(["pio", "test", "-e", "esp32_multi_testing", "--upload-port", DEVICE2_PORT])

# Open serial ports
ser1 = serial.Serial(DEVICE1_PORT, 57600, timeout=1)
ser2 = serial.Serial(DEVICE2_PORT, 57600, timeout=1)

# Assign roles
ser1.write(b"sender\n")
ser2.write(b"receiver\n")

time.sleep(2)  # Wait for tests to run

ser1.close()
ser2.close()
