/* ~/~ begin <<docs/implementacion.md#src/fsm.h>>[init] */
#ifndef FSM_H
#define FSM_H
#include <stdbool.h>
#include <stddef.h>

typedef struct fsm_ fsm_t;

typedef bool (*fsm_input_func_t)(fsm_t*);
typedef void (*fsm_output_func_t)(fsm_t*);
typedef void (*fsm_reset_func_t)(fsm_t*);

/* ~/~ begin <<docs/implementacion.md#nueva-estructura-transicion>>[init] */
typedef struct fsm_trans_t {
  int orig_state;
  fsm_input_func_t in;
  int dest_state;
  fsm_output_func_t out;
  bool reset;
} fsm_trans_t;
/* ~/~ end */
/* ~/~ begin <<docs/implementacion.md#nueva-estructura-fsm>>[init] */

typedef struct fsm_ {
  int current_state;
  fsm_trans_t* tt;
  /* ~/~ begin <<docs/implementacion.md#estados-refinados>>[init] */
  fsm_t** refined;
  /* ~/~ end */

  fsm_reset_func_t reset;
};
/* ~/~ end */
/* ~/~ begin <<docs/implementacion.md#acceso-a-refinamientos>>[init] */
#define fsm_refined(self, state) ((self)->refined[state])
/* ~/~ end */

fsm_t* fsm_new(fsm_trans_t* tt);
void fsm_init(fsm_t* self, fsm_trans_t* tt);
void fsm_update(fsm_t* self);

/* ~/~ begin <<docs/implementacion.md#tipo-tiempos>>[init] */
#include <esp_cpu.h>
#include <math.h>

typedef struct {
  esp_cpu_cycle_count_t max;
  esp_cpu_cycle_count_t last;
  float sum;
  float sum2;
  unsigned long n;
} timed_t;

void timed_init(timed_t* self);
void timed_tic(timed_t* self);
esp_cpu_cycle_count_t timed_toc(timed_t* self);
esp_err_t timed_stats(timed_t* self, float* avg,
                 float* sd);
/* ~/~ end */

#endif
/* ~/~ end */
