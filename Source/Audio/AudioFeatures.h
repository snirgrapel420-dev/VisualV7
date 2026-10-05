#pragma once
// ============================================================================
//  AudioFeatures — one analysis snapshot (framework independent).
//  All "normalised" values are 0..1 after adaptive gain control.
// ============================================================================
#include <array>
#include <cstdint>

namespace dali
{
struct AudioFeatures
{
    // level
    float rms = 0, peak = 0;                  // normalised (dB window)
    // bands (instant, normalised) and envelopes
    float low = 0, mid = 0, high = 0;
    float bassEnv = 0, midEnv = 0, highEnv = 0;
    float energy = 0;                          // overall energy envelope
    // events / envelopes
    float kick = 0;                            // kick envelope (1 on kick, decays)
    float transient = 0;                       // transient envelope
    float onset = 0;                           // onset pulse envelope
    float snare = 0;                           // snare / clap envelope (mid-band noise hits)
    float hat = 0;                             // hi-hat envelope (high-band hits)
    // musical structure
    float build = 0;                           // 0..1 rises through breakdowns / build-ups (no kick)
    float drop = 0;                            // 1 when the kick returns after a breakdown, decays ~1.5 s
    float activity = 0;                        // 0 = silence, 1 = music playing (smoothed gate)

    // ---- six-band picture (auto-levelled 0..1, envelope-followed) ------------------------
    float sub = 0;                             // 20-60 Hz    (felt, not heard: breathing)
    float lowMid = 0;                          // 150-500 Hz  (body, warmth)
    float highMid = 0;                         // 2-6 kHz     (presence, attack)
    // ---- time scales ------------------------------------------------------------------
    float bassSlow = 0, energySlow = 0;        // SLOW   (~2-4 s): deep deformation, scene evolution
    float energyMed = 0, midMed = 0;           // MEDIUM (~0.5 s): rhythm, morphing
    float centroidSlow = 0, fluxSlow = 0;      // character of the sound over the last seconds
    // ---- densities & dynamics -------------------------------------------------------
    float kickDensity = 0;                     // kicks per second, 0..1 (1 = 2.5+/s)
    float onsetDensity = 0;                    // transients per second, 0..1 (1 = 14+/s)
    float dynamicRange = 0;                    // crest factor 0 (squashed) .. 1 (open, punchy)
    // ---- musical state: CALM, BUILD, PEAK, CHAOS, RELEASE ---------------------------------
    enum State { Calm = 0, Build, Peak, Chaos, Release, numStates };
    int   state = Calm;
    float stateTime = 0;                       // seconds in the current state
    std::array<float, numStates> stateWeight {{ 1, 0, 0, 0, 0 }};   // smoothed cross-fade weights (sum 1)
    // spectral
    float centroid = 0;                        // log-frequency position 0..1
    float flux = 0;                            // normalised spectral flux
    // stereo
    float width = 0;                           // 0 = mono, 1 = very wide
    float stereoEnergy = 0;                    // normalised energy of the side signal
    float pan = 0;                             // -1 left .. +1 right
    // tempo
    float bpm = 0;                             // detected BPM (0 = unknown)
    float bpmConfidence = 0;                   // 0..1
    float beatPhase = 0;                       // 0..1 at 'streamTime'
    std::uint32_t beatCount = 0;               // increments on each detected beat
    std::uint32_t kickCount = 0;
    std::uint32_t onsetCount = 0;
    std::uint32_t snareCount = 0;
    float bassNote = 0;                        // a bassline note between the beats (role-separated from the kick)
    std::uint32_t bassNoteCount = 0;
    std::uint32_t hatCount = 0;
    std::uint32_t dropCount = 0;
    // full picture of the sound for the GPU (uploaded as a 128x2 texture)
    static constexpr int kSpectrumBands = 128;
    static constexpr int kWaveSamples = 128;
    std::array<float, kSpectrumBands> spectrum {};   // log-spaced 30 Hz .. 16 kHz, 0..1 (pink-weighted, auto-levelled)
    std::array<float, kWaveSamples> wave {};         // latest waveform (mono), -1..1 auto-levelled
    // bookkeeping
    double streamTime = 0;                     // seconds of audio analysed
    bool   silent = true;
};
} // namespace dali
