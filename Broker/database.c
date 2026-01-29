#include "database.h"
#include "stdio.h"
#include "constants.h"
#include <time.h>

extern FILE* db;
time_t currentTime;
char readableTime[32];

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
        fprintf(db, "Id;Timestamp;Temperature;Pressure\n"); // insert header
    }

    return 0;
}

void save2db(SensorData data) {
    if (db == NULL) return;
    
    // parse and write the data to the database
    fprintf(db, "%hhu;%lu;%hu;%hu\n",
        data.id,
        time(NULL), // current time (seconds since epoch)
        data.temperature,
        data.pressure
    );
}

void close_db() {
    if (db != NULL) {
        fclose(db);
        db = NULL;
    }
}