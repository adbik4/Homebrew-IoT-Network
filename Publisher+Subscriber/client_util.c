#include "client_util.h"
#include "data_structs.h"
#include <stdio.h>

// Funkcja PrintMeasurement, wyświetlanie danych na ekranie dla subscribera
void PrintMeasurement(const SensorData *data) {
    if (
        data->id == 0 &&
        data->temperature == 0 &&
        data->humidity == 0
    ) {
      printf("\nBrak aktywności pod tym tematem\n");
      printf("----------------------------------------\n");
    } 
    else {
        printf("Odebrano pomiar:\n");
        printf("  ID źródła: 0x%02X\n", data->id);
        printf("  Temperatura: %u.%01u°C\n", data->temperature/10, data->temperature%10);
        printf("  Wilgotność: %u.%01u%%\n", data->humidity/10, data->humidity%10);  // ZAMIENIONE: pressure -> humidity
        printf("----------------------------------------\n");
    }
}
