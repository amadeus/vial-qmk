# Quick Notes for Future Self

If updating the firmware from stable, make sure to first do a `git submodule
update` to make sure you on the latest submodules.

I have 2 more LAZYDESIGNER keyboards coming soon (beyond the 2 already here), so
I will be outfitting this with more keymaps as time goes on.

Firmwares can be built via make commands:

```bash
make lazydesigners/cloud:amadeus
make lazydesigners/duo:amadeus
```

Layouts are configured separately but in the same branch so AI can easily
compare things and ensure similarities where applicable.

I have, for the most part, removed any of the vial/via layout functionality.
Basically I'm pretty stingey with my keymap and it allows me to keep the
firmware install as lean as possible.

## Various Features That I Like

* Caps Word with double shift is also a jam that I've been using
* A special 5 tap fancy boot mode on enter in adjust
  * Not easy to activate and less annoying since I don't have to take the
    keyboard apart
* The eager debounce shit
  * Can cause chatter sometimes, but I dunno, the placebo is nice
* I have a very opinionated MIT layout.  I've had to tweak and adapt things a
  bit for the Duo since it doesn't have an 11key bottom row
