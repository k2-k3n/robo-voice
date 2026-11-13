// dsp.cpp: Robo Voice Engine.
#include "dsp.h"

namespace {

// A E I O U (a low, boxy voice), three formants each
const float kVowel[5][3] = {
    {730.0f, 1090.0f, 2440.0f},
    {530.0f, 1840.0f, 2480.0f},
    {290.0f, 2250.0f, 2900.0f},
    {570.0f, 840.0f, 2410.0f},
    {320.0f, 870.0f, 2240.0f},
};
const float kRatio[3] = {1.0f, 2.0f, 1.5f};
const float kC2 = 65.4064f;
const float kLo = 120.0f, kHi = 5600.0f;   // the vocoder's bands, spaced evenly in pitch

void tune(Dsp &d)
{
    const float hz = kC2 * resona_exp2f(d.semis * (1.0f / 12.0f));
    for (int k = 0; k < 3; ++k) d.inc[k] = hz * kRatio[k] * fx::kInvRate;
    d.minc1 = hz * 2.756f * fx::kInvRate;
    d.minc2 = hz * 5.404f * fx::kInvRate;
}

void set_vowel(Dsp &d, int v)
{
    d.vowel = v;
    for (int k = 0; k < 3; ++k) d.hz_t[k] = kVowel[v][k];
}

void clear(Dsp &d)
{
    d.in_hp.reset(); d.out_hp.reset(); d.noise_hp.reset();
    for (int k = 0; k < kBands; ++k) { d.band[k].ana.reset(); d.band[k].syn.reset(); d.band[k].env.reset(); }
    d.ph[0] = 0.0f; d.ph[1] = 0.37f; d.ph[2] = 0.71f;
    d.mph1 = d.mph2 = 0.0f;
    d.f1.reset(); d.f2.reset(); d.f3.reset();
    d.fast.reset(); d.slow.reset(); d.e_dry.reset(); d.e_wet.reset();
    d.since_pick = d.since_roll = 0.0f;
    d.rng.reset();
    set_vowel(d, 0);
    for (int k = 0; k < 3; ++k) d.hz[k] = d.hz_t[k];
    d.crusher.reset(); d.crush_lp.reset();
    d.gain = 0.0f; d.hum = 0.0f;
    d.tick = 0;
    d.limiter.reset();
}

}  // namespace

bool dsp_init(Dsp &d, fx::Pool &)
{
    const float step = resona_powf(kHi / kLo, 1.0f / (float)(kBands - 1));
    float hz = kLo;
    for (int k = 0; k < kBands; ++k) {
        d.band[k].ana.set(fx::Biquad::BP, hz, 3.2f);
        d.band[k].syn.set(fx::Biquad::BP, hz, 3.2f);
        d.band[k].env.set(2.0f, 45.0f);
        hz *= step;
    }
    d.in_hp.set_hz(70.0f);
    d.out_hp.set_hz(60.0f);
    d.noise_hp.set_hz(2500.0f);
    d.semis = d.semis_t = 9.0f;
    d.fast.set(1.0f, 60.0f);
    d.slow.set(30.0f, 220.0f);
    d.e_dry.set(3.0f, 200.0f);
    d.e_wet.set(3.0f, 200.0f);
    d.rng.seed(0x0B07B07u);
    d.crusher.set(0.0f);
    d.crush_lp.set_hz(9000.0f);
    d.limiter.init();
    d.w_oct.init(0.6f, 40.0f); d.w_fifth.init(0.0f, 40.0f); d.breath.init(0.3f, 40.0f);
    d.metal.init(0.25f, 40.0f); d.talk.init(0.8f, 40.0f); d.crush_mix.init(0.0f, 30.0f);
    d.mix.init(0.85f, 30.0f); d.level.init(1.0f, 30.0f);
    d.hum_amt = 0.5f;
    d.settled = false;
    clear(d);
    tune(d);
    return true;
}

void dsp_settle(Dsp &d)
{
    d.semis = d.semis_t;
    tune(d);
    d.w_oct.snap(); d.w_fifth.snap(); d.breath.snap(); d.metal.snap(); d.talk.snap(); d.crush_mix.snap();
    d.mix.snap(); d.level.snap();
    d.settled = true;
}

void dsp_reset(Dsp &d) { clear(d); dsp_settle(d); }

void dsp_param(Dsp &d, uint32_t index, float v)
{
    switch (index) {
    case P_NOTE: {
        const float n = (float)(int)(fx::clampf(v, 0.0f, 24.0f) + 0.5f);
        if (d.settled && n != d.semis_t) d.hum = 1.0f;   // it hums its new note
        d.semis_t = n;
        break;
    }
    case P_TALK: d.talk.set(fx::pct(v)); break;
    case P_METAL: d.metal.set(fx::pct(v)); break;
    case P_CRUSH: {
        const float c = fx::pct(v);
        d.crusher.set(c);
        d.crush_mix.set(c > 0.005f ? 1.0f : 0.0f);
        d.crush_lp.set_hz(11000.0f - 6000.0f * c);
        break;
    }
    case P_MIX: d.mix.set(fx::pct(v)); break;
    case P_LEVEL: d.level.set(resona_db_to_lin(v)); break;
    case P_BREATH: d.breath.set(fx::pct(v)); break;
    case P_HARMONY: {
        const int h = (int)(fx::clampf(v, 0.0f, 2.0f) + 0.5f);
        d.w_oct.set(h >= 1 ? 0.6f : 0.0f);
        d.w_fifth.set(h >= 2 ? 0.5f : 0.0f);
        break;
    }
    case P_HUM: d.hum_amt = fx::pct(v); break;
    default: break;
    }
}

