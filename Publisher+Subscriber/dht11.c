#include "dht11.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <stdarg.h>

// Dołącz nagłówki biblioteki libdriver
#include "driver_dht11.h"
#include "driver_dht11_interface.h"

static dht11_handle_t dht11_handle;
static uint8_t is_initialized = 0;

// Funkcje interfejsu dla biblioteki libdriver

static uint8_t dht11_bus_init(void) {
    // Inicjalizacja wiringPi
    if (wiringPiSetupGpio() == -1) {
        return 1;
    }
    
    // Ustawienie pinu DHT11 jako wyjście na początku
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    return 0;
}

static uint8_t dht11_bus_deinit(void) {
    // Przywróć pin do stanu domyślnego
    pinMode(DHT11_PIN, INPUT);
    return 0;
}

static uint8_t dht11_bus_read(uint8_t *value) {
    *value = digitalRead(DHT11_PIN);
    return 0;
}

static uint8_t dht11_bus_write(uint8_t value) {
    digitalWrite(DHT11_PIN, value);
    return 0;
}

static void dht11_delay_ms(uint32_t ms) {
    delay(ms);
}

static void dht11_delay_us(uint32_t us) {
    delayMicroseconds(us);
}

static void dht11_enable_irq(void) {
    // W tym projekcie nie używamy przerwań
}

static void dht11_disable_irq(void) {
    // W tym projekcie nie używamy przerwań
}

static void dht11_debug_print(const char *const fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// Inicjalizacja DHT11 z biblioteką libdriver
int DHT11_Init() {
    if (is_initialized) {
        return 0;  // Już zainicjalizowane
    }
    
    printf("Inicjalizacja DHT11 z biblioteką libdriver na pinie GPIO%d...\n", DHT11_PIN);
    
    // Linkowanie funkcji interfejsu
    DRIVER_DHT11_LINK_INIT(&dht11_handle, dht11_handle_t);
    DRIVER_DHT11_LINK_BUS_INIT(&dht11_handle, dht11_bus_init);
    DRIVER_DHT11_LINK_BUS_DEINIT(&dht11_handle, dht11_bus_deinit);
    DRIVER_DHT11_LINK_BUS_READ(&dht11_handle, dht11_bus_read);
    DRIVER_DHT11_LINK_BUS_WRITE(&dht11_handle, dht11_bus_write);
    DRIVER_DHT11_LINK_DELAY_MS(&dht11_handle, dht11_delay_ms);
    DRIVER_DHT11_LINK_DELAY_US(&dht11_handle, dht11_delay_us);
    DRIVER_DHT11_LINK_ENABLE_IRQ(&dht11_handle, dht11_enable_irq);
    DRIVER_DHT11_LINK_DISABLE_IRQ(&dht11_handle, dht11_disable_irq);
    DRIVER_DHT11_LINK_DEBUG_PRINT(&dht11_handle, dht11_debug_print);
    
    // Inicjalizacja biblioteki libdriver
    uint8_t result = dht11_init(&dht11_handle);
    if (result != 0) {
        fprintf(stderr, "Błąd: Nie można zainicjalizować DHT11 z biblioteką libdriver (kod: %d)\n", result);
        return -1;
    }
    
    is_initialized = 1;
    printf("DHT11 zainicjalizowany pomyślnie z biblioteką libdriver.\n");
    return 0;
}

// Odczyt danych z czujnika DHT11 używając biblioteki libdriver
int DHT11_ReadSensor(float *temperature, float *humidity) {
    if (!is_initialized) {
        fprintf(stderr, "Błąd: DHT11 nie został zainicjalizowany\n");
        return -1;
    }
    
    uint16_t temperature_raw;
    uint16_t humidity_raw;
    float temp_float;
    uint8_t humidity_int;
    
    // Odczyt danych za pomocą funkcji z libdriver
    uint8_t result = dht11_read_temperature_humidity(&dht11_handle, 
                                                     &temperature_raw, &temp_float,
                                                     &humidity_raw, &humidity_int);
    
    if (result != 0) {
        // W przypadku błędu, spróbuj ponownie (maksymalnie 3 razy)
        for (int attempt = 0; attempt < 3; attempt++) {
            usleep(100000);  // Czekaj 100ms między próbami
            result = dht11_read_temperature_humidity(&dht11_handle,
                                                     &temperature_raw, &temp_float,
                                                     &humidity_raw, &humidity_int);
            if (result == 0) {
                break;  // Sukces
            }
        }
        
        if (result != 0) {
            fprintf(stderr, "Błąd odczytu DHT11 (libdriver kod: %d)\n", result);
            return -1;
        }
    }
    
    // Sprawdź zakresy dla DHT11
    if (temp_float < 0.0f || temp_float > 50.0f || humidity_int < 20 || humidity_int > 90) {
        fprintf(stderr, "Nieprawidłowe wartości odczytane z DHT11: %.1f°C, %d%%\n", 
                temp_float, humidity_int);
        return -1;
    }
    
    *temperature = temp_float;
    *humidity = (float)humidity_int;  // Konwersja na float
    
    return 0;
}
