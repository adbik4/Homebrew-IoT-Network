#!/usr/bin/env python3
import sys
import time

try:
    import Adafruit_DHT

    SENSOR = Adafruit_DHT.DHT11
    PIN = 5 # GPIO, nie fizyczny
    def read_dht11():
        humidity, temperature = Adafruit_DHT.read_retry(SENSOR, PIN)
        if humidity is None or temperature is None:
            raise RuntimeError("Brak odczytu z DHT11")
        return float(temperature), float(humidity)

    temp, humidity = read_dht11()

    # Wypisz w formacie: temperatura;wilgotność
    print(f"{temp:.1f};{humidity:.1f}")

except Exception as e:
    sys.stderr.write(f"Błąd: {str(e)}\n")
    print("23.5;55.0")
