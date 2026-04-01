#!/usr/bin/env python3
"""
CRC8 Dallas/Maxim calculator for sensor packet testing

Usage: python3 crc8.py 0xA5 0x5A 0x01 0x02 0xAB 0xCD
"""

import sys

def crc8_dallas_maxim(data):
    """
    Compute CRC8 with Dallas/Maxim polynomial (0x31)

    Args:
        data: List of byte values

    Returns:
        CRC8 byte value
    """
    crc = 0x00
    poly = 0x31

    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ poly) & 0xFF
            else:
                crc = (crc << 1) & 0xFF

    return crc

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 crc8.py <byte1> <byte2> ... <byteN>")
        print("Example: python3 crc8.py 0xA5 0x5A 0x01 0x02 0xAB 0xCD")
        sys.exit(1)

    # Parse hex bytes from command line
    data = []
    for arg in sys.argv[1:]:
        try:
            if arg.startswith('0x') or arg.startswith('0X'):
                byte = int(arg, 16)
            else:
                byte = int(arg)

            if not 0 <= byte <= 255:
                print(f"Error: {arg} is not a valid byte (0-255)")
                sys.exit(1)

            data.append(byte)
        except ValueError:
            print(f"Error: {arg} is not a valid number")
            sys.exit(1)

    # Compute CRC
    crc = crc8_dallas_maxim(data)

    # Print result
    print(f"0x{crc:02X}")

if __name__ == "__main__":
    main()
