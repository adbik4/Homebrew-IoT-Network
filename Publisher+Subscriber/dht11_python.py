import sys
import time
import digitalio
import board
import adafruit_dht

try:
    dhtDevice = adafruit_dht.DHT11(board.D5, use_pulseio=False)

    time.sleep(2.0)

    temp = dhtDevice.temperature
    humidity = dhtDevice.humidity

    if temp is None or humidity is None:
        raise RuntimeError("Brak danych z czujnika")

    print(f"{temp:.1f};{humidity:.1f}")

except Exception as e:
    sys.stderr.write(f"Błąd: {e}\n")
    print("310.5;155.0")
