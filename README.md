# FlexiFlash

FlexiFlash is a network-connected microcontroller programmer built around a
shared ESP-IDF firmware codebase. The firmware currently has two application
variants: a handheld unit and a benchtop station.

## Firmware

The firmware scaffold lives under `firmware/`:

- `firmware/apps/handheld/` builds the handheld variant.
- `firmware/apps/desktop/` builds the benchtop station variant.
- `firmware/components/board/` provides the active board profile and capability
  contract shared by both apps.

Build from an app directory with ESP-IDF available in the shell:

```sh
cd firmware/apps/handheld
idf.py build
```

See `firmware/README.md` for the firmware layout and board-profile details.

## License

FlexiFlash is released under the FlexiFlash Personal Review License, which
permits personal review and experimentation only. See [LICENSE.md](LICENSE.md)
for the full terms.
