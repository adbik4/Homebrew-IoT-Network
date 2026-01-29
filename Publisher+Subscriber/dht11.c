#include "dht11.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int DHT11_Init() {
    printf("DHT11 init - używam Pythona do odczytu\n");
    return 0;
}

int DHT11_ReadSensor(float *temperature, float *humidity) {
    FILE *fp;
    char buffer[128];
    char cmd[] = "sudo python3 -c \"import board, adafruit_dht, time; dht = adafruit_dht.DHT11(board.D5); print(f'{dht.temperature:.1f};{dht.humidity:.1f}')\"";
    
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }
    
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (sscanf(buffer, "%f;%f", temperature, humidity) == 2) {
            pclose(fp);
            return 0;
        }
    }
    
    pclose(fp);
    return -1;
}
