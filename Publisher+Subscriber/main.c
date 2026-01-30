#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#include "constants.h"
#include "data_structs.h"
#include "simple_MQTT.h"
#include "dht11.h"

// Zmienne globalne
int tcp_conn_socket = -1;
int udp_socket = -1;
bool is_running = true;
uint8_t client_id = 0;
uint8_t target_id = 0;
uint8_t subscription_action = 0;
bool is_publisher = false;

void SignalHandler(int sig);

int main(int argc, char *argv[]) {
    // Rejestracja handlera sygnałów
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // Parsowanie argumentów
    if (argc < 2) {
        fprintf(stderr, "Użycie:\n");
        fprintf(stderr, "  %s <ID>                 - publikuj dane z podanym ID (hex)\n", argv[0]);
        fprintf(stderr, "  %s 0xFF <ID> <akcja>    - zarządzaj nasłuchem dla podanego ID (hex)\n", argv[0]);
        fprintf(stderr, "    akcja: 0 - rozpocznij nasłuch, 1 - zrezygnuj z nasłuchu\n");
        return 1;
    }
    
    // Konwersja ID z hex
    uint8_t id1 = (uint8_t)strtol(argv[1], NULL, 0);
    
    if (id1 != SPECIAL_LISTEN_ID) {
        // Tryb publikowania - wymagany 1 argument dodatkowy
        if (argc != 2) {
            fprintf(stderr, "Błąd: Dla trybu publikowania wymagany jest tylko 1 argument\n");
            fprintf(stderr, "Użycie: %s <ID>\n", argv[0]);
            return 1;
        }
        
        is_publisher = true;
        client_id = id1;
        printf("Klient publikujący z ID: 0x%02X\n", client_id);
        
        // Inicjalizacja wiringPi i czujnika DHT11 - ZMIENIONE NA GPIO5
        printf("Inicjalizacja czujnika DHT11 z biblioteką libdriver...\n");
        printf("Pin DHT11: GPIO%d (fizyczny pin 29)\n", DHT11_PIN);
        
        if (DHT11_Init() != 0) {
            fprintf(stderr, "Błąd: Nie udało się zainicjalizować czujnika DHT11\n");
            fprintf(stderr, "Upewnij się, że:\n");
            fprintf(stderr, "1. Czujnik DHT11 jest podłączony do pinu GPIO%d (fizyczny pin 29)\n", DHT11_PIN);
            fprintf(stderr, "2. Jest podłączony rezystor 10kΩ między DATA a 3.3V\n");
            fprintf(stderr, "3. Program jest uruchomiony z uprawnieniami sudo\n");
            fprintf(stderr, "4. Biblioteka wiringPi jest zainstalowana\n");
            return 1;
        }
        printf("Czujnik DHT11 zainicjalizowany pomyślnie z biblioteką libdriver\n");
        printf("Wykonam 2 testowe odczyty przed rozpoczęciem publikacji...\n");
        
        // Wykonaj testowe odczyty
        for (int i = 0; i < 2; i++) {
            float test_temp, test_humidity;
            if (DHT11_ReadSensor(&test_temp, &test_humidity) == 0) {
                printf("  Test %d: %.1f°C, %.1f%% - OK\n", i+1, test_temp, test_humidity);
            } else {
                printf("  Test %d: BŁĄD (używane będą wartości domyślne)\n", i+1);
            }
            if (i < 1) sleep(2); // DHT11 wymaga 2s między odczytami
        }
        printf("\n");
    } else {
        // Tryb nasłuchiwania - wymagane 3 argumenty dodatkowe (razem 4)
        if (argc != 4) {
            fprintf(stderr, "Błąd: Dla trybu nasłuchiwania wymagane są 3 argumenty\n");
            fprintf(stderr, "Użycie: %s 0xFF <ID> <akcja>\n", argv[0]);
            fprintf(stderr, "  akcja: 0 - rozpocznij nasłuch, 1 - zrezygnuj z nasłuchu\n");
            return 1;
        }
        
        is_publisher = false;
        client_id = SPECIAL_LISTEN_ID;
        target_id = (uint8_t)strtol(argv[2], NULL, 0);
        
        // Sprawdzenie akcji
        subscription_action = (uint8_t)strtol(argv[3], NULL, 0);
        if (subscription_action > 1) {
            fprintf(stderr, "Błąd: Nieprawidłowa akcja. Akceptowane wartości: 0 (start) lub 1 (stop)\n");
            return 1;
        }
        
        if (subscription_action == 0) {
            printf("Klient nasłuchujący dla ID: 0x%02X (rozpoczęcie nasłuchu)\n", target_id);
        } else {
            printf("Klient rezygnujący z nasłuchu dla ID: 0x%02X\n", target_id);
        }
    }
    
    // Połączenie z serwerem
    printf("Łączenie z serwerem MQTT...\n");
    if (Connect() != 0) {
        fprintf(stderr, "Nie udało się połączyć z serwerem\n");
        return 1;
    }
    
    printf("Połączono z serwerem MQTT\n");
    
    // Wysłanie żądania subskrypcji (jeśli dotyczy)
    if (!is_publisher) {
        if (subscription_action == 0){
            Subscribe(target_id, subscription_action);
        }
        // Jeśli rezygnujemy z nasłuchu, to od razu kończymy
        if (subscription_action == 1) {
            printf("Żądanie rezygnacji z nasłuchu wysłane. Zamykanie...\n");
            sleep(1); // Czekaj na potwierdzenie
            Disconnect();
            printf("Klient zakończył działanie\n");
            return 0;
        }
    }
    
    // Główna pętla programu
    if (is_publisher) {
        // Tryb publikowania - odczytuj dane z czujnika i wysyłaj
        printf("\n=== ROZPOCZYNANIE PUBLIKACJI DANYCH ===\n");
        printf("Czujnik: DHT11 (libdriver)\n");
        printf("ID klienta: 0x%02X\n", client_id);
        printf("Pin GPIO: %d (fizyczny pin 29)\n", DHT11_PIN);
        printf("Interwał odczytu: %d sekund\n", DHT11_WAIT_TIME);
        printf("Naciśnij Ctrl+C aby zakończyć\n");
        printf("=====================================\n\n");
        
        time_t last_read_time = 0;
        int successful_reads = 0;
        int failed_reads = 0;
        float last_temperature = 25.0f;
        float last_humidity = 50.0f;
        
        // Poczekaj chwilę przed rozpoczęciem
        sleep(1);
        
        while (is_running) {
            float temperature_c = 0.0f;
            float humidity_percent = 0.0f;
            
            // Sprawdź, czy minęło wystarczająco czasu od ostatniego odczytu DHT11
            time_t current_time = time(NULL);
            if (current_time - last_read_time >= DHT11_WAIT_TIME) {
                
                // Odczyt danych z czujnika DHT11 (nowa biblioteka)
                int read_result = DHT11_ReadSensor(&temperature_c, &humidity_percent);
                
                if (read_result == 0) {
                    // Sukces odczytu
                    successful_reads++;
                    printf("[%s] Odczyt #%d: Temp=%.1f°C, Wilgotność=%.1f%%\n", 
                           ctime(&current_time), successful_reads, temperature_c, humidity_percent);
                    
                    // Zapamiętaj ostatnie poprawne wartości
                    last_temperature = temperature_c;
                    last_humidity = humidity_percent;
                    last_read_time = current_time;
                    
                } else {
                    // Błąd odczytu - użyj ostatnich poprawnych wartości
                    failed_reads++;
                    fprintf(stderr, "[%s] BŁĄD odczytu #%d! Używam ostatnich poprawnych wartości\n", 
                            ctime(&current_time), failed_reads);
                    
                    temperature_c = last_temperature;
                    humidity_percent = last_humidity;
                    last_read_time = current_time;
                }
                
                // Przeliczanie jednostek
                // Temperatura: stopnie Celsjusza * 10 (dokładność 0.1°C)
                uint16_t temp_int = (uint16_t)(temperature_c * 10.0f);
                // Wilgotność: procenty * 10 (dokładność 0.1%)
                uint16_t humidity_int = (uint16_t)(humidity_percent * 10.0f);
                
                // Ograniczenie wartości do zakresu (0-1000 = 0-100%)
                if (humidity_int > 1000) humidity_int = 1000;
                
                // Wyślij dane do serwera MQTT
                Publish(temp_int, humidity_int);
                
                printf("  Wysłano do serwera: ID=0x%02X | Temp=%.1f°C | Humidity=%.1f%%\n",
                       client_id, temperature_c, humidity_percent);
                printf("  Statystyka: %d poprawnych, %d błędów\n\n", 
                       successful_reads, failed_reads);
                
                // Jeśli mamy 5 kolejnych błędów, wyświetl ostrzeżenie
                if (failed_reads >= 5 && failed_reads % 5 == 0) {
                    fprintf(stderr, "\nOSTRZEŻENIE: %d kolejnych błędów odczytu DHT11!\n", failed_reads);
                    fprintf(stderr, "   Sprawdź podłączenie czujnika i zasilanie.\n\n");
                }
                
            } else {
                // Czekaj do następnego odczytu
                usleep(100000); // 100ms
                continue;
            }
            
            // Krótka przerwa przed następną iteracją
            usleep(50000); // 50ms
        }
        
        // Podsumowanie po zakończeniu
        printf("\n=== PODSUMOWANIE ===\n");
        printf("Zakończono publikację danych\n");
        printf("Łącznie odczytów: %d\n", successful_reads + failed_reads);
        printf("  - Poprawnych: %d\n", successful_reads);
        printf("  - Błędów: %d\n", failed_reads);
        printf("Ostatnie wartości: %.1f°C, %.1f%%\n", last_temperature, last_humidity);
        
    } else {
        // Tryb nasłuchiwania - odbieraj dane
        printf("\n=== ROZPOCZYNANIE NASŁUCHIWANIA ===\n");
        printf("Nasłuchiwanie danych dla ID: 0x%02X\n", target_id);
        printf("Naciśnij Ctrl+C aby zakończyć\n");
        printf("====================================\n\n");
        
        int received_count = 0;
        
        while (is_running) {
            HandleIncomingData();
            usleep(100000); // 100ms
            
            // Wyświetl informację co 10 sekund, że nasłuchiwanie działa
            static time_t last_status_time = 0;
            time_t now = time(NULL);
            if (now - last_status_time >= 10) {
                printf("[%s] Nasłuchiwanie aktywne... (odebrano: %d)\n", 
                       ctime(&now), received_count);
                last_status_time = now;
            }
        }
    }
    
    // Rozłączenie
    printf("\nRozłączanie od serwera...\n");
    Disconnect();
    
    if (is_publisher) {
        printf("Czujnik DHT11 zamknięty\n");
    }
    
    printf("Klient zakończył działanie\n");
    return 0;
}


// Handler sygnałów
void SignalHandler(int sig) {
    printf("\n\n=== OTRZYMANO SYGNAŁ ZAKOŃCZENIA ===\n");
    printf("Sygnał: %d\n", sig);
    printf("Kończenie pracy...\n");
    is_running = false;
    
    // Daj czas na bezpieczne zamknięcie
    sleep(1);
}
