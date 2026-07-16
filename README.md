<h1 align="center">NickelHome</h1>

<p align="center">Customize your Kobo home screen to be more minimalist.</p>

**NickelHome** is a small mod for Kobo eReaders that hooks into the home screen and hides specific built-in widgets (certain columns and rows on the home screen) based on a simple configuration file.

It is built on [NickelHook](https://github.com/pgaskin/NickelHook) and is a standalone sibling of [NickelMenu](https://pgaskin.net/NickelMenu). The home-screen hiding feature started as an [experimental addition](https://github.com/pgaskin/NickelMenu/pull/227) to my NickelMenu fork. Because it may not get merged into NickelMenu (out of scope), I've decided to also offer it as a standalone option here. Depending on what happens with NickelMenu, either [my fork](https://github.com/nicoverbruggen/NickelMenu) or this package may eventually be deprecated.

## What it does

NickelHome hides home-screen widgets by their internal name. The lookups are scoped so other views that reuse the same names aren't affected.

| Config option                | Hides                                                                      |
|------------------------------|---------------------------------------------------------------------------|
| `hide_home_row1col2_enabled` | The dynamic slot next to your current read.                                |
| `hide_home_row2col2_enabled` | The dynamic slot next to "My Books".                                       |
| `hide_home_row2_enabled`     | The entire second row, including "My Books" and the content next to it.   |
| `hide_home_row3_enabled`     | The entire third row, which usually displays notices and CTAs.            |

The two "dynamic slots" are the spots Kobo fills with a collection, author, wishlist, related reads, recommendations, or Top Picks / store tile ("Top 50", "Coming soon to audiobooks", ...). Hiding a slot also stops it from being filled in the first place, so content arriving late (e.g. a Top Picks tile refreshed by a store sync) can't pop back up in a slot you've hidden.

## Compatibility

This needs firmware **4.23.15505 or newer** (the version that introduced the current home-screen layout), and works on the Kobo 4.x firmware line only. If a widget doesn't exist on your firmware, it's skipped and a note is written to the log. Support for the 5.x firmware line (a completely different system) is not planned.

## Is it safe?

- NickelHome only hides home-screen widgets; nothing else on your device is changed.
- If a widget can't be found on your firmware, it's simply left alone.
- If Nickel ever fails to start, a failsafe automatically uninstalls the mod, so it can't be the cause of a boot loop. You can also trigger it manually by powering the Kobo off right after it starts booting.
- It removes itself cleanly (see below), leaving your Kobo as it was.

## Installation

Download `KoboRoot.tgz` from the [latest release](../../releases/latest), copy it into the hidden `.kobo` folder on your Kobo, then eject. It installs on the next reboot.

## Configuration

**You don't need to configure anything.** Out of the box, NickelHome applies a "minimal" home screen that hides everything except your current read and your "My Books" widget. Just install it and reboot.

The defaults ship as a template at `KOBOeReader/.adds/nickel-home/default` (refreshed on every update). On first boot, if no config exists yet, NickelHome copies that template to `KOBOeReader/.adds/nickel-home/config`:

```
# The recommendation column next to your current read.
hide_home_row1col2_enabled:1

# The content next to "My Books".
hide_home_row2col2_enabled:1

# The entire third row, which usually displays notices and CTAs.
hide_home_row3_enabled:1
```

To change what's hidden, edit `config`: set an option to `0` to keep that element, or add `hide_home_row2_enabled:1` to also hide the entire second row (including "My Books"). Two more settings are available: `nhm_enabled:0` turns the mod off without uninstalling it, and `nhm_log:1` enables verbose logging.

Each line is `key:val`, with `#` for comments. Spaces around fields are ignored, and if an option is declared more than once, the first declaration wins. The full documentation is installed to `.adds/nickel-home/doc`. **A reboot is required for changes to take effect.**

## Reporting a problem

If something doesn't look right, NickelHome keeps a small log file that makes it much easier to help you. Connect your Kobo to a computer over USB and look for:

```
KOBOeReader/.adds/nickel-home/nickel-home.log
```

Attach that file when you report an issue. It records which version of the mod you're running, your Kobo's firmware, and which widgets were hidden, and it normally stays short. If you're asked for more detail, open the `config` file in the same folder, add the line `nhm_log:1`, reboot, and try again.

## Building

Needs podman or Docker; see [CONTRIBUTING.md](CONTRIBUTING.md) for the full instructions. In short: clone with `--recursive` and run `./build.sh` to produce `KoboRoot.tgz`.

## Uninstalling

Delete the entire `.adds/nickel-home/` folder, then reboot. NickelHome treats the missing configuration folder as an uninstall request and removes its installed library on the next startup. You can also create an empty file named `uninstall` in `.adds/nickel-home/`, then reboot.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports need the log file; the issue form asks for it.

## Credits & license

NickelHome stands entirely on the shoulders of [Patrick Gaskin](https://github.com/pgaskin)'s work, both NickelHook and NickelMenu. Because it is derived from a PR I made for NickelMenu itself, it is licensed under the MIT License; see [LICENSE](./LICENSE).

This mod was created by the author with the help of the following large language models:

- Claude Opus 4.8
- Claude Fable
