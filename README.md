# MSI Claw 8 EX AI+ Linux fixes

Experimental Arch Linux and CachyOS fixes for the **MSI Claw 8 EX AI+
CG3EM**, board **MS-1T91**.

> [!WARNING]
> These fixes are hardware-specific. The Claw 8 AI+ A2VM (non-EX) uses a
> different platform and audio path and is not supported by this repository.

## Available fixes

| Area | Fix | Status |
| --- | --- | --- |
| Audio | [Realtek RT721 SoundWire power workaround](fixes/audio-rt721/) | Speakers and microphone verified on CachyOS `7.2.0-rc7` |

The audio workaround is DMI-gated and refuses to run unless both the product
and board identifiers match the EX model. It is intended as a temporary bridge
until the power sequence can be integrated into the upstream RT721 codec
driver.

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

## Scope

This repository contains source code and configuration written for Linux. It
does not contain Windows/Realtek driver binaries, decoded vendor data, firmware,
or recordings.

## License

GPL-2.0-only. See [LICENSE](LICENSE).
