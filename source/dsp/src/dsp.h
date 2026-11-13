// dsp.h: Robo Voice Engine's state.
#ifndef DSP_H
#define DSP_H

#include <stdint.h>

#include "fx_core.hpp"
#include "params.h"

constexpr uint32_t kPoolFloats = 16u;   // no long buffers
constexpr int kBands = 10;

struct Band {
    fx::Biquad ana, syn;
    fx::Follower env;
};

struct Dsp {
    // the vocoder
    Band band[kBands];
    fx::HighPass1 in_hp, out_hp;
    float semis, semis_t, ph[3], inc[3];
    fx::Smooth w_oct, w_fifth, breath;
    fx::HighPass1 noise_hp;
    // metal
    float mph1, mph2, minc1, minc2;
    fx::Smooth metal;
    // talk
    fx::Svf f1, f2, f3;
    float hz[3], hz_t[3], g[3];
    int vowel;
    fx::Follower fast, slow;
    float since_pick, since_roll;
    fx::Rng rng;
    fx::Smooth talk;
    // crush
    fx::Crusher crusher;
    fx::LowPass1 crush_lp;
    fx::Smooth crush_mix;
    // loudness matching, hum, output
    fx::Follower e_dry, e_wet;
    float gain, hum, hum_amt;
    bool settled;
    uint32_t tick;
    fx::Limiter limiter;
    fx::Smooth mix, level;
};

bool dsp_init(Dsp &d, fx::Pool &pool);
void dsp_settle(Dsp &d);
void dsp_reset(Dsp &d);
void dsp_param(Dsp &d, uint32_t index, float value);
void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames);

#endif
