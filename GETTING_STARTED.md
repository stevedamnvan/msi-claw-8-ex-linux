# Getting started on the MSI Claw 8 EX AI+

This guide takes a fresh Arch Linux or CachyOS installation from hardware
identification through installation and verification of the repository's
device fixes. When complete, the internal audio should work, MSI's firmware
controls should be exposed, and the optional GameScope telemetry corrections
should be active.

## 1. Confirm the exact device

Run:

```bash
cat /sys/class/dmi/id/{sys_vendor,product_name,board_name}
```

The output must be exactly:

```text
Micro-Star International Co., Ltd.
Claw 8 EX AI+ CG3EM
MS-1T91
```

Stop if any line differs. In particular, the Claw 8 AI+ A2VM/non-EX model has
different hardware and is not supported. The fixes contain their own DMI
guards, but the identity check prevents building and installing irrelevant
packages.

## 2. Prepare the system

The tested baseline is CachyOS with kernel
`7.2.0-rc7-1-cachyos-rc`. The DKMS packages may build for other Arch-family
kernels, but every kernel that should receive a module needs its exactly
matching headers.

First bring an Arch-family installation fully up to date:

```bash
sudo pacman -Syu
```

Reboot before continuing if that command installed a new kernel. Then inspect
the running kernel and its build link:

```bash
uname -r
test -e "/usr/lib/modules/$(uname -r)/build/Makefile" \
  && echo "matching headers found" \
  || echo "matching headers are missing"
```

Install the matching header package if it is missing. Common examples are:

| Running kernel | Header package |
| --- | --- |
| CachyOS RC | `linux-cachyos-rc-headers` |
| CachyOS stable | `linux-cachyos-headers` |
| Arch Linux | `linux-headers` |

Do not choose a header package by name alone: repeat the build-link check after
installation. A DKMS build failure for only one installed kernel normally means
that kernel's headers are absent.

Install the common build and audio stack dependencies:

```bash
sudo pacman -S --needed \
  base-devel clang dkms git \
  sof-firmware linux-firmware-intel alsa-ucm-conf \
  pipewire-audio pipewire-alsa pipewire-pulse wireplumber
```

These packages complement the normal Linux SOF, ALSA, PipeWire, and WirePlumber
stack; the audio workaround does not replace firmware or the sound server.

If Secure Boot enforces kernel-module signatures, sign each installed DKMS
module and enroll the signing key using the procedure for the installed
distribution. Unsigned modules cannot load under an enforcing policy.

## 3. Download the repository

Clone the current `main` branch and enter it:

```bash
cd ~
git clone https://github.com/stevedamnvan/msi-claw-8-ex-linux.git
cd msi-claw-8-ex-linux
```

The commands below assume this location. If the repository is elsewhere,
adjust the paths while keeping each `makepkg` command inside its package
directory. Run `makepkg` as the regular user, not with `sudo`.

## 4. Install the audio workaround

This is the recommended audio path for normal use:

```bash
cd ~/msi-claw-8-ex-linux/fixes/audio-rt721
makepkg -si
sudo systemctl enable --now claw-rt721-fix.service
```

The one-shot service applies the RT721 codec, amplifier, and microphone D0
sequences. Its system-sleep hook reapplies them after resume. The helper module
unloads after the service finishes; that is expected.

Use the package method above on a new installation. The component also has a
direct developer installer, but files installed that way are outside Pacman's
database. Never mix the two methods.

To migrate an older direct installation, first confirm that Pacman does not own
the fix, then remove the direct copy before running `makepkg -si`:

```bash
pacman -Q claw-rt721-fix-dkms
cd ~/msi-claw-8-ex-linux/fixes/audio-rt721
sudo ./uninstall.sh
makepkg -si
sudo systemctl enable --now claw-rt721-fix.service
```

Only use this migration sequence when the first command says the package was
not found and `dkms status` still lists `claw-rt721-fix`.

## 5. Install TDP, profile, fan, and charge controls

Build the `msi-wmi-platform` DKMS backport:

```bash
cd ~/msi-claw-8-ex-linux/fixes/platform-controls
makepkg -si
```

This provides the firmware interfaces used for:

- the `custom`/manual performance profile;
- PL1/SPL from 8 to 35 W and PL2/SPPT from 9 to 45 W;
- both fan tachometers and firmware fan curves; and
- the battery charge threshold.

Do not hot-unload the active MSI WMI module. The new copy should bind during a
normal reboot. Keep automatic fan control enabled until the factory curves have
been inspected, and never create a curve whose fan duty falls as temperature
rises.

