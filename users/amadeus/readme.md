# Amadeus Userspace

Shared configuration and behavior for the `amadeus` keymaps on:

- `lazydesigners/cloud`
- `lazydesigners/duo`
- `lazydesigners/kradlex`
- `lazydesigners/dimpler3`

The userspace enables double-Shift Caps Word, a five-tap bootloader action,
link-time optimization, and per-key asymmetric eager/deferred debouncing with
a 16 ms debounce interval.

Cloud keeps its encoder map enabled in its keymap-specific `rules.mk` because
the other keyboards do not have an encoder.
