# Cloud Reactive Vial

This keymap combines the stock LazyDesigners Cloud layout with Vial and a
single-color, column-reactive RGB light-bar effect.

The effect is the default after RGBLight EEPROM is initialized or cleared. If
the keyboard already has saved lighting settings, Vial preserves them. Assign
the keyboard-specific **Reactive RGB** action to any key or encoder direction
to enable the LEDs, select the reactive effect, and save that selection.

Vial's standard lighting controls continue to set hue, saturation, brightness,
and on/off state. Animation timing, keypress energy, fade, and held-key levels
are intentionally compiled into the firmware rather than exposed as settings.
Internally, the reactive renderer owns QMK's RGB Test mode slot, so Vial's
standard lighting screen may still describe the selected mode as "RGB Test."

Build with:

```sh
qmk compile -kb lazydesigners/cloud -km reactive_vial
```

Flash with:

```sh
qmk flash -kb lazydesigners/cloud -km reactive_vial
```
