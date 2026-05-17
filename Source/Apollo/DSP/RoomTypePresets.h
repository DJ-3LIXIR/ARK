#pragma once

#include "EarlyReflections.h"

/**
    14 reverb room type presets.
    Each preset modifies internal DSP parameters that the user's
    controls feed into — they don't replace user parameters.
*/
struct ReverbRoomPreset
{
    const char* name;

    float sizeScale;       // multiplier on user's size value
    float densityScale;    // multiplier on allpass diffusion
    float dampingHigh;     // high-frequency damping coefficient (0-1)
    float dampingLow;      // low-frequency damping coefficient (0-1)
    float diffusionScale;  // multiplier on diffuser coefficients
    float modDepthScale;   // multiplier on modulation depth
    float predelayOffset;  // ms added to user's predelay
    float stereoSpread;    // decorrelation width (0-1)
    float decayScale;      // multiplier on user's decay time
    float erDensity;       // early reflection density (tap count scaling)

    int numTaps;
    EarlyReflections::Tap taps[24]; // early reflection tap pattern
};

// ============================================================================
// Tap patterns for each of the 14 room types
// ============================================================================

static const ReverbRoomPreset reverbRoomPresets[14] =
{
    // 0: Room — small, intimate, moderate reflections
    { "Room", 0.5f, 0.7f, 0.55f, 0.10f, 0.8f, 1.0f, 0.0f, 0.5f, 0.8f, 0.7f, 12,
      {
        { 3.2f,  0.85f, 0.80f }, { 5.1f,  0.78f, 0.82f }, { 7.8f,  0.72f, 0.75f },
        { 11.0f, 0.68f, 0.70f }, { 14.5f, 0.60f, 0.65f }, { 18.2f, 0.55f, 0.58f },
        { 22.0f, 0.48f, 0.52f }, { 26.5f, 0.42f, 0.45f }, { 30.0f, 0.36f, 0.40f },
        { 34.0f, 0.30f, 0.34f }, { 37.5f, 0.25f, 0.28f }, { 40.0f, 0.20f, 0.22f },
      }
    },

    // 1: Chamber — warm, musical, slightly longer
    { "Chamber", 0.6f, 0.8f, 0.45f, 0.08f, 0.9f, 0.8f, 2.0f, 0.6f, 1.0f, 0.8f, 14,
      {
        { 4.0f,  0.80f, 0.82f }, { 6.5f,  0.76f, 0.78f }, { 9.5f,  0.70f, 0.74f },
        { 13.0f, 0.66f, 0.68f }, { 16.5f, 0.60f, 0.64f }, { 20.5f, 0.55f, 0.58f },
        { 25.0f, 0.50f, 0.52f }, { 29.5f, 0.44f, 0.48f }, { 34.0f, 0.40f, 0.42f },
        { 38.5f, 0.35f, 0.38f }, { 43.0f, 0.30f, 0.33f }, { 48.0f, 0.26f, 0.28f },
        { 53.0f, 0.22f, 0.24f }, { 58.0f, 0.18f, 0.20f },
      }
    },

    // 2: Concert Hall — large, spacious, wide stereo
    { "Concert Hall", 1.5f, 0.9f, 0.35f, 0.05f, 0.95f, 0.6f, 15.0f, 0.9f, 1.5f, 0.9f, 20,
      {
        { 8.0f,  0.70f, 0.30f }, { 12.5f, 0.35f, 0.72f }, { 17.0f, 0.65f, 0.40f },
        { 22.5f, 0.42f, 0.68f }, { 28.0f, 0.60f, 0.45f }, { 34.0f, 0.48f, 0.62f },
        { 40.0f, 0.55f, 0.38f }, { 46.5f, 0.40f, 0.58f }, { 53.0f, 0.52f, 0.42f },
        { 60.0f, 0.44f, 0.54f }, { 67.0f, 0.48f, 0.36f }, { 74.0f, 0.38f, 0.50f },
        { 81.0f, 0.42f, 0.34f }, { 88.0f, 0.34f, 0.46f }, { 95.0f, 0.38f, 0.30f },
        { 102.0f, 0.30f, 0.40f }, { 108.0f, 0.32f, 0.26f }, { 114.0f, 0.26f, 0.35f },
        { 120.0f, 0.28f, 0.24f }, { 125.0f, 0.22f, 0.30f },
      }
    },

    // 3: Theatre — medium-large, warm, focused
    { "Theatre", 1.2f, 0.85f, 0.40f, 0.06f, 0.85f, 0.5f, 10.0f, 0.7f, 1.3f, 0.8f, 16,
      {
        { 6.0f,  0.75f, 0.72f }, { 10.0f, 0.70f, 0.68f }, { 15.0f, 0.64f, 0.66f },
        { 20.5f, 0.58f, 0.60f }, { 26.0f, 0.54f, 0.52f }, { 32.0f, 0.48f, 0.50f },
        { 38.0f, 0.44f, 0.42f }, { 44.5f, 0.40f, 0.38f }, { 51.0f, 0.36f, 0.34f },
        { 58.0f, 0.32f, 0.30f }, { 65.0f, 0.28f, 0.26f }, { 72.0f, 0.24f, 0.22f },
        { 79.0f, 0.20f, 0.19f }, { 86.0f, 0.17f, 0.16f }, { 92.0f, 0.14f, 0.13f },
        { 98.0f, 0.12f, 0.11f },
      }
    },

    // 4: Synth Hall — huge, ethereal, highly modulated
    { "Synth Hall", 1.8f, 1.0f, 0.20f, 0.03f, 1.0f, 1.5f, 20.0f, 1.0f, 2.0f, 1.0f, 18,
      {
        { 10.0f, 0.62f, 0.40f }, { 16.0f, 0.45f, 0.65f }, { 22.0f, 0.58f, 0.50f },
        { 30.0f, 0.50f, 0.60f }, { 38.0f, 0.55f, 0.42f }, { 47.0f, 0.44f, 0.56f },
        { 56.0f, 0.48f, 0.38f }, { 66.0f, 0.40f, 0.52f }, { 76.0f, 0.44f, 0.36f },
        { 87.0f, 0.36f, 0.48f }, { 98.0f, 0.40f, 0.34f }, { 108.0f, 0.34f, 0.44f },
        { 118.0f, 0.36f, 0.30f }, { 128.0f, 0.30f, 0.38f }, { 138.0f, 0.32f, 0.28f },
        { 148.0f, 0.28f, 0.34f }, { 158.0f, 0.26f, 0.24f }, { 168.0f, 0.22f, 0.28f },
      }
    },

    // 5: Digital — clean, precise, evenly spaced
    { "Digital", 1.0f, 1.0f, 0.10f, 0.01f, 1.0f, 0.3f, 0.0f, 0.8f, 1.0f, 1.0f, 16,
      {
        { 4.0f,  0.70f, 0.70f }, { 8.0f,  0.65f, 0.65f }, { 12.0f, 0.60f, 0.60f },
        { 16.0f, 0.55f, 0.55f }, { 20.0f, 0.50f, 0.50f }, { 24.0f, 0.45f, 0.45f },
        { 28.0f, 0.40f, 0.40f }, { 32.0f, 0.36f, 0.36f }, { 36.0f, 0.32f, 0.32f },
        { 40.0f, 0.28f, 0.28f }, { 44.0f, 0.25f, 0.25f }, { 48.0f, 0.22f, 0.22f },
        { 52.0f, 0.20f, 0.20f }, { 56.0f, 0.18f, 0.18f }, { 60.0f, 0.16f, 0.16f },
        { 64.0f, 0.14f, 0.14f },
      }
    },

    // 6: Dark Room — heavily damped highs, intimate
    { "Dark Room", 0.7f, 0.75f, 0.80f, 0.15f, 0.7f, 0.4f, 3.0f, 0.4f, 0.9f, 0.6f, 10,
      {
        { 3.5f,  0.90f, 0.88f }, { 6.0f,  0.82f, 0.80f }, { 9.0f,  0.74f, 0.72f },
        { 13.0f, 0.66f, 0.64f }, { 17.0f, 0.58f, 0.56f }, { 22.0f, 0.50f, 0.48f },
        { 27.0f, 0.42f, 0.40f }, { 32.0f, 0.34f, 0.32f }, { 37.0f, 0.26f, 0.24f },
        { 42.0f, 0.20f, 0.18f },
      }
    },

    // 7: Dense — thick, smeared, high diffusion
    { "Dense", 0.8f, 1.0f, 0.50f, 0.08f, 1.0f, 0.7f, 1.0f, 0.6f, 1.0f, 1.0f, 20,
      {
        { 2.0f,  0.60f, 0.58f }, { 3.5f,  0.58f, 0.56f }, { 5.0f,  0.56f, 0.54f },
        { 6.8f,  0.54f, 0.52f }, { 8.5f,  0.52f, 0.50f }, { 10.5f, 0.50f, 0.48f },
        { 12.5f, 0.48f, 0.46f }, { 14.8f, 0.46f, 0.44f }, { 17.0f, 0.44f, 0.42f },
        { 19.5f, 0.42f, 0.40f }, { 22.0f, 0.40f, 0.38f }, { 24.5f, 0.38f, 0.36f },
        { 27.0f, 0.36f, 0.34f }, { 29.5f, 0.34f, 0.32f }, { 32.0f, 0.32f, 0.30f },
        { 34.5f, 0.30f, 0.28f }, { 37.0f, 0.28f, 0.26f }, { 39.5f, 0.26f, 0.24f },
        { 42.0f, 0.24f, 0.22f }, { 44.5f, 0.22f, 0.20f },
      }
    },

    // 8: Smooth Space — lush, enveloping
    { "Smooth Space", 1.3f, 0.95f, 0.60f, 0.10f, 0.95f, 0.9f, 8.0f, 0.8f, 1.4f, 0.9f, 16,
      {
        { 5.0f,  0.72f, 0.70f }, { 9.5f,  0.68f, 0.66f }, { 14.5f, 0.62f, 0.64f },
        { 20.0f, 0.58f, 0.56f }, { 26.0f, 0.54f, 0.52f }, { 32.5f, 0.50f, 0.48f },
        { 39.0f, 0.46f, 0.44f }, { 46.0f, 0.42f, 0.40f }, { 53.0f, 0.38f, 0.36f },
        { 60.5f, 0.34f, 0.32f }, { 68.0f, 0.30f, 0.28f }, { 75.0f, 0.26f, 0.25f },
        { 82.0f, 0.23f, 0.22f }, { 89.0f, 0.20f, 0.19f }, { 96.0f, 0.18f, 0.17f },
        { 103.0f, 0.16f, 0.15f },
      }
    },

    // 9: Vocal Hall — mid-focused, flattering for vocals
    { "Vocal Hall", 1.0f, 0.8f, 0.50f, 0.12f, 0.8f, 0.6f, 5.0f, 0.5f, 1.1f, 0.7f, 14,
      {
        { 4.0f,  0.78f, 0.76f }, { 7.5f,  0.72f, 0.70f }, { 12.0f, 0.66f, 0.64f },
        { 17.0f, 0.60f, 0.58f }, { 22.5f, 0.54f, 0.52f }, { 28.0f, 0.48f, 0.46f },
        { 34.0f, 0.42f, 0.40f }, { 40.0f, 0.38f, 0.36f }, { 46.0f, 0.34f, 0.32f },
        { 52.0f, 0.30f, 0.28f }, { 58.0f, 0.26f, 0.25f }, { 64.0f, 0.23f, 0.22f },
        { 70.0f, 0.20f, 0.19f }, { 76.0f, 0.17f, 0.16f },
      }
    },

    // 10: Reflective Hall — bright, long early reflections
    { "Reflective Hall", 1.4f, 0.6f, 0.25f, 0.04f, 0.6f, 0.5f, 12.0f, 0.9f, 1.6f, 0.5f, 12,
      {
        { 8.0f,  0.82f, 0.35f }, { 16.0f, 0.40f, 0.80f }, { 25.0f, 0.78f, 0.38f },
        { 35.0f, 0.42f, 0.75f }, { 46.0f, 0.72f, 0.40f }, { 58.0f, 0.45f, 0.70f },
        { 70.0f, 0.66f, 0.42f }, { 83.0f, 0.44f, 0.64f }, { 96.0f, 0.60f, 0.40f },
        { 109.0f, 0.40f, 0.58f }, { 122.0f, 0.54f, 0.36f }, { 135.0f, 0.36f, 0.52f },
      }
    },

    // 11: Strange Room — irregular, asymmetric, modulated
    { "Strange Room", 1.1f, 0.9f, 0.30f, 0.20f, 0.7f, 2.0f, 0.0f, 1.0f, 1.2f, 0.8f, 18,
      {
        { 2.5f,  0.90f, 0.30f }, { 4.0f,  0.20f, 0.85f }, { 7.5f,  0.75f, 0.40f },
        { 9.0f,  0.35f, 0.80f }, { 13.5f, 0.65f, 0.50f }, { 15.0f, 0.45f, 0.70f },
        { 19.5f, 0.55f, 0.35f }, { 23.0f, 0.30f, 0.60f }, { 28.0f, 0.62f, 0.28f },
        { 31.0f, 0.25f, 0.55f }, { 36.5f, 0.50f, 0.32f }, { 40.0f, 0.28f, 0.48f },
        { 44.5f, 0.45f, 0.22f }, { 49.0f, 0.20f, 0.42f }, { 54.0f, 0.38f, 0.18f },
        { 59.0f, 0.16f, 0.35f }, { 65.0f, 0.30f, 0.15f }, { 70.0f, 0.14f, 0.28f },
      }
    },

    // 12: Airy — bright, open, sparse early reflections
    { "Airy", 1.6f, 0.7f, 0.15f, 0.02f, 0.9f, 1.2f, 18.0f, 0.9f, 1.8f, 0.6f, 10,
      {
        { 12.0f, 0.60f, 0.55f }, { 24.0f, 0.52f, 0.50f }, { 38.0f, 0.46f, 0.44f },
        { 54.0f, 0.40f, 0.38f }, { 72.0f, 0.34f, 0.32f }, { 90.0f, 0.28f, 0.26f },
        { 110.0f, 0.24f, 0.22f }, { 130.0f, 0.20f, 0.18f }, { 150.0f, 0.16f, 0.15f },
        { 170.0f, 0.13f, 0.12f },
      }
    },

    // 13: Bloomy — lush low-end, gradual build
    { "Bloomy", 1.4f, 0.95f, 0.45f, 0.03f, 0.95f, 1.0f, 10.0f, 0.7f, 1.7f, 0.9f, 18,
      {
        { 5.0f,  0.50f, 0.48f }, { 8.0f,  0.52f, 0.50f }, { 12.0f, 0.55f, 0.53f },
        { 16.0f, 0.58f, 0.56f }, { 21.0f, 0.60f, 0.58f }, { 26.0f, 0.58f, 0.56f },
        { 32.0f, 0.55f, 0.53f }, { 38.0f, 0.52f, 0.50f }, { 44.0f, 0.48f, 0.46f },
        { 51.0f, 0.44f, 0.42f }, { 58.0f, 0.40f, 0.38f }, { 65.0f, 0.36f, 0.34f },
        { 72.0f, 0.32f, 0.30f }, { 80.0f, 0.28f, 0.26f }, { 88.0f, 0.24f, 0.23f },
        { 96.0f, 0.20f, 0.19f }, { 104.0f, 0.17f, 0.16f }, { 112.0f, 0.14f, 0.13f },
      }
    },
};
