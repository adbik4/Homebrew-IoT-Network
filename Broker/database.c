#include "database.h"
#include "stdio.h"
#include "constants.h"
#include <time.h>

extern FILE* db;
time_t currentTime;


int open_db() {
    // open for reading and appending at the end of file
    // (file is created if it doesn't exist)
    db = fopen(DATABASE_NAME, "a+");
    if (db == NULL) {
        return -1;
    }

    // if file is empty
    fseek(db, 0, SEEK_END);
    if (ftell(db) == 0) {
        fprintf(db, "Id;Timestamp;Temperature;Humidity\n"); // insert header
    }

    return 0;
}

void save2db(SensorData data) {
    if (db == NULL) return;

    time_t currentTime; // current time (seconds since epoch)
    char readableTime[32];

    time(&currentTime); 
    strftime(readableTime, sizeof(readableTime), "%d-%m-%Y %H:%M:%S", localtime(&currentTime));
    
    // parse and write the data to the database
    fprintf(db, "%hhu;%s;%hu;%hu\n",
        data.id,
        readableTime,
        data.temperature,
        data.humidity
    );
}

void close_db() {
    if (db != NULL) {
        fclose(db);
        db = NULL;
    }
}