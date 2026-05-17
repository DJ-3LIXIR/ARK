#pragma once

/**
    8 room simulator presets.
    Each defines the physical dimensions (as ratios relative to the
    user's room size parameter) and material absorption properties.
*/
struct RoomSimPreset
{
    const char* name;

    // Dimension ratios (multiplied by roomSize in meters)
    float widthRatio;
    float heightRatio;
    float depthRatio;

    // Absorption coefficients (0 = fully reflective, 1 = fully absorbed)
    float wallAbsorption;
    float floorAbsorption;
    float ceilingAbsorption;

    // Base diffusion coefficient
    float diffusionCoeff;

    // Late reverb character
    float lateDensity;     // density of late FDN (0-1)
    float lateDecayScale;  // multiplier on user's decay parameter
};

static const RoomSimPreset roomSimPresets[8] =
{
    // 0: Theater — wide, tall, long, moderate absorption
    { "Theater",
      2.0f, 1.2f, 2.5f,
      0.30f, 0.40f, 0.50f,
      0.70f, 0.85f, 1.2f },

    // 1: Church — tall ceiling, narrow, very reverberant
    { "Church",
      1.5f, 2.5f, 3.0f,
      0.20f, 0.30f, 0.20f,
      0.80f, 0.90f, 1.5f },

    // 2: Studio — small, treated, dead
    { "Studio",
      1.0f, 0.6f, 1.2f,
      0.70f, 0.60f, 0.80f,
      0.40f, 0.60f, 0.6f },

    // 3: Hall — balanced, medium-large
    { "Hall",
      1.8f, 1.5f, 2.0f,
      0.25f, 0.35f, 0.40f,
      0.75f, 0.80f, 1.3f },

    // 4: Garage — hard surfaces, boxy
    { "Garage",
      1.3f, 0.7f, 1.5f,
      0.10f, 0.15f, 0.10f,
      0.30f, 0.70f, 0.8f },

    // 5: Bathroom — tiny, very reflective
    { "Bathroom",
      0.5f, 0.6f, 0.4f,
      0.05f, 0.10f, 0.05f,
      0.20f, 0.95f, 0.5f },

    // 6: Bedroom — medium, soft surfaces
    { "Bedroom",
      0.8f, 0.5f, 0.9f,
      0.60f, 0.50f, 0.70f,
      0.50f, 0.65f, 0.7f },

    // 7: Warehouse — huge, hard, sparse reflections
    { "Warehouse",
      3.0f, 2.0f, 4.0f,
      0.15f, 0.20f, 0.10f,
      0.60f, 0.75f, 1.8f },
};
