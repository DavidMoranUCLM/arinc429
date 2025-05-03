#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include "fsm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    fsm_t parent;
    struct {
      uint32_t trace;
      uint8_t bitCounter;
    } internal;
    
    struct {
      uint32_t traza;
      bool transOrder;
      struct {
        uint32_t *traza;
        bool *transOrder;
        SemaphoreHandle_t mutex;
      } interface0;
    } in;

    struct{
      bool transDone;
      bool bit0;
      bool bit1;
      struct {
        bool transDone;
        bool bit0;
        bool bit1;
        SemaphoreHandle_t mutex;
      } interface0;
    } out;
} transciever_t;


transciever_t* transciever_new();
void transciever_init(transciever_t* self, bool *transOrder, uint32_t *traza, SemaphoreHandle_t mutex);

#endif