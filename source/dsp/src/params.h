// params.h: Robo Voice Engine. A robot repeats whatever you play in its one-note voice.
// A vocoder: the guitar's spectrum, band by band, is laid over a buzzing voice that only knows
// one NOTE (plus its octave and fifth), so every pick comes out at that pitch. TALK runs it
// through vowel formants that jump to another vowel on each pick; METAL ring-modulates it;
// CRUSH is an old speech chip.
//
//   guitar -> 10 band envelopes --+
//   voice at NOTE -> 10 bands ----x--> METAL ring -> TALK formants -> CRUSH
//         -> matched to the guitar's own loudness -> MIX -> limiter
#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

#include <resona/module.h>

#define MODULE_ID "org.resona.creative.robo-voice.voice"
#define MODULE_NAME "Robo Voice Engine"
#define MODULE_TITLE "ROBO VOICE"
#define MODULE_VERSION "1.0.0"

enum Param : uint32_t { P_NOTE, P_TALK, P_METAL, P_CRUSH, P_MIX, P_LEVEL, P_BREATH, P_HARMONY, P_HUM, P_COUNT };

const resona_param kParams[P_COUNT] = {
    {"NOTE", 0.0f, 24.0f, 9.0f, "st"},      // semitones above C2
    {"TALK", 0.0f, 100.0f, 80.0f, "%"},
    {"METAL", 0.0f, 100.0f, 25.0f, "%"},
    {"CRUSH", 0.0f, 100.0f, 0.0f, "%"},
    {"MIX", 0.0f, 100.0f, 85.0f, "%"},
    {"LEVEL", -12.0f, 6.0f, 0.0f, "dB"},
    {"BREATH", 0.0f, 100.0f, 30.0f, "%"},   // noise in the voice: picks hiss like consonants
    {"HARMONY", 0.0f, 2.0f, 1.0f, ""},
    {"HUM", 0.0f, 100.0f, 50.0f, "%"},      // the note it hums when NOTE changes
};

static const char *const kHarmonyNames[3] = {"ROOT", "OCTAVE", "POWER"};
#define PX_NONE {0, 0.0f, 0, nullptr}
const resona_param_ext kParamExt[P_COUNT] = {
    {RESONA_PARAM_STEPPED, 1.0f, 0, nullptr}, PX_NONE, PX_NONE, PX_NONE, PX_NONE, PX_NONE, PX_NONE,
    {RESONA_PARAM_CHOICE, 0.0f, 3, kHarmonyNames}, PX_NONE,
};

#endif
