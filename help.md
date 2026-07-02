# Quick Notes for Future Self

If updating the firmware from stable, make sure to first do a `git submodule
update` to make sure you on the latest submodules.

I have 2 more LAZYDESIGNER keyboards coming soon (beyond the 2 already here), so
I will be outfitting this with more keymaps as time goes on.

Firmwares are built with the qmk cli (it brings its own toolchains, so plain
`make` won't find arm-none-eabi-gcc for the ARM boards):

```bash
qmk compile -kb lazydesigners/cloud -km amadeus
qmk compile -kb lazydesigners/duo -km amadeus
qmk compile -kb lazydesigners/kradlex -km amadeus
```

Layouts are configured separately but in the same branch so AI can easily
compare things and ensure similarities where applicable.

I have, for the most part, removed any of the vial/via layout functionality.
Basically I'm pretty stingey with my keymap and it allows me to keep the
firmware install as lean as possible.

## Adding a Layout for a New Board

1. Scaffold the keymap from the board's default:

   ```bash
   qmk new-keymap -kb lazydesigners/<board> -km amadeus
   ```

2. Port over the closest existing `keymaps/amadeus/` (kradlex for MIT ortho
   boards, cloud if the board has an encoder/RGB, duo for the weird ones),
   keeping the scaffold's `LAYOUT` macro and key count -- check the board's
   `info.json`/`keyboard.json` for physical key sizes
3. Keep the required features: double shift Caps Word (plus the custom
   `caps_word_press_user` that only shifts letters), the 5 tap `TD_FLASH`
   bootloader on Enter in Adjust, `asym_eager_defer_pk` debounce with
   `DEBOUNCE 16`, and vial/via stripped out of rules.mk
4. Drop anything the hardware doesn't have (encoder maps, RGB keycodes)
   rather than leaving dead keys around
5. Redraw the ASCII art to match the physical key sizes, then build with
   `qmk compile -kb lazydesigners/<board> -km amadeus` and make sure it's
   warning-free

## Various Features That I Like

* Caps Word with double shift is also a jam that I've been using
* A special 5 tap fancy boot mode on enter in adjust
  * Not easy to activate and less annoying since I don't have to take the
    keyboard apart
* The eager debounce shit
  * Can cause chatter sometimes, but I dunno, the placebo is nice
* I have a very opinionated MIT layout.  I've had to tweak and adapt things a
  bit for the Duo since it doesn't have an 11key bottom row
