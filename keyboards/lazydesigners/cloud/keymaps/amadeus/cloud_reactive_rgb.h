// Copyright 2026 Amadeus
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// These defaults make the module directly reusable by another Cloud keymap.
// Override any of them in config.h to tune the effect.
#ifndef CLOUD_REACTIVE_FRAME_INTERVAL_MS
#    define CLOUD_REACTIVE_FRAME_INTERVAL_MS 16U
#endif

#ifndef CLOUD_REACTIVE_SPEED_FEEDBACK_MS
#    define CLOUD_REACTIVE_SPEED_FEEDBACK_MS 1000U
#endif

#ifndef CLOUD_REACTIVE_HIT_INCREMENT
#    define CLOUD_REACTIVE_HIT_INCREMENT 160U
#endif

#ifndef CLOUD_REACTIVE_NEIGHBOR_INCREMENT
#    define CLOUD_REACTIVE_NEIGHBOR_INCREMENT 56U
#endif

#ifndef CLOUD_REACTIVE_FADE_STEP
#    define CLOUD_REACTIVE_FADE_STEP 3U
#endif

// A held key settles from its initial pulse to these steady energy levels. Its
// normal fade resumes from here as soon as the last key in that column releases.
#ifndef CLOUD_REACTIVE_HOLD_LEVEL
#    define CLOUD_REACTIVE_HOLD_LEVEL 96U
#endif

#ifndef CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL
#    define CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL 32U
#endif

// Once the last keypress has faded to CLOUD_IDLE_BOUNCE_HANDOFF_SCALE/256 of the
// brightness it landed with, and no keys are held, its brightest pixel relights
// and swings across the bar like a pendulum until input resumes or the idle
// timeout turns the lights off. Higher hands off sooner. Apart from the handoff
// itself, none of these follow the RGB speed setting.
#ifndef CLOUD_IDLE_BOUNCE_HANDOFF_SCALE
#    define CLOUD_IDLE_BOUNCE_HANDOFF_SCALE 160U
#endif

// Brightness of the trail the dot leaves behind, out of 256 of the dot's own.
#ifndef CLOUD_IDLE_BOUNCE_TRAIL_SCALE
#    define CLOUD_IDLE_BOUNCE_TRAIL_SCALE 128U
#endif

#ifndef CLOUD_IDLE_BOUNCE_SWEEP_MS
#    define CLOUD_IDLE_BOUNCE_SWEEP_MS 1500U
#endif

#ifndef CLOUD_IDLE_BOUNCE_RAMP_MS
#    define CLOUD_IDLE_BOUNCE_RAMP_MS 250U
#endif

// Only used when RGBLIGHT_IDLE_TIMEOUT_MS is defined.
#ifndef CLOUD_IDLE_BOUNCE_FADE_OUT_MS
#    define CLOUD_IDLE_BOUNCE_FADE_OUT_MS 500U
#endif

// Center energy of the moving dot. Its glow reaches half this one LED away.
#ifndef CLOUD_IDLE_BOUNCE_LEVEL
#    define CLOUD_IDLE_BOUNCE_LEVEL 112U
#endif

// Fraction of the trail kept each frame, out of 256. Higher is a longer trail.
#ifndef CLOUD_IDLE_BOUNCE_TRAIL_DECAY
#    define CLOUD_IDLE_BOUNCE_TRAIL_DECAY 216U
#endif

// Every keypress sends a dot from the current light (the last key, the bounce,
// or a dot still in flight) to the new key at a constant speed, and the hit
// lands when it arrives. This is the time to cross the whole bar.
#ifndef CLOUD_REACTIVE_DASH_FULL_SPAN_MS
#    define CLOUD_REACTIVE_DASH_FULL_SPAN_MS 150U
#endif

#ifndef CLOUD_REACTIVE_REVERSE_LED_ORDER
#    define CLOUD_REACTIVE_REVERSE_LED_ORDER 0
#endif

void cloud_reactive_rgb_activate(const keyrecord_t *record);
void cloud_reactive_rgb_process_record(uint16_t keycode, const keyrecord_t *record);
void cloud_reactive_rgb_task(void);
