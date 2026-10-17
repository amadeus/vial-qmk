# Amadeus Userspace

Shared configuration and behavior for the `amadeus` keymaps on:

- `lazydesigners/cloud`
- `lazydesigners/duo`
- `lazydesigners/kradlex`
- `lazydesigners/dimpler3`

The userspace enables double-Shift Caps Word with custom continuation rules,
link-time optimization, and per-key asymmetric eager/deferred debouncing with
a 16 ms debounce interval. The eager press behavior keeps key response feeling
immediate while deferred release handling helps reject chatter.

The Adjust-layer Enter key uses a five-tap tap dance to enter the bootloader.
Requiring five taps makes the action available without opening the case while
keeping it difficult to trigger accidentally.

Cloud keeps its encoder map enabled in its keymap-specific `rules.mk` because
the other keyboards do not have an encoder. RGB configuration also remains in
Cloud's keyboard definition because it is the only current target with RGB.

Build every target with the QMK CLI so its bundled toolchains are available:

```bash
qmk compile -c -kb lazydesigners/cloud -km amadeus
qmk compile -c -kb lazydesigners/duo -km amadeus
qmk compile -c -kb lazydesigners/kradlex -km amadeus
qmk compile -c -kb lazydesigners/dimpler3 -km amadeus
```

Run clean builds sequentially because each `-c` invocation removes the shared
`.build` directory.
