# MSI Claw 8 EX AI+ Linux fixes

Experimental Arch Linux and CachyOS fixes for the **MSI Claw 8 EX AI+
CG3EM**, board **MS-1T91**.

> [!WARNING]
> These fixes are hardware-specific. The Claw 8 AI+ A2VM (non-EX) uses a
> different platform and audio path and is not supported by this repository.

## Available fixes

| Area | Fix | Status |
| --- | --- | --- |
| Audio | [RT721 in-driver power-management patch](patches/rt721-power-management/) | Cold boot, runtime PM, and speaker resume verified |
| Audio | [Realtek RT721 SoundWire power workaround](fixes/audio-rt721/) | Working fallback; leaves hidden D0 gates enabled |
| Power | [SteamOS MSI platform-controls backport](fixes/platform-controls/) | Runtime verified on CachyOS 7.2-rc7 |
| Telemetry | [GameScope battery and Intel GPU power package](fixes/mangohud-telemetry/) | Installed and RAPL read verified; live GameScope validation pending |

The audio workaround is DMI-gated and refuses to run unless both the product
and board identifiers match the EX model. It is intended as a temporary bridge
until the power sequence can be integrated into the upstream RT721 codec
driver.

The experimental kernel patch now implements that integration. It applies the
model-specific D0 and D3 sequences around the existing RT721 runtime-PM and
DAPM transitions. Keep using the packaged workaround until the patch's
helper-independent microphone resume test is complete.

## Quick start

Install the headers matching the running kernel plus the build tools. For the
currently tested CachyOS release:

```bash
sudo pacman -S --needed base-devel dkms clang linux-cachyos-rc-headers
git clone https://github.com/stevedamnvan/msi-claw-8-ex-linux.git
cd msi-claw-8-ex-linux/fixes/audio-rt721
makepkg -si
sudo systemctl enable --now claw-rt721-fix.service
```

If a different kernel is running, replace `linux-cachyos-rc-headers` with its
matching headers package. See the [audio fix documentation](fixes/audio-rt721/)
for verification, manual installation, removal, and limitations.

Kernel builders can instead test the
[in-driver power-management patch](patches/rt721-power-management/). It is
based on Linux `v7.2-rc7`, whose RT721 source matches CachyOS
`7.2.0-rc7-1-cachyos-rc`.

The [platform-controls DKMS package](fixes/platform-controls/) backports
Valve's pending exact-device support for TDP and performance profiles, both fan
curves, and the battery charge threshold. CPU scaling and Intel Xe GPU clocks
are already exposed by the kernel and SteamOS Manager; the backport supplies
the missing MSI firmware interfaces that tie the remaining controls together.

The [MangoHud telemetry package](fixes/mangohud-telemetry/) corrects the EX
firmware's wrapped battery discharge current for GameScope and adds Intel
integrated-GPU watts through the RAPL uncore counter. GPU utilization remains a
focused-game, per-client Xe metric; the package does not relabel it as
whole-system utilization.

## Scope

This repository contains source code and configuration written for Linux. It
does not contain Windows/Realtek driver binaries, decoded vendor data, firmware,
or recordings.

## License

GPL-2.0-only, except the MangoHud-derived telemetry fix, which retains
MangoHud's MIT license. See [LICENSE](LICENSE) and
[fixes/mangohud-telemetry/LICENSE](fixes/mangohud-telemetry/LICENSE).
