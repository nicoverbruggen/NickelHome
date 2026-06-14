<h1 align="center">NickelHome</h1>

<p align="center">Customize your Kobo home screen to be more minimalist.</p>

**NickelHome** is a small mod for Kobo eReaders that hooks into the home screen and hides specific built-in widgets (specific columns and rows on the homescreen) based on a simple configuration file.

It is built on [NickelHook](https://github.com/pgaskin/NickelHook) and is a standalone sibling of [NickelMenu](https://pgaskin.net/NickelMenu). The home-screen hiding feature originated as an [experimental addition](https://github.com/pgaskin/NickelMenu/pull/227) to my NickelMenu fork. Because it may not get merged into NickelMenu (out of scope) I've made the decision to also provide it as a standalone option here.

In the future, depending on what happens with NickelMenu, either [my fork](https://github.com/nicoverbruggen/NickelMenu) or this package may be deprecated.

## Features

NickelHome hides home-screen widgets by their internal Qt object name. The hooks are scoped within `mainContainer` so other views that reuse the same names aren't affected.

| Config option                | Hides                                                                      |
|------------------------------|---------------------------------------------------------------------------|
| `hide_home_row1col2_enabled` | The recommendation column next to your current read.                      |
| `hide_home_row2col2_enabled` | The content next to "My Books".                                           |
| `hide_home_row2_enabled`     | The entire second row, including "My Books" and the content next to it.   |
| `hide_home_row3_enabled`     | The entire third row, which usually displays notices and CTAs.            |

This requires firmware 4.23.15505+ (the version that introduced the current home screen layout). If a widget doesn't exist on your firmware, it's silently skipped and a warning is written to syslog (`logread`).

## Configuration

**You don't need to configure anything.** Out of the box, NickelHome applies a "minimal" home screen that hides everything except your current read and your My Books widget. Just install it and reboot.

These defaults ship as a template installed to `KOBOeReader/.adds/nickelhome/default` (refreshed on every update).

On first boot, if no config exists yet, NickelHome copies that template to `KOBOeReader/.adds/nickelhome/config`:

```
#
# NickelHome configuration file
#
# This is the default configuration file, it is the default "minimal" configuration
# that hides everything but your current read and the "My Books" widget.
#

# The recommendation column next to your current read.
hide_home_row1col2_enabled:1

# The content next to "My Books".
hide_home_row2col2_enabled:1

# The entire third row, which usually displays notices and CTAs.
hide_home_row3_enabled:1
```

To change what's hidden, edit `config`: set an option to `0` to keep that
element, or add `hide_home_row2_enabled:1` to also hide the entire second row
(including My Books). 

Each line is `key:val`, with `#` for comments; spaces around fields are ignored, and if an option is declared more than once, the first declaration wins. The full documentation is installed to `.adds/nickelhome/doc`.

**A reboot is required for changes to take effect.**

## Installation

Download `KoboRoot.tgz` from the build artifacts, copy it into the `.kobo` folder of your eReader, then eject. It installs on the next reboot.

## Uninstalling

Delete the entire `.adds/nickelhome/` folder, then reboot. NickelHome treats the missing configuration folder as an uninstall request and removes its installed library on the next startup.

You can also create an empty file named `uninstall` in `.adds/nickelhome/`, then reboot. As a last resort you can trigger the failsafe mechanism by immediately powering off the Kobo right after it starts booting. This is also an additional safeguard to prevent NickelHome from being the source of a boot loop.

## Compiling

Like NickelMenu, NickelHome is designed to be compiled with [NickelTC](https://github.com/pgaskin/NickelTC). Make sure to clone the project with its submodule (`git clone --recurse-submodules`), then:

- With Docker/Podman:
  `docker run --volume="$PWD:$PWD" --user="$(id -u):$(id -g)" --workdir="$PWD" --env=HOME --entrypoint=make --rm -it ghcr.io/pgaskin/nickeltc:1.0 all koboroot`
- On the host:
  `make CROSS_COMPILE=/path/to/nickeltc/bin/arm-nickel-linux-gnueabihf- all koboroot`
- Local Podman wrapper: `./build.sh` (defaults to `clean all koboroot`; you can
  pass alternate make targets, e.g. `./build.sh all`).

To run the symbol-presence tests (no device needed):

```
cd test/syms && go build -o ../../test.syms . && cd ../../src && ../test.syms
```

## Credits

NickelHome stands entirely on the shoulders of [Patrick Gaskin](https://github.com/pgaskin)'s work, both NickelHook and NickelMenu. Because it is derived from a PR I made to be added to NickelMenu itself, this is also licensed under the MIT License; see [LICENSE](./LICENSE).
