# MSI Claw 8 EX AI+ Linux fixes

My journey getting Arch Linux and CachyOS working on the **MSI Claw 8 EX AI+
CG3EM**, board **MS-1T91**.

> [!WARNING]
> This repository supports only the Claw 8 EX AI+ CG3EM / MS-1T91. The Claw 8
> AI+ A2VM (non-EX) has different hardware and is not supported.

## About this project

This repository collects the fixes that worked for me. I am sharing them to
give other Claw EX owners a practical starting point and to provide useful
evidence for proper Linux, SteamOS, MangoHud, Arch Linux, and CachyOS support.

This is an unofficial, experimental project. It is not an MSI, Valve, Arch
Linux, or CachyOS product. Results apply to the tested setup described here,
and local workarounds should disappear as official fixes become available.

For test evidence and upstream notes, see [ENGINEERING.md](ENGINEERING.md).

## Compatibility

- **Tested hardware:** MSI Claw 8 EX AI+ CG3EM / MS-1T91.
- **Tested system:** [CachyOS Handheld
  Edition](https://wiki.cachyos.org/installation/installation_handheld/) with
  KDE Plasma.
- **Arch Linux:** The packages use standard Pacman, `PKGBUILD`, and DKMS tools,
  but a complete vanilla Arch test is still pending.
- **SteamOS:** Use this work as a reference, not as a supported SteamOS overlay.
- **Other Arch-based systems:** Best effort; not tested.
- **Other Claw models:** Unsupported.

## Start here

Follow the **[getting-started guide](GETTING_STARTED.md)**. It checks the exact
device, installs the fixes in a safe order, verifies each component, and
includes update and removal steps.

My current software baseline is:

- `7.2.0-rc7-1-cachyos-rc` with matching headers;
- `linux-cachyos-deckify` kept as a fallback; and
- matching 64-bit and 32-bit Mesa/Intel Vulkan `26.2.0` packages.

Mesa is an optional software compatibility update, not a hardware fix. Prefer
current distribution packages and keep the 64-bit and 32-bit versions matched.
Mesa loads automatically. After updating it, reboot or use **Return to Gaming
Mode** (`steamos-session-select gamescope`).

I use `fred=off` for a Dark Souls III Wine/Proton issue. It is not required for
the hardware fixes.

This repository does not install an operating system, change the bootloader,
or partition storage.

## Included fixes

For normal use, install the audio and platform-control packages. Add the
telemetry package if you want corrected performance-overlay data.

- **[Audio](fixes/audio-rt721/):** Restores the internal speakers and
  microphone. Both work, with a documented idle-power limitation.
- **[Platform controls](fixes/platform-controls/):** Exposes TDP limits,
  performance profiles, fan curves, fan speeds, and the battery charge limit.
  These interfaces are live-tested on the exact device.
- **[GameScope/MangoHud telemetry](fixes/mangohud-telemetry/):** Corrects
  battery draw, CPU/GPU power, shared-memory VRAM, fan speeds, and CPU/GPU
  temperatures. The sensor paths are live-tested; a foreground overlay capture
  is still pending.
- **[RT721 kernel patch](patches/rt721-power-management/):** My original audio
  fix, built into the codec driver. It is now the development
  and upstream path; use the packaged workaround for normal use and do not
  install both. Speaker resume works; a clean microphone-resume test is still
  pending.

Linux `intel_pstate` still manages CPU scaling and boost. Intel Xe clock control
needs compatible SteamOS Manager support. GPU utilization shows the focused
game's Xe activity, not whole-system GPU use.

## Scope and license

The repository contains Linux source code and configuration. It does not
include Windows or Realtek binaries, decoded vendor data, firmware, or
recordings. Hardware-specific changes are gated to the exact EX model.

The project is GPL-2.0-only except for the MangoHud-derived work, which retains
MangoHud's MIT license. See [LICENSE](LICENSE) and the [MangoHud
license](fixes/mangohud-telemetry/LICENSE).
