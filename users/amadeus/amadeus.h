// Copyright 2026 Amadeus
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

enum amadeus_tap_dances {
    TD_FLASH,
    AMADEUS_TAP_DANCE_COUNT,
};

extern tap_dance_action_t tap_dance_actions[AMADEUS_TAP_DANCE_COUNT];
