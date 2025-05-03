#include "brz.h"

brz_t* brz_new(bool* active, uint32_t* w)
{
    brz_t* self = (brz_t*)malloc(sizeof(brz_t));
    if (self) {
        brz_init(self, active, w);
    }
    return self;
}

enum {
    BRZ_IDLE,
    BRZ_CAPTURE,
    BRZ_XMIT,
    BRZ_WAIT_ACK,
};

static bool guarda_brz_idle(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    return *brz->active;
}

static bool guarda_brz_xmit_wait_ack(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    return brz->bit == 32;
}

static bool guarda_brz_wait_ack(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    return !*brz->active;
}

static bool guarda_brz_xmit_xmit(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    return brz->bit < 32 && !brz->rz;
}

static bool guarda_brz_xmit_xmit2(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    return brz->rz;
}

static bool guarda_true(fsm_t* self) { return true; }

static void salida_brz_idle(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    brz->bit = 0;
    brz->done = false;
}

static void salida_brz_capture(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    brz->data = *brz->w;
    brz->rz = false;
}

static void salida_brz_xmit_wait_ack(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    brz->bit = 0;
}

static void salida_brz_wait_ack(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    brz->done = false;
    brz->out = 0;
}

static void salida_brz_xmit_xmit(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    brz->data = (brz->data << 1);
    brz->bit++;
    brz->done = false;
    brz->out = (brz->data & 1 ? 2: 1);
    brz->rz = !brz->rz;
}

static void salida_brz_xmit_xmit2(fsm_t* self)
{
    brz_t* brz = (brz_t*)self;
    brz->out = 0;
    brz->rz = !brz->rz;
}

void brz_init(brz_t* self, bool* active, uint32_t* w)
{
    static fsm_trans_t tt[] = {
        { BRZ_IDLE, guarda_brz_idle, BRZ_CAPTURE, salida_brz_idle, true },
        { BRZ_CAPTURE, guarda_true, BRZ_XMIT, salida_brz_capture, true },
        { BRZ_XMIT, guarda_brz_xmit_wait_ack, BRZ_WAIT_ACK, salida_brz_xmit_wait_ack, true },
        { BRZ_XMIT, guarda_brz_xmit_xmit, BRZ_XMIT, salida_brz_xmit_xmit, true },
        { BRZ_XMIT, guarda_brz_xmit_xmit2, BRZ_XMIT, salida_brz_xmit_xmit2, true },
        { BRZ_WAIT_ACK, guarda_brz_wait_ack, BRZ_IDLE, salida_brz_wait_ack, true },
        { -1, NULL, -1, NULL, true },
    };

    fsm_init((fsm_t*)self, tt);
    self->data = 0;
    self->bit = 0;
    self->done = false;
    self->rz = false;
    self->out = 0;
    self->active = active;
    self->w = w;
}
