#ifndef WEBDATA_H
#define WEBDATA_H

#include "FreeRTOS.h"
#include "semphr.h"

xSemaphoreHandle xSemaphore;

void prep_data(void *buf);

#endif