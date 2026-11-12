<h1 align="center">NickelHome</h1>

<p align="center">Customize your Kobo home screen to be more minimalist.</p>

**NickelHome** is a small mod for Kobo eReaders that hooks into the home screen and hides specific built-in widgets (certain columns and rows on the home screen) based on a simple configuration file.

It is built on [NickelHook](https://github.com/pgaskin/NickelHook) and is a standalone sibling of [NickelMenu](https://pgaskin.net/NickelMenu). The home-screen hiding feature started in my NickelMenu fork and is offered here as a standalone option.

> [!IMPORTANT]
> Older NickelMenu forks also hid Home widgets and can conflict with NickelHome. Use standard NickelMenu on firmware 4, or the current Qt6 NickelMenu fork on firmware 5/6. Do not install two mods that change the same Home widgets.

## What it does

NickelHome hides home-screen widgets by their internal name. The lookups are scoped so other views that reuse the same names aren't affected.

| Config option                | Hides                                                                      |
|------------------------------|---------------------------------------------------------------------------|
| `hide_home_row1col2_enabled` | The dynamic slot next to your current read.                                |
| `hide_home_row2col2_enabled` | The dynamic slot next to "My Books".                                       |
| `hide_home_row2col1_enabled` | My Books, without hiding the tile beside it (Qt6 only). |
| `hide_home_row2_enabled`     | The entire second row, including "My Books" and the content next to it.   |
| `hide_home_row3_enabled`     | The entire third row, which usually displays notices and CTAs.            |

The two "dynamic slots" are the spots Kobo fills with a collection, author, wishlist, related reads, recommendations, or Top Picks / store tile ("Top 50", "Coming soon to audiobooks", ...). On firmware 4, hiding a slot prevents it from being filled. On firmware 5/6, Nickel still updates its content and NickelHome reapplies visibility after those updates.

My Books stays available with one or two books to show. It hides when there are no books to show and returns when books are available. Store promotions cannot replace it while NickelHome is enabled. Explicitly hiding My Books or its entire row still takes precedence.

## Compatibility

Use the package that matches your firmware:

| Firmware | Qt | Package |
| --- | --- | --- |
| Kobo 4.23.15505 or newer on the 4.x line | Qt5 | `KoboRoot.tgz` |
| Kobo 6.0 _and_ Tolino 5.18 | Qt6 | `Kobo.tgz` |

**Qt6 support is experimental.** These packages contain different binaries and archive paths; do not rename them. Missing widgets are left unchanged. A missing required Home hook prevents the Qt6 plugin from loading.

## Is it safe?

- NickelHome changes Home widgets and, on Qt6, the reading prompt shortcut. It does not change your book files.
- If a widget can't be found on your firmware, it's simply left alone.
- NickelHook has a startup failsafe. It does not guarantee recovery from every failure; back up important data before installing experimental mods.
- It removes itself cleanly (see below), leaving your Kobo as it was.

## Installation

Download the matching package from the [latest release](../../releases/latest), copy it into the hidden `.kobo` folder on your Kobo, then eject. It installs on the next reboot.

## Configuration

**You don't need to configure anything.** Out of the box, NickelHome applies a "minimal" home screen that hides everything except your current read and your "My Books" widget. Just install it and reboot.

The defaults live at `KOBOeReader/.adds/nickel-home/default`. The Qt5 package refreshes this template on installation. The Qt6 library creates missing support files on first startup and preserves existing files on later updates. If no config exists yet, NickelHome copies the template to `KOBOeReader/.adds/nickel-home/config`:

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

There is no interactive Settings page in either package yet. To make changes, edit the config file and reboot. A settings integration with NickelModManager is planned separately.

## Reporting a problem

If something doesn't look right, NickelHome keeps a small log file that makes it much easier to help you. Connect your Kobo to a computer over USB and look for:

```
KOBOeReader/.adds/nickel-home/nickel-home.log
```

Attach that file when you report an issue. It records which version of the mod you're running, your Kobo's firmware, and which widgets were hidden, and it normally stays short. If you're asked for more detail, open the `config` file in the same folder, add the line `nhm_log:1`, reboot, and try again.

## Building

Needs podman or Docker; see [CONTRIBUTING.md](CONTRIBUTING.md) for the full instructions. In short: clone with `--recursive` and run `./build.sh` to produce `build/qt5/KoboRoot.tgz` and `build/qt6/Kobo.tgz`.

## Uninstalling

Delete the `uninstall` file in `.adds/nickel-home/`, then reboot. NickelHome treats that file going missing as an uninstall request: on the next startup it removes its installed library and the whole `.adds/nickel-home/` folder. You can also create an empty file named `uninstall-now` in the same folder, or delete the entire folder, then reboot.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports need the log file; the issue form asks for it.

## Credits & license

NickelHome stands entirely on the shoulders of [Patrick Gaskin](https://github.com/pgaskin)'s work, both NickelHook and NickelMenu. Because it is derived from a PR I made for NickelMenu itself, it is licensed under the MIT License; see [LICENSE](./LICENSE).

This mod was created by the author with the help of the following large language models: Claude Opus 5.5, Claude Fable 5, GPT-6 Astra, and some old-school manual work as well.
