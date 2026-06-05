// main.c — tiny “as-brainy-as-possible” single-file in plain C
// build: clang -O2 -std=c11 -Wall main.c -o brain && ./brain
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h> // usleep

// ------------------------- knobs (simple & intuitive) -------------------------
#define N            8          // neurons (<=32 if you want bitsets later)
#define MAX_EDGES   24
#define SLOTS        8          // delay slots (max synaptic delay in ticks)
#define DT_MS       30          // ~33Hz update
#define SIM_TICKS  2000

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
#define RATE_TGT    0.05f       // target spikes / tick (≈ 5% firing)
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
static float frand01(void){ return (float)rand() / (float)RAND_MAX; }
static int   slot_idx(int t){ return t % SLOTS; }

static void connect(int s, int d, float w, int delay_ticks){
    if (M < MAX_EDGES) {
        E[M++] = (Edge){ .src=s, .dst=d, .w=w, .delay=delay_ticks % SLOTS };
    }
}

// --------------------------- STDP --------------------------------------------
// Apply LTP on POST spike (pre before post), LTD on PRE spike (post before pre)
static void stdp_on_post(int post){
    for (int e=0; e<M; ++e){
        if (E[e].dst != post) continue;
        int pre = E[e].src;
        int dt  = tick_now - G[pre].last_spike;     // pre -> post delay
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
        int dt   = tick_now - G[post].last_spike;   // post -> pre (post was before)
        if (dt > 0 && dt <= STDP_WIN){
            float s = (float)(STDP_WIN - dt) / (float)STDP_WIN;
            E[e].w -= A_MINUS * s;
            if (E[e].w < W_MIN) E[e].w = W_MIN;
        }
    }
}

// --------------------------- step --------------------------------------------
static void step(void){
    int slot = slot_idx(tick_now);

    // 1) deliver scheduled EPSPs for this slot
    for (int i=0;i<N;i++){
        G[i].v += inc[slot][i];
        inc[slot][i] = 0.0f;
    }

    // 2) leak + noise + refractory countdown
    for (int i=0;i<N;i++){
        // leak toward 0
        G[i].v -= G[i].v * LEAK;
        if (G[i].v < 0) G[i].v = 0;
        // tiny noise (can help exploration)
        G[i].v += NOISE * (frand01() - 0.5f);
        if (G[i].v < 0) G[i].v = 0;
        if (G[i].refrac > 0) G[i].refrac--;
    }

    // 3) threshold test → spike
    int fired[N] = {0};
    for (int i=0;i<N;i++){
        if (G[i].refrac == 0 && G[i].v >= G[i].thr){
            fired[i] = 1;
            G[i].v = RESET_V;
            G[i].refrac = REF_TICKS;
            G[i].last_spike = tick_now;

            // STDP: pre-before-post potentiates (handled on post),
            // here do pre-spike LTD for posts that fired shortly before
            stdp_on_pre(i);

            // lateral inhibition
            for (int j=0;j<N;j++){
                if (j==i) continue;
                G[j].v -= INHIB;
                if (G[j].v < 0) G[j].v = 0;
            }
        }
    }

    // 4) schedule outgoing spikes with delays; STDP on post
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

    // 5) homeostasis: adjust thresholds toward a target rate
    for (int i=0;i<N;i++){
        G[i].rate = (1.0f-ETA_RATE)*G[i].rate + ETA_RATE*(fired[i]?1.0f:0.0f);
        float err = G[i].rate - RATE_TGT;      // positive → firing too much
        G[i].thr += HOMEOK * err;
        if (G[i].thr < THR_MIN) G[i].thr = THR_MIN;
        if (G[i].thr > THR_MAX) G[i].thr = THR_MAX;
    }

    tick_now++;
}

// --------------------------- build a small cortex-ish net --------------------
static void build_network(void){
    // ring backbone
    for (int i=0;i<N;i++){
        connect(i, (i+1)%N, W_INIT, 1);
    }
    // a couple of shortcuts (like small-world)
    connect(0, 4, 0.7f, 2);
    connect(2, 6, 0.6f, 3);
    connect(5, 1, 0.5f, 2);

    // init neurons
    for (int i=0;i<N;i++){
        G[i].v = 0.0f;
        G[i].thr = THR_INIT;
        G[i].rate = 0.0f;
        G[i].refrac = 0;
        G[i].last_spike = -1000000;
    }
    // clear schedule
    for (int s=0;s<SLOTS;s++) for (int i=0;i<N;i++) inc[s][i]=0.0f;
}

// --------------------------- simple I/O --------------------------------------
static void pulse(int i, float strength){
    // inject external current by scheduling immediate EPSP
    if (i>=0 && i<N){
        int slot = slot_idx(tick_now);
        inc[slot][i] += strength;
    }
}
static void print_status(void){
    // compact state line: membrane (0..9), threshold (~0..9), and '*' for spikes
    // (spikes already executed this tick, so approximate by v close to RESET_V)
    printf("[t=%4d] V:", tick_now);
    for (int i=0;i<N;i++){
        int dv = (int)fminf(9.0f, floorf(G[i].v / THR_MAX * 9.0f + 0.5f));
        printf("%d", dv);
        if (i!=N-1) putchar(' ');
    }
    printf("  thr:");
    for (int i=0;i<N;i++){
        int dt = (int)fminf(9.0f, floorf((G[i].thr-THR_MIN)/(THR_MAX-THR_MIN)*9.0f + 0.5f));
        printf("%d", dt);
        if (i!=N-1) putchar(' ');
    }
    printf("  w[0→1]=%.2f", (M>0?E[0].w:0.0f));
    putchar('\n');
}

// --------------------------- main -------------------------------------------
int main(void){
    srand((unsigned)time(NULL));
    build_network();

    // kickstart a couple of neurons
    pulse(0, 0.9f);
    pulse(3, 0.7f);

    for (int t=0; t<SIM_TICKS; ++t){
        // periodic & random external input (like sensory noise)
        if ((t % 40) == 0) pulse(0, 0.6f);
        if ((t % 95) == 0) pulse(2, 0.5f);
        if (frand01() < 0.03f)  pulse(rand()%N, 0.3f);

        step();
        print_status();
        usleep(DT_MS * 1000);
    }

    // print final synapses (to see what learning did)
    puts("\nFinal synapses (src dst w delay):");
    for (int e=0;e<M;e++)
        printf("%d %d %.3f %d\n", E[e].src, E[e].dst, E[e].w, E[e].delay);
    return 0;
}
