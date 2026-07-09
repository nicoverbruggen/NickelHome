# Changelog

## v0.5

### Added

- **Standalone NickelHome release:** hides configurable home-screen widgets without requiring a NickelMenu fork.
- **Minimal default configuration:** hides the recommendation column, the content beside My Books, and the footer row while keeping the current book and My Books visible.
- **Configurable home-screen rows and columns:** each supported widget can be kept or hidden independently from `.adds/nickelhome/config`.
- **Safe uninstall paths:** removing the configuration directory or creating the `uninstall` flag removes the installed library on the next boot; the NickelHook failsafe remains available as a last resort.
- **Automated builds and releases:** GitHub Actions builds `KoboRoot.tgz` and `libnickelhome.so`, and tagged releases use these changelog entries as their release notes.
