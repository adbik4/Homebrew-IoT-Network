#ifndef DATABASE_H
#define DATABASE_H
#include "data_structs.h"

int open_db(void);
void save2db(SensorData data);
void close_db(void);

#endif