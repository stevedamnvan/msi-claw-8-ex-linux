# GameScope telemetry

This Arch/CachyOS package rebuilds MangoHud `0.8.4` for the **MSI Claw 8
EX AI+ CG3EM**, board **MS-1T91**. It corrects the battery discharge value
used by Steam's GameScope performance overlay, adds Intel CPU/GPU power, and
makes the overlay's VRAM row report the focused Xe client's shared-memory
residency.

## Fixes

### Battery watts and remaining time

While discharging, this firmware can expose values such as `-62980000` µA.
That is not a 62.98 A discharge. It is a wrapped 16-bit milliamps value:

```text
-62980 mA + 65536 mA = 2556 mA
```

Stock MangoHud takes the absolute value and can therefore display more than
1,000 W and an unusable remaining-time estimate. The patch decodes the value
only when all of the following are true:

- system vendor is `Micro-Star International Co., Ltd.`
- product is `Claw 8 EX AI+ CG3EM`
- board is `MS-1T91`
- battery state is `Discharging`
- the magnitude is in the impossible wrapped range

Normal charging current and ordinary signed discharge values are unchanged.

### Intel CPU and GPU power

The integrated Xe GPU has no DRM hwmon energy sensor. The second patch uses
the Intel RAPL `uncore` energy counter when that sensor is absent. This is a
backport of the approach in upstream
[MangoHud PR #2080](https://github.com/flightlessmango/MangoHud/pull/2080),
which was smoke-tested with `mangoapp` under SteamOS on an MSI Claw 8 AI+.

MangoHud already supports Intel CPU package power through the RAPL `package-0`
counter. CachyOS exposes both energy files as root-readable, so the packaged
service checks the exact EX DMI values and the `package-0`/`uncore` zone names,
then grants read access to the `video` group only. It does not change any power
limit. The overlay's CPU value is package power, so it includes more than just
the execution cores.

### Intel shared-memory VRAM

The integrated Xe GPU uses system memory instead of discrete VRAM. The kernel
reports the focused DRM client's resident graphics allocations through
`drm-resident-gtt`. MangoHud `0.8.4` reads that value into its per-process VRAM
metric, but GameScope's presets display the separate system-VRAM metric, which
therefore remains at zero.

The third patch mirrors the focused client's GTT residency into GameScope's
VRAM row only for integrated Intel graphics. It also retries fdinfo discovery
while the value is zero, avoiding the initial delay of up to ten seconds when a
game opens its rendering client after MangoHud starts. This is a per-game
resident-memory value, not total system RAM or a fixed firmware UMA allocation.

## GPU utilization status

This package does **not** alter the GPU-percent calculation. Linux Xe exports
per-DRM-client engine counters, and MangoHud reports the focused game's render
client rather than whole-system GPU use. The kernel documents the calculation
as the delta of `drm-cycles-rcs` divided by the delta of
`drm-total-cycles-rcs`.

The faster initial fdinfo discovery used for VRAM also lets the existing GPU
load calculation see a late-opened render client sooner; its formula and scope
are unchanged.

A local Dark Souls III capture found the expected Xe client and a stable
3.9–4.1% render load while the game was background-idle; the other engine
counters stayed at zero. A foreground GameScope capture is still required
before changing this code. Aggregating arbitrary processes would give a
different metric, not repair the per-game metric.

See the kernel's
[Xe client usage documentation](https://docs.kernel.org/gpu/xe/xe-drm-usage-stats.html)
and the generic
[DRM usage-stat specification](https://docs.kernel.org/gpu/drm-usage-stats.html).

## Build and install

```bash
sudo pacman -S --needed base-devel appstream glslang libxrandr meson python-mako
makepkg -si
sudo systemctl enable --now claw-mangohud-rapl-access.service
```

`mangohud-claw` provides and replaces the 64-bit `mangohud` package. The
distribution's `lib32-mangohud` remains installed because it has no overlapping
files. GameScope's `mangoapp` executable is 64-bit and uses the patched build.
The NVIDIA-only XNVCtrl backend is disabled; NVML support remains available.

Restart the gaming session or reboot after installation. The old `mangoapp`
process must exit before the replacement is used.

## Verify

Run the status helper:

```bash
claw-mangohud-telemetry-status
```

For the battery test, disconnect external power first. The output should show
a raw current near `-63000000` µA, a corrected current near 2–3 A, and a
plausible draw in watts. While charging, the raw positive current is left
unchanged.

For CPU/GPU power, confirm the service is active:

```bash
systemctl status claw-mangohud-rapl-access.service
```

The helper should then print nonzero `CPU package power` and `GPU uncore power`
readings under load. After the gaming session restarts, Steam's level-3/level-4
performance overlay should show those metrics and a nonzero VRAM value while a
game has resident graphics allocations.

## Validation status

- all three patches apply cleanly to the pinned MangoHud `0.8.4` release
- the modified battery and Xe telemetry translation units pass compiler checks
- `makepkg` completes all 117 build targets and creates the installable package
- all packaged executable and shared-library dependencies resolve
- the DMI-gated access helper selects only local `package-0` and `uncore` zones
- the package installs cleanly and passes Pacman's installed-file verification
- an unplugged sample decoded `-64931000` µA to `605000` µA and reported 10.40 W
- the service provides live CPU package and GPU uncore samples to `deck`
- a local Vulkan probe reported 0.048 GiB of Xe shared memory on its first
  500 ms refresh instead of waiting for the ten-second fallback scan

The remaining live check is a foreground overlay capture after restarting into
the native GameScope session.

## Remove

```bash
sudo systemctl disable --now claw-mangohud-rapl-access.service
sudo pacman -Rns mangohud-claw
sudo pacman -S mangohud
```

Disabling or removing the access service does not need a manual permission
reset; sysfs permissions return to the kernel defaults at reboot.

## Security note

Fine-grained energy counters can be useful to side-channel analysis, which is
why current kernels restrict them by default. The service exposes only the
Intel `package-0` and `uncore` counters, only on the exact device, and only to
members of the local `video` group. Do not broaden them to world-readable on a
multi-user system.
