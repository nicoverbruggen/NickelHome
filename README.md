<h1 align="center">NickelHome</h1>

<p align="center">Customize your Kobo home screen to be more minimalist.</p>

**NickelHome** is a small mod for Kobo eReaders that hooks into the home screen
(`HomePageView`) and hides specific built-in widgets — recommendation columns,
promo/quick-tour cards, the footer row, and so on — based on a simple
configuration file.

It is built on [NickelHook](https://github.com/pgaskin/NickelHook) and is a
standalone sibling of [NickelMenu](https://pgaskin.net/NickelMenu). The
home-screen hiding feature originated as an experimental addition to a
NickelMenu fork; since it isn't menu injection, it lives here on its own.

## Features

NickelHome hides home-screen widgets by their internal Qt object name. The hooks
are scoped within `mainContainer` so other views that reuse the same leaf object
names aren't affected.

| Config option                | Hides                                                                      |
|------------------------------|---------------------------------------------------------------------------|
| `hide_home_row1col2_enabled` | The recommendation column next to your current read.                      |
| `hide_home_row2col2_enabled` | The content next to "My Books".                                           |
| `hide_home_row2_enabled`     | The entire second row, including "My Books" and the content next to it.   |
| `hide_home_row3_enabled`     | The entire third row, which usually displays notices and CTAs.            |

This requires firmware 4.23.15505+ (the version that introduced the current home
screen layout). If a widget doesn't exist on your firmware, it's silently
skipped and a warning is written to syslog (`logread`).

## Configuration

NickelHome installs a default template to `KOBOeReader/.adds/nickelhome/default`
(refreshed on every update). On first boot, if no config exists yet, it copies
that template to `KOBOeReader/.adds/nickelhome/config` with every option disabled:

```
# NickelHome configuration file

# The recommendation column next to your current read.
hide_home_row1col2_enabled:0

# The content next to "My Books".
hide_home_row2col2_enabled:0

# The entire second row, including "My Books" and the content next to it.
hide_home_row2_enabled:0

# The entire third row, which usually displays notices and CTAs.
hide_home_row3_enabled:0
```

Set an option to `1` to hide it. Each line is `key:val`, with `#` for comments;
spaces around fields are ignored, and if an option is declared more than once,
the first declaration wins. A reboot is required for changes to take effect. The
full documentation is installed to `.adds/nickelhome/doc`.

## Installation

Download `KoboRoot.tgz` from the build artifacts, copy it into the `.kobo` folder
of your eReader, then eject. It installs on the next reboot.

## Uninstalling

Create an empty file named `uninstall` in `.adds/nickelhome/`, then reboot. As a
last resort you can trigger the failsafe mechanism by immediately powering off
the Kobo right after it starts booting.

## Compiling

NickelHome is designed to be compiled with
[NickelTC](https://github.com/pgaskin/NickelTC). Clone with submodules
(`git clone --recurse-submodules`), then:

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

NickelHome stands entirely on the shoulders of
[Patrick Gaskin](https://github.com/pgaskin)'s work — NickelHook provides the
hooking machinery and NickelMenu is where the home-screen hiding feature first
grew. Licensed under the MIT License; see [LICENSE](./LICENSE).
