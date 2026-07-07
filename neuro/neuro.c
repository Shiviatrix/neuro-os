#include "neuro/neuro.h"
#include "kernel/vga.h"
#include <stdint.h>

// ------------------------- math & random -------------------------
static uint32_t xorshift32_state = 1;
static uint32_t xor_rand(void) {
    uint32_t x = xorshift32_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift32_state = x;
    return x;
}
static float frand01(void){ return (float)(xor_rand() & 0xFFFFFF) / (float)0xFFFFFF; }
static float fminf_custom(float a, float b) { return a < b ? a : b; }
static float floorf_custom(float x) { return (float)((int)x); }

// ------------------------- formatting ----------------------------
static void print_uint(uint32_t val) {
    if (val == 0) {
        vga_putchar('0');
        return;
    }
    char buf[12];
    int i = 0;
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

static void print_float2(float val) {
    // print integer part
    int int_part = (int)val;
    print_uint(int_part);
    vga_putchar('.');
    // print fractional part (2 digits)
    int frac_part = (int)((val - (float)int_part) * 100.0f);
    if (frac_part < 0) frac_part = -frac_part;
    if (frac_part < 10) vga_putchar('0');
    print_uint(frac_part);
}

// ------------------------- knobs (simple & intuitive) -------------------------
#define N            64         // neurons (LSM: 8 In, 48 Liquid, 8 Out)
#define MAX_EDGES   1024
#define SLOTS        8          // delay slots (max synaptic delay in ticks)

// neuron dynamics
#define LEAK        0.02f       // per tick leak (fraction of voltage)
#define THR_INIT    1.00f       // initial threshold
#define THR_MIN     0.50f
#define THR_MAX     2.00f
#define RESET_V     0.20f       // membrane after spike
#define REF_TICKS   4           // refractory ticks
#define INHIB       0.06f       // lateral inhibition delivered to others
#define NOISE       0.02f       // membrane noise amplitude per tick

// synapse dynamics
#define W_INIT      0.50f       // initial weight
#define W_MIN       0.00f
#define W_MAX       1.50f
#define EPSP_GAIN   0.50f       // delivered EPSP = w * EPSP_GAIN

// STDP (very simple triangular window)
#define STDP_WIN    20          // ticks
#define A_PLUS      0.010f      // LTP scale
#define A_MINUS     0.012f      // LTD scale

// homeostasis (move thresholds toward target rate)
#define RATE_TGT    0.05f       // target spikes / tick (~ 5% firing)
#define ETA_RATE    0.01f       // EMA step for rate
#define HOMEOK      0.02f       // threshold adjust per step

// --------------------------- structures --------------------------------------
typedef struct { int src, dst; float w; int delay; } Edge;

typedef struct {
    float v;            // membrane potential
    float thr;          // threshold
    float rate;         // EMA of firing rate
    int   refrac;       // refractory ticks left
    int   last_spike;   // last spike tick (for STDP)
} Neuron;

// --------------------------- globals -----------------------------------------
static Neuron G[N];
static Edge   E[MAX_EDGES];
static int    M = 0;                 // edge count
static float  inc[SLOTS][N];         // scheduled incoming EPSP by (slot, neuron)
static int    tick_now = 0;

// --------------------------- helpers -----------------------------------------
static int   slot_idx(int t){ return t % SLOTS; }

static void connect(int s, int d, float w, int delay_ticks){
    if (M < MAX_EDGES) {
        E[M].src = s;
        E[M].dst = d;
        E[M].w = w;
        E[M].delay = delay_ticks % SLOTS;
        M++;
    }
}

// --------------------------- STDP --------------------------------------------
static void stdp_on_post(int post){
    for (int e=0; e<M; ++e){
        if (E[e].dst != post) continue;
        int pre = E[e].src;
        int dt  = tick_now - G[pre].last_spike;
        if (dt > 0 && dt <= STDP_WIN){
            float s = (float)(STDP_WIN - dt) / (float)STDP_WIN;
            E[e].w += A_PLUS * s;
            if (E[e].w > W_MAX) E[e].w = W_MAX;
        }
    }
}

static void stdp_on_pre(int pre){
    for (int e=0; e<M; ++e){
        if (E[e].src != pre) continue;
        int post = E[e].dst;
        int dt   = tick_now - G[post].last_spike;
        if (dt > 0 && dt <= STDP_WIN){
            float s = (float)(STDP_WIN - dt) / (float)STDP_WIN;
            E[e].w -= A_MINUS * s;
            if (E[e].w < W_MIN) E[e].w = W_MIN;
        }
    }
}

// --------------------------- step --------------------------------------------
void neuro_step(void){
    int slot = slot_idx(tick_now);

    for (int i=0;i<N;i++){
        G[i].v += inc[slot][i];
        inc[slot][i] = 0.0f;
    }

    for (int i=0;i<N;i++){
        G[i].v -= G[i].v * LEAK;
        if (G[i].v < 0) G[i].v = 0;
        G[i].v += NOISE * (frand01() - 0.5f);
        if (G[i].v < 0) G[i].v = 0;
        if (G[i].refrac > 0) G[i].refrac--;
    }

    int fired[N];
    for (int i=0; i<N; i++) fired[i] = 0;
    
    for (int i=0;i<N;i++){
        if (G[i].refrac == 0 && G[i].v >= G[i].thr){
            fired[i] = 1;
            G[i].v = RESET_V;
            G[i].refrac = REF_TICKS;
            G[i].last_spike = tick_now;

            stdp_on_pre(i);

            for (int j=0;j<N;j++){
                if (j==i) continue;
                G[j].v -= INHIB;
                if (G[j].v < 0) G[j].v = 0;
            }
        }
    }

    for (int e=0; e<M; ++e){
        int s = E[e].src;
        if (!fired[s]) continue;
        int slot_future = slot_idx(tick_now + E[e].delay);
        int d = E[e].dst;
        inc[slot_future][d] += E[e].w * EPSP_GAIN;
    }
    for (int i=0;i<N;i++){
        if (fired[i]) stdp_on_post(i);
    }

    for (int i=0;i<N;i++){
        G[i].rate = (1.0f-ETA_RATE)*G[i].rate + ETA_RATE*(fired[i]?1.0f:0.0f);
        float err = G[i].rate - RATE_TGT;
        G[i].thr += HOMEOK * err;
        if (G[i].thr < THR_MIN) G[i].thr = THR_MIN;
        if (G[i].thr > THR_MAX) G[i].thr = THR_MAX;
    }

    tick_now++;
}

// --------------------------- simple I/O --------------------------------------
static void pulse(int i, float strength){
    if (i>=0 && i<N){
        int slot = slot_idx(tick_now);
        inc[slot][i] += strength;
    }
}

void neuro_init(void){
    xorshift32_state = 123456789; // "seed"
    M = 0; // reset edge count

    // Liquid State Machine topology
    // Input: 0-7, Liquid: 8-55, Output: 56-63
    
    // 1. Input -> Liquid (sparse, random)
    for (int i=0; i<8; i++){
        for (int j=0; j<4; j++) {
            int target = 8 + (xor_rand() % 48);
            connect(i, target, W_INIT, 1 + (xor_rand() % 3));
        }
    }

    // 2. Liquid -> Liquid (recurrent, random)
    for (int i=8; i<56; i++){
        for (int j=0; j<10; j++) {
            int target = 8 + (xor_rand() % 48);
            if (target != i) {
                connect(i, target, W_INIT, 1 + (xor_rand() % SLOTS));
            }
        }
    }

    // 3. Liquid -> Output (sparse, random)
    for (int i=56; i<64; i++){
        for (int j=0; j<8; j++) {
            int src = 8 + (xor_rand() % 48);
            connect(src, i, W_INIT, 1 + (xor_rand() % 2));
        }
    }

    for (int i=0;i<N;i++){
        G[i].v = 0.0f;
        G[i].thr = THR_INIT;
        G[i].rate = 0.0f;
        G[i].refrac = 0;
        G[i].last_spike = -1000000;
    }
    for (int s=0;s<SLOTS;s++) for (int i=0;i<N;i++) inc[s][i]=0.0f;
    
    pulse(0, 0.9f);
    pulse(3, 0.7f);
}

void neuro_step_and_print(void){
    if ((tick_now % 40) == 0) pulse(0, 0.6f);
    if ((tick_now % 95) == 0) pulse(2, 0.5f);
    if (frand01() < 0.03f) pulse((xor_rand() % 8), 0.3f); // sensory noise into Input Layer

    neuro_step();

    int liquid_spikes = 0;
    for (int i=8; i<56; i++) {
        if (G[i].refrac == REF_TICKS) liquid_spikes++;
    }

    vga_writestring("[t=");
    print_uint(tick_now);
    vga_writestring("] Liquid Spikes: ");
    print_uint(liquid_spikes);
    vga_writestring("  Output V: [");
    
    for (int i=56; i<64; i++){
        int dv = (int)fminf_custom(9.0f, floorf_custom(G[i].v / THR_MAX * 9.0f + 0.5f));
        vga_putchar('0' + dv);
        if (i!=63) vga_putchar(' ');
    }
    
    vga_writestring("]  M: ");
    print_uint(M);
    vga_putchar('\n');
}

float neuro_get_voltage(int neuron_id) {
    if (neuron_id >= 0 && neuron_id < N) {
        return G[neuron_id].v;
    }
    return 0.0f;
}
