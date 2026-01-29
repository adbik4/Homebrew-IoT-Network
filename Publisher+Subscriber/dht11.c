#include "dht11.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>

// Inicjalizacja wiringPi i czujnika DHT11
int DHT11_Init() {
    // Inicjalizacja wiringPi
    if (wiringPiSetupPinType(WPI_PIN_PHYS) == -1) {
        perror("wiringPiSetup");
        return -1;
    }
    
    // Ustawienie pinu DHT11 jako wyjście na początku
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    return 0;
}

// Odczyt danych z czujnika DHT11
int DHT11_ReadSensor(float *temperature, float *humidity){
	uint64_t counter = 0;
	uint8_t j, i;
	int dht11_dat[41];
	unsigned long time_in_micros;
	struct timeval return_of_sig;
	pinMode(DHT11_PIN, OUTPUT);
	digitalWrite(DHT11_PIN, LOW);
	delay(2);
	pinMode(DHT11_PIN, INPUT);
	for(i = 1; i < 2000; i++){
		if(digitalRead(DHT11_PIN) == 0){
			break;
		}
	}
	for(j=0; j < 41; j++){
		for(i = 1; i < 2000; i++){
			if(digitalRead(DHT11_PIN) == 1){
				break;
			}
		}
		
		gettimeofday(&return_of_sig, NULL);
		time_in_micros = 1000000 * return_of_sig.tv_sec + return_of_sig.tv_usec;
		
		for(i = 1; i < 2000; i++){
			if(digitalRead(DHT11_PIN) == 0){
				break;
			}
		}
		
		gettimeofday(&return_of_sig, NULL);
		buf[j] = ((1000000 * return_of_sig.tv_sec + return_of_sig.tv_usec) - time_in_micros) > 50;
	}
	
	int byte1 = getByte(1, buf);
	int byte2 = getByte(2, buf);
	int byte3 = getByte(3, buf);
	int byte4 = getByte(4, buf);
	int byte5 = getByte(5, buf);
	
	pinMode(DHT11_PIN, OUTPUT);
	digitalWrite(DHT11_PIN, HIGH);
	
	if(byte5 == ((byte1+byte2+byte3+byte4)&0xFF)){
		float humidity_temp = (float)(byte1 << 8 | byte2)/10.0;
		int neg = byte3 & 0x80;
		byte3 = byte3 & 0x7F;
		float temperature_temp = (float)(byte3 << 8 | byte4) / 10.0;
		if(neg > 0){
			temperature_temp = -temperature_temp;
		}
		*humidity = humidity_temp;
		*temperature = temperature_temp;
		return 0;
	}
	return -1;
}

uint8_t getByte(int b, int buf[]){
	int i;
	uint8_t result = ;
	b = (b - 1)*8 + 1;
	for(i = b; i <= b+7; i++){
		result = result << 1;
		result = result | buf[i];
	}
	return result;
}
