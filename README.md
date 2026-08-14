# MSI Claw 8 EX AI+ Linux fixes

Experimental Arch Linux and CachyOS fixes for the **MSI Claw 8 EX AI+
CG3EM**, board **MS-1T91**.

> [!WARNING]
> These fixes are hardware-specific. The Claw 8 AI+ A2VM (non-EX) uses a
> different platform and audio path and is not supported by this repository.

## Project purpose and maturity

This is an **unofficial, experimental device-enablement and engineering
reference project**. It has two complementary goals:

1. give Claw 8 EX AI+ CG3EM owners a reproducible way to bring up currently
   missing audio, firmware-control, and performance-overlay support; and
2. turn the device observations into narrowly scoped, reviewable evidence that
   can help inform durable fixes in the upstream Linux kernel and ALSA stack,
   SteamOS Manager, MangoHud, and Arch Linux/CachyOS packaging.

The repository is not an official MSI, Valve, Arch Linux, or CachyOS project,
and inclusion here does not mean that a fix has been accepted by any of those
projects. Experimental means that each documented result has a specific tested
environment and validation boundary; it does not imply production support for
every kernel or distribution release. See the [engineering
reference](ENGINEERING.md) for provenance, observed evidence, open tests, and
the intended upstream or retirement path for each workaround.

Local workarounds are bridges, not permanent forks. They should be removed when
an official kernel, SteamOS, MangoHud, or distribution package provides an
equivalent verified fix.

## Compatibility

| Target | Current compatibility | Intended use |
| --- | --- | --- |
| MSI Claw 8 EX AI+ CG3EM / MS-1T91 | Exact supported hardware; fixes are DMI-gated and live-tested | Device bring-up and continued validation |
| CachyOS | Verified reference environment on the kernel listed below | Current experimental installation path and primary validation baseline |
| Arch Linux | Compatible design target using standard `PKGBUILD`, Pacman, and DKMS tooling; a complete vanilla-Arch pass is not yet recorded | Testing with headers that exactly match the running kernel, plus feedback suitable for proper distribution integration |
| SteamOS | Engineering and upstream reference target; these local packages are not presented as a supported SteamOS overlay | Inform native kernel and SteamOS Manager support, then use the official implementation when released |
| Other Arch-derived distributions | Unverified and best-effort | Evaluation only with matching kernel headers |
| Claw 8 AI+ A2VM/non-EX or other devices | Unsupported | Do not install these hardware-specific fixes |

## Start here

For a first installation, follow the
**[complete getting-started guide](GETTING_STARTED.md)**. It confirms the exact
hardware, selects matching kernel headers, installs the fixes in a safe order,
uses one reboot, verifies every component, and provides update and rollback
steps.

This repository does not install an operating system, select a kernel, modify a
bootloader, or partition storage. The packages have been tested on CachyOS with
kernel `7.2.0-rc7-1-cachyos-rc`; other Arch-family kernels require their own
matching headers.

## Available fixes

| Area | Fix | Recommendation | Status |
| --- | --- | --- | --- |
| Audio | [RT721 SoundWire power workaround](fixes/audio-rt721/) | Install for normal use | Speakers and microphone working; see the idle-power limitation |
| Platform | [SteamOS MSI platform-controls backport](fixes/platform-controls/) | Install for TDP, profiles, fans, and charge limits | Runtime verified on the exact device |
| Telemetry | [GameScope/MangoHud telemetry package](fixes/mangohud-telemetry/) | Optional; install for corrected overlay data | Live sensor paths verified; native foreground overlay capture pending |
| Audio development | [RT721 in-driver power-management patch](patches/rt721-power-management/) | Kernel builders only; alternative to the workaround | Cold boot, runtime PM, and speaker resume verified; clean microphone-resume test pending |

The normal-user route is the packaged audio workaround, platform-controls DKMS
backport, and optional MangoHud package. The experimental RT721 kernel patch is
an upstream-development path, not an additional package to layer on top.

## What the fixes provide

- Internal speaker and microphone power sequencing for the RT721 SoundWire
  codec.
- MSI firmware interfaces for the `custom` performance profile, PL1/SPL
  8–35 W, PL2/SPPT 9–45 W, both fan curves, fan tachometers, and the battery
  charge threshold.
- Corrected GameScope battery discharge watts and remaining time, Intel CPU
  package and GPU uncore power, and focused-game Xe shared-memory residency in
  the VRAM row.
- Both MSI WMI fan tachometers, CPU package temperature, and Intel PMT's
  documented Panther Lake graphics temperature in the detailed overlay.

CPU scaling and boost remain the kernel's `intel_pstate` responsibility. Intel
Xe clock control remains a SteamOS Manager function. GPU utilization remains a
focused-game, per-client Xe metric rather than whole-system utilization.

## Project layout

- [`GETTING_STARTED.md`](GETTING_STARTED.md) — end-to-end install, validation,
  maintenance, troubleshooting, and removal
- [`ENGINEERING.md`](ENGINEERING.md) — formal problem statements, solution
  records, evidence, upstream provenance, and contribution criteria
- [`fixes/audio-rt721/`](fixes/audio-rt721/) — packaged temporary audio
  workaround and technical notes
- [`fixes/platform-controls/`](fixes/platform-controls/) — SteamOS
  `msi-wmi-platform` backport and read-only status helper
- [`fixes/mangohud-telemetry/`](fixes/mangohud-telemetry/) — patched MangoHud
  package and telemetry status helper
- [`patches/rt721-power-management/`](patches/rt721-power-management/) —
  experimental kernel-driver integration

Engineers evaluating or upstreaming a change should begin with the
[engineering reference](ENGINEERING.md), which assigns stable identifiers to
each solution and separates observed evidence, implementation claims, open
tests, and retirement conditions.

## Scope

This repository contains source code and configuration written for Linux. It
does not contain Windows or Realtek driver binaries, decoded vendor data,
firmware, or recordings. All hardware-specific changes are gated on the exact
EX model identifiers.

## License

GPL-2.0-only, except the MangoHud-derived telemetry fix, which retains
MangoHud's MIT license. See [LICENSE](LICENSE) and
[fixes/mangohud-telemetry/LICENSE](fixes/mangohud-telemetry/LICENSE).
