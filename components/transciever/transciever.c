#include "transciever.h"

#include <stdio.h>

#include "esp_log.h"

static const char* TAG = "TRANSCEIVER";

enum {
  TRANSCIEVER_STATE_START,
  TRANSCIEVER_STATE_TRANS_STEP,
  TRANSCIEVER_STATE_TX_BIT,
  TRANSCIEVER_STATE_TX_NULL,
};

static bool updateInputs(transciever_t* transciever) {
  if (xSemaphoreTake(transciever->in.interface0.mutex, portMAX_DELAY) ==
      pdTRUE) {
    transciever->in.traza = *(transciever->in.interface0.traza);
    transciever->in.transOrder = *(transciever->in.interface0.transOrder);
    return xSemaphoreGive(transciever->in.interface0.mutex);
  }
  return false;
}

static bool updateOutputs(transciever_t* transciever) {
  if (xSemaphoreTake(transciever->out.interface0.mutex, portMAX_DELAY) ==
      pdTRUE) {
    transciever->out.interface0.transDone = transciever->out.transDone;
    transciever->out.interface0.bit0 = transciever->out.bit0;
    transciever->out.interface0.bit1 = transciever->out.bit1;
    return xSemaphoreGive(transciever->out.interface0.mutex);
  }
  return false;
}

// Guards
static bool guard_transciever_start(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  if (!updateInputs(t)) {
    ESP_LOGE(TAG, "Failed to update inputs");
    return false;
  }
  bool result = (t->in.transOrder);
  ESP_LOGI(TAG, "Guard: START (transOrder=%d) -> %s", t->in.transOrder,
           result ? "true" : "false");
  return result;
}

static bool guard_transciever_trans_step(fsm_t* self) {
  ESP_LOGI(TAG, "Guard: TRANS_STEP -> true");
  return true;
}

static bool guard_transciever_tx_bit(fsm_t* self) {
  ESP_LOGI(TAG, "Guard: TX_BIT -> true");
  return true;
}

static bool guard_transciever_tx_null_trans_step(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  bool result = (t->internal.bitCounter < 31);
  ESP_LOGI(TAG, "Guard: TX_NULL_TRANS_STEP (bitCounter=%d) -> %s",
           t->internal.bitCounter, result ? "true" : "false");
  return result;
}

static bool guard_transciever_tx_null_start(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  bool result = (t->internal.bitCounter == 31);
  ESP_LOGI(TAG, "Guard: TX_NULL_START (bitCounter=%d) -> %s",
           t->internal.bitCounter, result ? "true" : "false");
  return result;
}

// Actions
static void action_tranciever_start(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  ESP_LOGI(TAG, "Action: START");

  if (!updateInputs(t)) {
    ESP_LOGE(TAG, "Failed to update inputs");
    return;
  }

  t->internal.trace = t->in.traza;
  t->internal.bitCounter = 0;
  t->out.transDone = 0;

  if (!updateOutputs(t)) {
    ESP_LOGE(TAG, "Failed to update outputs");
    return;
  }
}

static void action_transciever_trans_step(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  uint32_t bit = (t->internal.trace >> (t->internal.bitCounter)) & 0x1;
  ESP_LOGI(TAG, "Action: TRANS_STEP (bitCounter=%u, bit=%lu)",
           t->internal.bitCounter, bit);
  t->out.bit1 = bit;
  t->out.bit0 = !bit;

  if (!updateOutputs(t)) {
    ESP_LOGE(TAG, "Failed to update outputs");
    return;
  }
}

static void action_transciever_tx_bit(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  ESP_LOGI(TAG, "Action: TX_BIT");
  t->out.bit0 = 0;
  t->out.bit1 = 0;

  if (!updateOutputs(t)) {
    ESP_LOGE(TAG, "Failed to update outputs");
    return;
  }
}

static void action_transciever_tx_null_trans_step(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  t->internal.bitCounter++;
  ESP_LOGI(TAG, "Action: TX_NULL_TRANS_STEP (bitCounter incremented to %d)",
           t->internal.bitCounter);
}

static void action_transciever_tx_null_start(fsm_t* self) {
  transciever_t* t = (transciever_t*)self;
  ESP_LOGI(TAG, "Action: TX_NULL_START");
  t->out.transDone = 1;

  if(!updateOutputs(t)) {
    ESP_LOGE(TAG, "Failed to update outputs");
    return;
  }
}

transciever_t* transciever_new() {
  transciever_t *self = (transciever_t*)malloc(sizeof(transciever_t));

  self->out.interface0.mutex = xSemaphoreCreateMutex();
  if (self->out.interface0.mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create mutex");
    free(self);
    return NULL;
  }
  xSemaphoreGive(self->out.interface0.mutex);

  return self;
}

void transciever_init(transciever_t* self, bool* transOrder, uint32_t* traza,
                      SemaphoreHandle_t mutex) {
  static fsm_trans_t tt[] = {
      {TRANSCIEVER_STATE_START, guard_transciever_start,
       TRANSCIEVER_STATE_TRANS_STEP, action_tranciever_start, true},
      {TRANSCIEVER_STATE_TRANS_STEP, guard_transciever_trans_step,
       TRANSCIEVER_STATE_TX_BIT, action_transciever_trans_step, true},
      {TRANSCIEVER_STATE_TX_BIT, guard_transciever_tx_bit,
       TRANSCIEVER_STATE_TX_NULL, action_transciever_tx_bit, true},
      {TRANSCIEVER_STATE_TX_NULL, guard_transciever_tx_null_trans_step,
       TRANSCIEVER_STATE_TRANS_STEP, action_transciever_tx_null_trans_step,
       true},
      {TRANSCIEVER_STATE_TX_NULL, guard_transciever_tx_null_start,
       TRANSCIEVER_STATE_START, action_transciever_tx_null_start, true},
      {-1, NULL, -1, NULL, true},
  };

  fsm_init((fsm_t*)self, tt);

  self->in.interface0.mutex = mutex;

  self->in.interface0.transOrder = transOrder;
  self->in.interface0.traza = traza;
  self->out.transDone = 1;
  self->out.bit0 = 0;
  self->out.bit1 = 0;

  updateOutputs(self);

  self->internal.trace = 0;
  self->internal.bitCounter = 0;

  return;
}