void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    const float thr = 0.004f;
    for (uint32_t i = 0; i < frames; ++i) {
        const float x = in[i];

        if ((d.tick++ & 7u) == 0u) {
            d.semis += 0.012f * (d.semis_t - d.semis);          // about 14 ms: a short slide
            tune(d);
            const float pi_fs = fx::kPi * fx::kInvRate;
            for (int k = 0; k < 3; ++k) {
                d.hz[k] += 0.035f * (d.hz_t[k] - d.hz[k]);       // vowels glide over about 5 ms
                d.g[k] = fx::tan_small(d.hz[k] * pi_fs);
            }
        }

        // a pick (or a long held note) makes it say another vowel
        const float ef = d.fast.process(x), es = d.slow.process(x);
        d.since_pick += 1.0f; d.since_roll += 1.0f;
        const bool pick = ef > 1.9f * es && ef > thr * 2.0f && d.since_pick > 4000.0f;
        const bool held = es > thr * 1.5f && d.since_roll > 26000.0f;
        if (pick || held) {
            if (pick) d.since_pick = 0.0f;
            d.since_roll = 0.0f;
            set_vowel(d, (d.vowel + 1 + (int)(d.rng.next() % 4u)) % 5);   // never the same twice
        }

        // the robot's own voice: saws at its note (octave, fifth) and a little hiss
        const float w[3] = {1.0f, d.w_oct.next(), d.w_fifth.next()};
        float car = 0.0f;
        for (int k = 0; k < 3; ++k) {
            d.ph[k] += d.inc[k]; if (d.ph[k] >= 1.0f) d.ph[k] -= 1.0f;
            car += w[k] * (2.0f * d.ph[k] - 1.0f - fx::blep(d.ph[k], d.inc[k]));
        }
        car += d.noise_hp.process(d.rng.bi()) * d.breath.next() * 1.2f;

        // it hums when its note is changed
        float hum = 0.0f;
        if (d.hum > 0.0f) {
            d.hum -= 1.0f / 16800.0f;                              // 350 ms
            if (d.hum < 0.0f) d.hum = 0.0f;
            const float a = 1.0f - d.hum;                          // 0 -> 1 through the hum
            hum = fx::clampf(a * 12.0f, 0.0f, 1.0f) * d.hum * d.hum * d.hum_amt * 0.03f;
        }

        // vocoder: each band of the voice is as loud as that band of the guitar (the highs a
        // little more, as a string has few of them)
        const float xin = d.in_hp.process(x);
        float wet = 0.0f, tilt = 1.0f;
        for (int k = 0; k < kBands; ++k) {
            Band &b = d.band[k];
            const float e = b.env.process(b.ana.process(xin)) * tilt + hum * (k < 7 ? 1.0f : 0.3f);
            wet += b.syn.process(car) * e;
            tilt *= 1.22f;
        }
        wet = d.out_hp.process(wet) * 6.0f;

        // METAL: ring it against two tones that are not in its harmonic series
        const float m = d.metal.next();
        d.mph1 = fx::wrap01(d.mph1 + d.minc1); d.mph2 = fx::wrap01(d.mph2 + d.minc2);
        const float ring = 0.7f * fx::sin01(d.mph1) + 0.4f * fx::sin01(d.mph2);
        wet = wet * (1.0f - 0.45f * m) + wet * ring * (1.6f * m);

        // TALK: vowel formants
        const float t = d.talk.next();
        float lp, b1, b2, b3;
        d.f1.process(wet, d.g[0], 0.16f, lp, b1);
        d.f2.process(wet, d.g[1], 0.13f, lp, b2);
        d.f3.process(wet, d.g[2], 0.13f, lp, b3);
        wet = wet * (1.0f - t) + (b1 + 0.7f * b2 + 0.35f * b3) * 0.5f * t;

        // loudness: as loud as the guitar itself, whatever the knobs do
        const float ed = d.e_dry.process(x) + hum;
        const float ew = d.e_wet.process(wet);
        // (a low buzz is peakier than a high one, so it gets a little more)
        const float gt = fx::clampf((0.95f + 0.36f * (1.0f - d.semis * (1.0f / 24.0f))) * ed / (ew + 2.0e-5f), 0.0f, 12.0f);
        d.gain = fx::flush(d.gain + 0.0015f * (gt - d.gain));
        wet *= d.gain;

        // CRUSH, scaled to the level it finds so quiet playing is not swallowed by the steps
        const float cm = d.crush_mix.next();
        if (cm > 0.0005f) {
            const float ref = ed * 2.5f + 1.0e-4f;
            wet += cm * (d.crush_lp.process(d.crusher.process(wet / ref)) * ref - wet);
        }

        const float mx = d.mix.next();
        out[i] = d.limiter.process((x * (1.0f - mx) + wet * mx) * d.level.next());
    }
}
