#include "encoder.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "fsm.h"

static const char* TAG = "ENCODER";

enum {
  ENCODER_STATE_IDLE,
  ENCODER_STATE_READ,
};

static bool updateInputs(encoder_t* encoder) {
  if (xSemaphoreTake(encoder->in.interface0.mutex, portMAX_DELAY) == pdTRUE) {
    encoder->in.clk = *(encoder->in.interface0.clk);
    encoder->in.transDone = *(encoder->in.interface0.transDone);
    encoder->in.heading = *(encoder->in.interface0.heading);
    return xSemaphoreGive(encoder->in.interface0.mutex);
  }
  return false;
}

static bool updateOutputs(encoder_t* encoder) {
  if (xSemaphoreTake(encoder->out.interface0.mutex, portMAX_DELAY) == pdTRUE) {
    encoder->out.interface0.traza = encoder->out.traza;
    encoder->out.interface0.transOrder = encoder->out.transOrder;
    return xSemaphoreGive(encoder->out.interface0.mutex);
  }
  return false;
}

static uint8_t getReversedLabel(uint8_t label) {
  uint8_t reversedLabel = 0;
  for (int i = 0; i < 8; i++) {
    reversedLabel |= ((label >> i) & 1) << (7 - i);
  }
}
  

static uint32_t getTrace(encoder_t* encoder) {
  union {
    uint32_t trace;
    struct __attribute__((__packed__)) {
      uint32_t label : 8;
      uint32_t sdi : 2;
      uint32_t ssm : 2;
      uint32_t headingBCD : 19;
      uint32_t parity : 1;
    } fields;
  } u;

  u.fields.label = getReversedLabel(encoder->internal.label);
  u.fields.sdi = encoder->internal.sdi;
  u.fields.ssm = encoder->internal.ssm;
  u.fields.headingBCD = encoder->internal.headingBCD;
  u.fields.parity = encoder->internal.parity;

  return u.trace;
}

static uint32_t getHeadingBCD(encoder_t* encoder) {
  union {
    uint32_t d;
    struct __attribute__((__packed__)) {
      uint32_t d0 : 4;
      uint32_t d1 : 4;
      uint32_t d2 : 4;
      uint32_t d3 : 4;
      uint32_t d4 : 3;
    } headingBCD;
  } u;

  u.headingBCD.d0 = (encoder->internal.headingDec % 10);
  u.headingBCD.d1 = (encoder->internal.headingDec / 10) % 10;
  u.headingBCD.d2 = (encoder->internal.headingDec / 100) % 10;
  u.headingBCD.d3 = (encoder->internal.headingDec / 1000);
  u.headingBCD.d4 = 0;

  return u.d;
}

static uint32_t getParity(encoder_t* encoder) {
  union {
    uint32_t parity;
    struct __attribute__((__packed__)) {
      uint32_t label : 8;
      uint32_t sdi : 2;
      uint32_t ssm : 2;
      uint32_t headingBCD : 19;
      uint32_t parity : 1;
    } fields;
  } u;

  u.parity = 0;

  u.fields.label = encoder->internal.label;
  u.fields.sdi = encoder->internal.sdi;
  u.fields.ssm = encoder->internal.ssm;
  u.fields.headingBCD = encoder->internal.headingBCD;

  u.parity = (u.parity >> 16 ^ u.parity) & 0x0000FFFF;
  u.parity = (u.parity >> 8 ^ u.parity) & 0x000000FF;
  u.parity = (u.parity >> 4 ^ u.parity) & 0x0000000F;
  u.parity = (u.parity >> 2 ^ u.parity) & 0x00000003;
  u.parity = ((u.parity >> 1 ^ u.parity) ^ 0x00000001);  // Corregir

  return u.parity;
}

void action_encoder_read(fsm_t* self) {
  encoder_t* encoder = (encoder_t*)self;
  if (!updateInputs(encoder)) {
    ESP_LOGE(TAG, "Failed to update inputs");
    return;
  }

  ESP_LOGI(TAG, "Action: READ (headingDec=%lu)", encoder->internal.headingDec);

  encoder->internal.headingBCD = getHeadingBCD(encoder);
  encoder->internal.parity = getParity(encoder);

  encoder->out.traza = getTrace(encoder);
  encoder->out.transOrder = 1;

  if (!updateOutputs(encoder)) {
    ESP_LOGE(TAG, "Failed to update outputs");
  }
}

void action_encoder_idle(fsm_t* self) {
  encoder_t* encoder = (encoder_t*)self;
  if (!updateInputs(encoder)) {
    ESP_LOGE(TAG, "Failed to update inputs");
    return;
  }
  encoder->internal.headingDec = encoder->in.heading;
  encoder->out.transOrder = 0;

  if (!updateOutputs(encoder)) {
    ESP_LOGE(TAG, "Failed to update outputs");
  }
  ESP_LOGI(TAG, "Action: IDLE (headingDec set to %lu, transOrder=0)",
           encoder->internal.headingDec);
}

bool guard_encoder_read(fsm_t* self) {
  ESP_LOGI(TAG, "Guard: READ -> true");
  return true;
}
bool guard_encoder_idle(fsm_t* self) {
  encoder_t* encoder = (encoder_t*)self;

  if (!updateInputs(encoder)) {
    ESP_LOGE(TAG, "Failed to update inputs");
    return false;
  }

  bool result = (encoder->in.clk && encoder->in.transDone);

  if (!updateOutputs(encoder)) {
    ESP_LOGE(TAG, "Failed to update outputs");
  }

  ESP_LOGI(TAG, "Guard: IDLE (clk=%d, transDone=%d) -> %s", encoder->in.clk,
           encoder->in.transDone, result ? "true" : "false");

  return result;
}

encoder_t* encoder_new() {
  encoder_t *self = (encoder_t*)malloc(sizeof(encoder_t));

  self->out.interface0.mutex = xSemaphoreCreateMutex();
  if (self->out.interface0.mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create mutex");
    free(self);
    return NULL;
  }
  xSemaphoreGive(self->out.interface0.mutex);

  return self;
}

void encoder_init(encoder_t* self, uint32_t* heading, bool* clk,
                  bool* transDone, SemaphoreHandle_t mutex) {
  static fsm_trans_t tt[] = {
      {ENCODER_STATE_IDLE, guard_encoder_idle, ENCODER_STATE_READ,
       action_encoder_idle, true},
      {ENCODER_STATE_READ, guard_encoder_read, ENCODER_STATE_IDLE,
       action_encoder_read, true},
      {-1, NULL, -1, NULL, true},
  };

  fsm_init((fsm_t*)self, tt);

  self->in.interface0.heading = heading;
  self->in.interface0.clk = clk;
  self->in.interface0.transDone = transDone;
  self->in.interface0.mutex = mutex;

  self->out.traza = 0;
  self->out.transOrder = 0;

  updateOutputs(self);

  memset((void*)&self->internal, 0, sizeof(self->internal));
  ESP_LOGI(TAG, "Encoder initialized");
  return;
}