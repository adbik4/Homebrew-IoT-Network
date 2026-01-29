#include "client_util.h"
#include "data_structs.h"
#include <time.h>
#include <stdio.h>

// Funkcja PrintMeasurement - wyświetla odebrane dane pomiarowe (ZAMIENIONE: pressure -> humidity)
void PrintMeasurement(const SensorData *data) {
    printf("Odebrano pomiar:\n");
    printf("  ID źródła: 0x%02X\n", data->id);
    printf("  Temperatura: %u.%01u°C\n", data->temperature/10, data->temperature%10);
    printf("  Wilgotność: %u.%01u%%\n", data->humidity/10, data->humidity%10);  // ZAMIENIONE: pressure -> humidity
    printf("----------------------------------------\n");
}