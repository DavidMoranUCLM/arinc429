#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "driver/gpio.h"
#include "encoder.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "transciever.h"

static const char* TAG = "P4";

//Test Alvaro

bool active = 1;
uint32_t heading;

typedef struct {
  fsm_t* encoder_fsm;
  fsm_t* transciever_fsm;
  bool* clk;
  TickType_t period;
} task_fsm_data_t;

typedef struct {
  uint32_t* heading;
  bool* clk;
  TickType_t period;
} task_heading_data_t;

void encoder_fsm_task(void* p) {
  task_fsm_data_t* data = (task_fsm_data_t*)p;
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    fsm_update(data->encoder_fsm);
    vTaskDelayUntil(&last, data->period);
  }
}

void transciever_fsm_task(void* p) {
  task_fsm_data_t* data = (task_fsm_data_t*)p;
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    fsm_update(data->transciever_fsm);
    vTaskDelayUntil(&last, data->period);
  }
}

void push_heading_task(void* p) {
  task_heading_data_t* data = (task_heading_data_t*)p;
  uint32_t heading = 0;

  TickType_t last = xTaskGetTickCount();
  for (;;) {
    vTaskDelay(data->period);
    heading = esp_cpu_get_cycle_count() % 3599;
    ESP_LOGI(TAG, "Heading: %lu", heading);

    *data->heading = heading;

    *data->clk = 1;
    vTaskDelay(pdMS_TO_TICKS(10));
    *data->clk = 0;

    vTaskDelayUntil(&last, data->period);
  }
}

void app_main(void) {
  encoder_t* encoder_fsm = encoder_new();
  transciever_t* transciever_fsm = transciever_new();

  encoder_init(encoder_fsm, &heading, &active, &transciever_fsm->out.transDone,
               transciever_fsm->out.interface0.mutex);
  transciever_init(transciever_fsm, &encoder_fsm->out.transOrder,
                   &encoder_fsm->out.traza, encoder_fsm->out.interface0.mutex);

  task_fsm_data_t encoder_data = {.encoder_fsm = (fsm_t*)encoder_fsm,
                                  .transciever_fsm = NULL,
                                  .clk = &active,
                                  .period = pdMS_TO_TICKS(100)};

  task_fsm_data_t transciever_data = {
      .encoder_fsm = NULL,
      .transciever_fsm = (fsm_t*)transciever_fsm,
      .clk = NULL,
      .period = pdMS_TO_TICKS(1)};

  task_heading_data_t data3 = {
      .clk = &active, .heading = &heading, .period = pdMS_TO_TICKS(2000)};

  TaskHandle_t t1, t2, t3;
  xTaskCreate(encoder_fsm_task, "t1", 2048, &encoder_data, 4, &t1);
  xTaskCreate(transciever_fsm_task, "t2", 2048, &transciever_data, 4, &t2);
  xTaskCreate(push_heading_task, "t3", 2048, &data3, 4, &t3);

  vTaskSuspend(NULL);
}
