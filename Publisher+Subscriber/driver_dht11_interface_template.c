/**
 * Interfejs dla biblioteki libdriver DHT11 dla Raspberry Pi z GPIO5
 */
#include "driver_dht11_interface.h"
#include "dht11.h"  // Dodano do użycia DHT11_PIN
#include <wiringPi.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

/**
 * @brief  Inicjalizacja interfejsu GPIO dla GPIO5
 * @return status code
 *         - 0 success
 *         - 1 bus init failed
 */
uint8_t dht11_interface_init(void) {
    printf("dht11_interface: Inicjalizacja GPIO%d...\n", DHT11_PIN);
    
    if (wiringPiSetupGpio() == -1) {
        printf("dht11_interface: wiringPiSetupGpio failed\n");
        return 1;
    }
    
    // Ustaw pin DHT11 (GPIO5) jako wyjście na początku
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    // Daj czas na stabilizację
    delay(1000);
    
    printf("dht11_interface: GPIO%d zainicjalizowany\n", DHT11_PIN);
    return 0;
}

/**
 * @brief  Deinicjalizacja interfejsu GPIO
 * @return status code
 *         - 0 success
 *         - 1 bus deinit failed
 */
uint8_t dht11_interface_deinit(void) {
    pinMode(DHT11_PIN, INPUT);
    printf("dht11_interface: GPIO%d zdeinicjalizowany\n", DHT11_PIN);
    return 0;
}

/**
 * @brief      Odczyt z GPIO5
 * @param[out] *value pointer to a value buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 */
uint8_t dht11_interface_read(uint8_t *value) {
    *value = digitalRead(DHT11_PIN);
    return 0;
}

/**
 * @brief     Zapis do GPIO5
 * @param[in] value written value
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 */
uint8_t dht11_interface_write(uint8_t value) {
    digitalWrite(DHT11_PIN, value);
    return 0;
}

/**
 * @brief     Opóźnienie w milisekundach
 * @param[in] ms czas w milisekundach
 */
void dht11_interface_delay_ms(uint32_t ms) {
    delay(ms);
}

/**
 * @brief     Opóźnienie w mikrosekundach
 * @param[in] us czas w mikrosekundach
 */
void dht11_interface_delay_us(uint32_t us) {
    delayMicroseconds(us);
}

/**
 * @brief Włącz przerwania (nie używane w tym projekcie)
 */
void dht11_interface_enable_irq(void) {
    // W tym projekcie nie używamy przerwań
}

/**
 * @brief Wyłącz przerwania (nie używane w tym projekcie)
 */
void dht11_interface_disable_irq(void) {
    // W tym projekcie nie używamy przerwań
}

/**
 * @brief     Debug print
 * @param[in] fmt format string
 */
void dht11_interface_debug_print(const char *const fmt, ...) {
#ifdef DEBUG_DHT11
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}
