# Contributing to NickelHome

Technical guide for building, testing, and changing the mod. These mods follow the shared conventions in [NickelGuidance](https://github.com/nicoverbruggen/NickelGuidance).

## Building

Build both targets with the public [kobuild](https://github.com/nicoverbruggen/kobuild) SDK images. Docker is the default; set `CONTAINER_ENGINE=podman` to use Podman. The build script pins the Qt5 and Qt6 images from v0.5.3 by digest.

```sh
git clone --recursive https://github.com/nicoverbruggen/NickelHome
cd NickelHome
./build.sh
```

Outputs are `build/qt5/KoboRoot.tgz` and `build/qt6/Kobo.tgz`, with `libnickelhome.so` beside each package. Use `./build.sh --qt 5` or `--qt 6` for one target. `JOBS` controls compiler parallelism. Each target builds a fresh source copy to avoid mixing Qt objects.

The version comes from `git describe --tags --always --dirty`. Use `--version VERSION` for a local test stamp. CI checks out full history. Do not add private tool paths or development-platform references to source or documentation.

The Makefile selects `src/nickelhome.cc` for Qt5 and `src/home-qt6.cc` plus `findbooks.cc` for Qt6. Both compile `src/mybooks.cc` to keep the native My Books tile available with fewer than three covers, and use the same config parser and keys. Qt6 builds embed the support files with NickelHook's resource installer; only `imageformats/libnickelhome.so` goes in `Kobo.tgz`. Neither target compiles an interactive Settings page.

Run `python3 -m unittest discover -s test -p 'test_*.py'` for host package checks. Every build also validates archive paths, ownership, permissions, ARM hard-float ABI, Qt dependencies and the absence of Settings hooks. These checks do not establish device compatibility.

## Testing on a device

1. Copy the matching `KoboRoot.tgz` (Qt5) or `Kobo.tgz` (Qt6) into the Kobo's hidden `.kobo` folder over USB.
2. Eject and reboot; the firmware installs it and deletes the tgz.
3. The mod's folder is `KOBOeReader/.adds/nickel-home/` (`doc`, `default`, `config`, and once it logs, `nickel-home.log`).

Boot safety / recovery: NickelHook's failsafe (`failsafe_delay = 3`) uninstalls the mod if Nickel crashes within ~3 s of boot; power off within that window to recover a bad build. Deleting the whole `.adds/nickel-home/` folder (or creating an empty `uninstall-now` file in it) and rebooting also removes it.

For My Books changes, check zero, one, two and three available books, including removal and reimport. Verify the tile hides when it has no books to show, returns when books are available, opens the library, and is never replaced by cached store promotions. Repeat with `nhm_enabled:0` to confirm stock behavior, and with the whole row hidden (plus My Books alone on Qt6) to confirm hiding still works.

## Logs & debugging

The mod logs to `KOBOeReader/.adds/nickel-home/nickel-home.log` (and to syslog via `nh_log`, viewable with `logread`). Every message carries the mod version; the startup block logs the mod version, the firmware version, the effective config, and whether the home-screen hook resolved. A healthy boot stays short. Set `nhm_log:1` in the config for verbose tracing (a malformed config turns it on automatically so mistakes self-diagnose). The log is size-capped (256 KB) and rotates once to `nickel-home.log.old`.

## Firmware compatibility

The hooked `libnickel` symbol (`HomePageView::HomePageView`) carries a `//libnickel <first> <last|*> <symbol>` annotation. The `test/syms` checker (CI job `syms`, also runnable locally with Go: `cd test/syms && go build -o ../../test.syms . && cd ../src && ../test.syms`) verifies it against ~70 real firmware dumps (4.6 → 4.45). The current home-screen layout was introduced in firmware 4.23.15505, which is the effective floor. The hook is `.optional`: on firmware where the symbol is missing, the mod loads inert instead of failing. That checker covers Qt5 only. Qt6 uses a required Home constructor hook and optional content/navigation hooks; test the package on its intended firmware. The config parser is read-only after startup, so changes require a reboot.

## Pull requests

- Add an entry to `CHANGELOG.md` under the heading for the version it will ship in, for any user-visible change (release notes are generated from it).
- Annotate any new `libnickel` symbol with `//libnickel …`; CI verifies it.
- State the device + firmware you tested on, and attach the relevant `nickel-home.log` excerpt (the PR template asks for both).

## Releases (maintainers)

Add release notes to `CHANGELOG.md` under the intended tag name before tagging. The manual Release workflow builds an existing tag, validates both packages and attaches them to one draft release. It refuses a missing or empty changelog section. Review and test both packages before publishing. Building locally does not publish a release.
