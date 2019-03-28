from time import sleep
import spidev
import ctypes
spi = spidev.SpiDev()
spi.open(0,0)
spi.max_speed_hz = 150000

try:
    kl = 't'
    while kl!='q':
        kl = raw_input("Akcja: ")
        if kl == 'w':
            v = int(raw_input("Predkosc: "))
            spi.xfer([0x03, v])
        elif kl == 's':
            v = int(raw_input("Predkosc: "))
            spi.xfer([0x02, v])
        elif kl == 'e':
            v = int(raw_input("Predkosc: "))
            spi.xfer([0x04, v])
        elif kl == 'd':
            v = int(raw_input("Predkosc: "))
            spi.xfer([0x05, v])
        elif kl == 'z':
            spi.xfer([0x0C])
            val1 = spi.xfer([0x00])
            val2 = spi.xfer([0x00])
            n11 = ctypes.c_int16(val2[0]<<8|(val1[0])).value
            print n11
        elif kl == 'x':
            spi.xfer([0x0E])
            val1 = spi.xfer([0x00])
            val2 = spi.xfer([0x00])
            n21 = ctypes.c_int16(val2[0]<<8|(val1[0])).value
            print n21
        elif kl == 'r':
            spi.xfer([0x10])
            val1 = spi.xfer([0x00])
            val2 = spi.xfer([0x00])
            n21 = ctypes.c_int16(val2[0]<<8|(val1[0])).value
            print n21
            v = (n21*505.)/1024.
            print v/50.
        elif kl == 'l':
            spi.xfer([0x12])
finally:
    spi.xfer([0x02, 0x00])
    spi.xfer([0x04, 0x00])
    spi.close()