SteamOS Manager support is optional at the package level. When a distribution
image includes a compatible version, it consumes these interfaces for Gaming
Mode TDP control. The kernel's `intel_pstate` driver handles CPU scaling and
boost; SteamOS Manager handles Intel Xe clock limits.

> [!NOTE]
> At the last verification on 2026-08-14, the exact
> [`MS-1T91` device profile](https://gitlab.steamos.cloud/holo/steamos-manager/-/blob/main/data/devices/msi-claw-8ex.toml)
> was present on SteamOS Manager's `main` branch but absent from the newest
> official `holo-main` package, `26.4.1-2`. The DKMS backport exposes the
> kernel-side controls now, but a native Steam TDP slider also requires a
> future compatible manager package. Check the
> [official package index](https://steamdeck-packages.steamos.cloud/archlinux-mirror/holo-main/os/x86_64/)
> for a version newer than `26.4.1`.

## 6. Optionally install corrected GameScope telemetry

Install this package if Steam's performance overlay reports impossible battery
draw, lacks Intel CPU/GPU watts or temperatures, omits the two fan tachometers,
or shows zero shared-memory VRAM:

```bash
sudo pacman -S --needed \
  appstream glslang libxrandr meson python-mako
cd ~/msi-claw-8-ex-linux/fixes/mangohud-telemetry
makepkg -si
sudo systemctl enable --now claw-mangohud-rapl-access.service
```

Accept Pacman's replacement of the stock 64-bit `mangohud` package with
`mangohud-claw`. An installed `lib32-mangohud` package remains in place. The
service grants the local `video` group read access only to the Intel
`package-0` and `uncore` energy counters and Panther Lake normal PMT telemetry
endpoint. It changes no power limit, fan curve, or sensor value.

The metrics have deliberately narrow meanings:

| Overlay value | Meaning on this device |
| --- | --- |
| Battery watts/time | DMI-gated correction of the EX firmware's wrapped discharge current |
| CPU power | Intel RAPL package power, including more than execution cores |
| GPU power | Intel RAPL uncore power because Xe has no DRM hwmon energy sensor |
| VRAM | Focused game's resident Xe GTT/shared-memory allocations |
| GPU utilization | Focused game's per-client Xe render activity, not the whole system |
| Fan RPM | `fan1/fan2` from the `msi_wmi_platform` hwmon device |
| CPU temperature | `coretemp` sensor labeled `Package id 0` |
| GPU temperature | Intel PMT `GT_MAX` from Panther Lake normal telemetry GUID `0x03086000` |

The package discovers the PMT endpoint by GUID because its `telemN` index can
change. The GPU fallback is exact-DMI-gated and is used only while Xe lacks a
native hwmon temperature source.

## 7. Reboot once

After installing the selected packages:

```bash
sudo reboot
```

The reboot activates the platform-controls DKMS override and replaces any old
GameScope `mangoapp` process with the patched build.

## 8. Verify every installed component

### Kernel and DKMS

```bash
uname -r
dkms status
```

The audio and platform entries should say `installed` for the running kernel.
If an entry is missing, install that kernel's matching headers and reinstall
the relevant package.

### Audio

```bash
systemctl --no-pager --full status claw-rt721-fix.service
sudo journalctl -b -k --no-pager | grep claw_rt721_amp
wpctl status
```

The service should be `active (exited)` with a successful result. Kernel
messages should show all three masks applied without a verification mismatch,
and WirePlumber should list the internal speaker and microphone. Test both
playback and capture, then test them once more after suspend/resume.

### Platform controls

```bash
modinfo -n msi_wmi_platform
claw-platform-controls-status
```

The module path must contain `updates/dkms`. The read-only helper should find
the `custom` profile, `ppt_pl1_spl`, `ppt_pl2_sppt`, both fans, and their
curves. On an image with compatible SteamOS Manager support, it also reports
the manager's exported properties.

### GameScope telemetry

```bash
systemctl --no-pager --full status claw-mangohud-rapl-access.service
claw-mangohud-telemetry-status
```

Disconnect external power for the battery portion of the test. Under load, the
helper should report plausible battery draw plus nonzero CPU package and GPU
uncore power, both fan speeds, CPU package temperature, and a PMT `GT_MAX` GPU
temperature. In Gaming Mode, enable Steam's level-3 or level-4 performance
overlay and launch a game. The detailed overlay should show `FAN 1/2`, CPU/GPU
temperatures, CPU watts, and shared-memory VRAM; GPU watts should respond to
load. GPU utilization can remain low when the focused game is idle or frame
limited.

## 9. Update after repository or kernel changes

Update the source without creating a merge commit:

```bash
cd ~/msi-claw-8-ex-linux
git switch main
git pull --ff-only
```

Re-run `makepkg -si` inside a component directory when its package files
change. Re-enable its service if Pacman reports it disabled, and reboot after a
platform-controls rebuild.

After every kernel update:

```bash
test -e "/usr/lib/modules/$(uname -r)/build/Makefile" \
  && echo "matching headers found" \
  || echo "matching headers are missing"
dkms status
```

Remember to reboot into the new kernel before checking its build link. DKMS
normally rebuilds automatically when matching headers are installed.

## 10. Troubleshoot by symptom

### A DKMS package does not build

Compare `uname -r` with the header package and
`/usr/lib/modules/<kernel>/build`. Install the matching headers, then reinstall
the affected package with `makepkg -si`. The platform source is compiled with
Clang to match the tested kernel; do not remove its `clang` dependency.

### Audio devices exist, but playback or capture is silent

Check the audio service and boot journal shown above. A verification mismatch
or a DMI refusal is more useful than changing PipeWire profiles at random. If
the service succeeds, use `wpctl status` to confirm that WirePlumber selected
the internal speaker and microphone, then test suspend/resume.

### TDP or fan controls are absent

Run `modinfo -n msi_wmi_platform`. If the path does not contain
`updates/dkms`, the in-tree module is active: verify `dkms status` and reboot.
If the DKMS path is active, run `claw-platform-controls-status` and retain its
complete output when reporting a problem.

### The overlay still shows old or missing values

Restart the entire Gaming Mode session or reboot; replacing the package does
not replace an already running `mangoapp`. Confirm the RAPL access service is
active, despite its compatibility-era name. Run
`claw-mangohud-telemetry-status`: if GPU temperature says the PMT endpoint is
not readable, restart `claw-mangohud-rapl-access.service`. Test battery
telemetry while unplugged and power/temperature metrics while a game is under
load.

## 11. Remove the fixes

Remove only the components that were installed. Reverse order is simplest.

Telemetry:

```bash
sudo systemctl disable --now claw-mangohud-rapl-access.service
sudo pacman -Rns mangohud-claw
sudo pacman -S mangohud
```

Platform controls:

```bash
sudo pacman -Rns claw-msi-wmi-platform-dkms
sudo reboot
```

Packaged audio workaround:

```bash
sudo systemctl disable --now claw-rt721-fix.service
sudo pacman -Rns claw-rt721-fix-dkms
```

Reboot after audio removal to reset the hidden power state. If telemetry alone
was removed, restart the Gaming Mode session or reboot so the stock `mangoapp`
process replaces the patched one.

If audio was previously installed with `sudo ./install.sh` instead of Pacman,
run `sudo ./uninstall.sh` from `fixes/audio-rt721/`; do not use the package
removal command for that direct installation.

## Advanced audio-driver development

Kernel builders can test the
[RT721 in-driver power-management patch](patches/rt721-power-management/),
which integrates the EX power transitions into runtime PM rather than leaving
the workaround's selected D0 gates enabled. It is based on Linux `v7.2-rc7`.

Treat that patch as an alternative to the one-shot workaround. The workaround
service and post-resume hook can mask microphone-resume behavior, so disable or
bypass them for clean patched-kernel validation. The remaining required test is
a helper-independent microphone capture after suspend/resume.

## Known limitations

- Only the exact `CG3EM`/`MS-1T91` EX model is supported.
- The packaged audio workaround leaves selected hidden D0 power gates enabled
  until hardware reset; its idle battery impact has not been measured.
- The in-driver RT721 patch is experimental and still needs the clean
  microphone-resume test described above.
- The telemetry package corrects specific data paths. It does not convert the
  focused-client Xe utilization metric into whole-system GPU utilization.
- GPU temperature uses Intel's public Panther Lake PMT `GT_MAX` definition
  until Xe exposes a native hwmon temperature sensor; the access service makes
  the matching raw PMT block readable to the local `video` group.
- These packages track pinned development snapshots and are not substitutes
  for future upstream kernel, SteamOS Manager, or MangoHud releases.

For component internals and current validation evidence, follow the links in
the repository's [project layout](README.md#project-layout). Developers should
also use the [engineering reference](ENGINEERING.md) for formal solution
records, upstream provenance, validation requirements, and an issue-report
template.
