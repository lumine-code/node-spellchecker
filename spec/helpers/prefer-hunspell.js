"use strict";

// Most of the suite exercises Hunspell on every platform, which the native
// module selects from this variable. It has to be set before the addon is
// loaded, so it lives in a helper: Jasmine runs helpers before it loads any
// spec file, and setting it at the top of one spec file only works for as long
// as no other spec file sorts ahead of it.
//
// Cases that want the platform's own service ask for it explicitly with
// `ALWAYS_USE_SYSTEM`, which overrides this.
process.env.SPELLCHECKER_PREFER_HUNSPELL = "1";
