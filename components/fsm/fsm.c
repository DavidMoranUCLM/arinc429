/* ~/~ begin <<docs/implementacion.md#src/fsm.c>>[init] */
#include "fsm.h"

#include <stdlib.h>

/* ~/~ begin <<docs/implementacion.md#fsm-new>>[init] */
fsm_t* fsm_new(fsm_trans_t* tt) {
  fsm_t* self = (fsm_t*)malloc(sizeof(fsm_t));
  fsm_init(self, tt);
  return self;
}
/* ~/~ end */
/* ~/~ begin <<docs/implementacion.md#fsm-init>>[init] */
void fsm_init(fsm_t* self, fsm_trans_t* tt) {
  self->tt = tt;
  self->current_state = tt[0].orig_state;
  int max_state = 0;
  for (fsm_trans_t* t = self->tt; t->orig_state >= 0; ++t) {
    if (t->orig_state > max_state) max_state = t->orig_state;
    if (t->dest_state > max_state) max_state = t->dest_state;
  }
  self->refined = (fsm_t**)calloc(max_state + 1, sizeof(fsm_t*));
  self->reset = NULL;
}
/* ~/~ end */
/* ~/~ begin <<docs/implementacion.md#fsm-update>>[init] */
void fsm_update(fsm_t* self) {
  int prev = self->current_state;
  fsm_trans_t* t;
  for (t = self->tt; t->orig_state >= 0; ++t) {
    if ((self->current_state == t->orig_state) && t->in(self)) {
      self->current_state = t->dest_state;
      if (t->out) t->out(self);
      break;
    }
  }
  
  fsm_t* refined = self->refined[self->current_state];
  if (!refined) return;  // no refinement
  if (t->reset && refined->reset && prev != self->current_state)
    refined->reset(refined);
  fsm_update(refined);
}
/* ~/~ end */
/* ~/~ begin <<docs/implementacion.md#implementacion-tipo-tiempos>>[init] */
void timed_init(timed_t* self) {
  self->sum = self->sum2 = 0.f;
  self->n = self->max = self->last = 0L;
}

void timed_tic(timed_t* self) { self->last = esp_cpu_get_cycle_count(); }

esp_cpu_cycle_count_t timed_toc(timed_t* self) {
  esp_cpu_cycle_count_t elapsed = esp_cpu_get_cycle_count() - self->last;
  if (elapsed > self->max) self->max = elapsed;
  self->sum += elapsed;
  self->sum2 += elapsed * elapsed;
  ++self->n;
  return elapsed;
}

esp_err_t timed_stats(timed_t* self, float* avg, float* sd) {
  if (self->n == 0) return ESP_FAIL;
  float mu = self->sum / self->n;
  if (avg) *avg = mu;
  if (sd) *sd = sqrt(self->sum2 / self->n - mu * mu);
  return ESP_OK;
}
/* ~/~ end */
/* ~/~ end */
