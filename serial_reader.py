import serial

name = input("Enter port name (COM3, COM4): ")
ser = serial.Serial(
    port= name,
    baudrate=57600,
    parity=serial.PARITY_ODD,
    stopbits=serial.STOPBITS_TWO,
    bytesize=serial.SEVENBITS
)

while(True):
    print(ser.readline())

ser.close()