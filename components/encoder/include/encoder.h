#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "fsm.h"

typedef struct {
  fsm_t parent;
  struct {
    uint32_t label : 8;
    uint32_t sdi : 2;
    uint32_t ssm : 2;
    uint32_t headingBCD : 19;
    uint32_t parity : 1;
    //
    uint32_t headingDec;
  } internal;

  struct {
    uint32_t heading;
    bool clk;
    bool transDone;
    struct {
      uint32_t *heading;
      bool *clk;
      bool *transDone;
      SemaphoreHandle_t mutex;
    } interface0;
  } in;

  struct {
    uint32_t traza;
    bool transOrder;
    struct {
      SemaphoreHandle_t mutex;
      uint32_t traza;
      bool transOrder;
    } interface0;
  } out;
} encoder_t;

encoder_t *encoder_new();
void encoder_init(encoder_t *self, uint32_t *heading, bool *clk,
                  bool *transDone, SemaphoreHandle_t mutex);

#endif