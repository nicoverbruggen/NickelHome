# Changelog

## Unreleased

### Added

- Separate Qt5 and experimental Qt6 builds from the same repository, using pinned kobuild SDK images.
- `Kobo.tgz` for firmware 5.18 and 6.x, alongside `KoboRoot.tgz` for firmware 4.x.
- Qt6 Home rules, an independent My Books hide option and an optional Kobo-style Home layout on Tolino. The Kobo reading prompt opens My Books while the mod is enabled.
- Package validation and a manual release workflow that attaches both packages to one draft release.

### Changed

- Qt6 creates support files on first startup without replacing existing configuration. Neither package includes an interactive Settings page; edit the config and reboot.

## v0.6

### Changed

- **Config folder renamed** from `.adds/nickelhome/` to `.adds/nickel-home/`. Settings are not carried over: the new folder starts from the default configuration, so if you customized your config, copy it into the new folder and delete the old one.

### Fixed

- **Hidden dynamic slots stay hidden after a store sync:** hiding the recommendation column (`row1col2`) or the card beside My Books (`row2col2`) now also stops Kobo from re-filling those slots. Previously a tile refreshed later by a store sync (a Top Picks or "Coming soon to audiobooks" tile) could reappear in a slot you had hidden.

### Added

- **`nhm_enabled` switch:** set it to `0` to leave the home screen untouched without uninstalling the mod.
- **On-device log** at `.adds/nickel-home/nickel-home.log`, recording the mod version, your firmware version, and which widgets were hidden. It stays short on a healthy boot, is size-capped, and rotates to `nickel-home.log.old`. Set `nhm_log:1` for verbose logging.

## v0.5

### Added

- **Standalone NickelHome release:** hides configurable home-screen widgets without requiring a NickelMenu fork.
- **Minimal default configuration:** hides the recommendation column, the content beside My Books, and the footer row while keeping the current book and My Books visible.
- **Configurable home-screen rows and columns:** each supported widget can be kept or hidden independently from `.adds/nickelhome/config`.
- **Safe uninstall paths:** removing the configuration directory or creating the `uninstall` flag removes the installed library on the next boot; the NickelHook failsafe remains available as a last resort.
- **Automated builds and releases:** GitHub Actions builds `KoboRoot.tgz` and `libnickelhome.so`, and tagged releases use these changelog entries as their release notes.
