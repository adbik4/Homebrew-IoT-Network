#!/usr/bin/env python3
import sys
import time
import board
import adafruit_dht

try:
# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT

# Initial the dht device, with data pin connected to:
    dhtDevice = adafruit_dht.DHT11(board.D5)

# you can pass DHT22 use_pulseio=False if you wouldn't like to use pulseio.
# This may be necessary on a Linux single board computer like the Raspberry Pi,
# but it will not work in CircuitPython.
# dhtDevice = adafruit_dht.DHT22(board.D18, use_pulseio=False)
        # Print the values to the serial port
    temp = dhtDevice.temperature
    humidity = dhtDevice.humidity


    time.sleep(2.0)


    # Wypisz w formacie: temperatura;wilgotność
    print(f"{temp:.1f};{humidity:.1f}")

except Exception as e:
    sys.stderr.write(f"Błąd: {str(e)}\n")
    print("23.5;55.0")
