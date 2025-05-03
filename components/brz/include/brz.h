#ifndef BRZ_H
#define BRZ_H

#include "fsm.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    fsm_t parent;
    uint32_t data;
    unsigned bit;
    bool done;
    bool rz;
    unsigned out;
    bool* active;
    uint32_t* w;
} brz_t;


brz_t* brz_new(bool* active, uint32_t* w);
void brz_init(brz_t* self, bool* active, uint32_t* w);

#endif