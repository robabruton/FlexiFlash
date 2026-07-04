# Firmware

The firmware is an ESP-IDF scaffold with two application variants sharing
common status/result primitives, product-wide build limits, and a board-profile
component.

| Path | Role |
| --- | --- |
| `apps/handheld/` | ESP-IDF project for the handheld variant |
| `apps/desktop/` | ESP-IDF project for the benchtop station variant |
| `components/common/` | Shared status and result primitives |
| `components/config/` | Product-wide build limits shared by both app variants |
| `components/board/` | Active board identity and capability contract |
| `sdkconfig.defaults.common` | Configuration shared by both apps |

Each app has its own `main/`, partition table, and `sdkconfig.defaults`. Both
apps reference `firmware/components/` through `EXTRA_COMPONENT_DIRS`; the
shared component tree contains common firmware primitives, product-wide build
limits, and the `board` component used by both apps.

## Building

Run `idf.py` from the app directory after loading the ESP-IDF environment:

```sh
cd firmware/apps/handheld
idf.py build
```

Use `firmware/apps/desktop` for the desktop build. Each app selects its board
profile through its variant-specific `sdkconfig.defaults`.

Bench bring-up on development hardware stays out of the tracked app projects.
Keep local development-board configuration under `local/`.
