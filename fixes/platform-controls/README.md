# MSI Claw platform controls

This DKMS package backports Valve's pending `msi-wmi-platform` work for the
MSI Claw 8 EX AI+ CG3EM, board `MS-1T91`. It replaces the kernel's in-tree
module after the next reboot and exposes the interfaces expected by
`steamos-manager`.

## What it enables

- MSI performance profiles, including the EX-specific `custom`/manual mode
- firmware-attribute TDP controls with the firmware limits Valve identified:
  PL1/SPL 8–35 W and PL2/SPPT 9–45 W
- both fan tachometers and both firmware fan curves through hwmon
- restoration of the factory fan curves when manual fan control is disabled
  or the driver is unloaded
- battery charge-threshold control

CPU boost and scaling controls already come from `intel_pstate`. Intel Xe GPU
clock controls already come from `steamos-manager`; this module does not
replace either subsystem.

## Upstream status

Valve's
[`steamos-manager` device profile](https://gitlab.steamos.cloud/holo/steamos-manager/-/commit/7de5caad5e4d4f3bd9f179ddd7acc94e1cb2d677)
already identifies `MS-1T91` and requests the `msi-wmi-platform` firmware
attribute backend. The kernel side is the missing half. This package pins the
result of Valve's three-commit kernel development series:

1. [`1cd45521751f`](https://github.com/evlaV/linux-integration/commit/1cd45521751f918b7ce1cf73871c72c452dba36b)
   — clean up devices, separate PL1/PL2 minimums, and fix the debugfs WMI
   write buffer
2. [`014b12dc04ab`](https://github.com/evlaV/linux-integration/commit/014b12dc04ab2a46c3c2db4bff6f29e2bd58e71e)
   — add AMD TDP support and the custom-shift infrastructure
3. [`002ae467a84c`](https://github.com/evlaV/linux-integration/commit/002ae467a84c83b07727e38511d3ce9c6ce3b603)
   — add the exact `MS-1T91` Claw 8 EX quirk

The final commit is marked `FOR-UPSTREAM` but was not yet present in Valve's
public `7.2/integration` beta branch when this package was created. Source is
pinned by commit and checksum rather than following a moving branch.

## Install

Install the headers matching every kernel that should receive the module. For
the currently tested CachyOS RC kernel:

```bash
sudo pacman -S --needed base-devel dkms clang linux-cachyos-rc-headers
makepkg -si
reboot
```

Do not hot-unload the active MSI WMI module. A reboot lets the DKMS override
bind during normal device initialization and preserves the firmware-owned fan
state until then.

## Verify after reboot

Confirm that the DKMS copy is active:

```bash
modinfo -n msi_wmi_platform
```

The path should contain `updates/dkms`. Then check the exported controls:

```bash
grep -H . /sys/class/platform-profile/platform-profile-*/{name,choices,profile}
find /sys/class/firmware-attributes/msi-wmi-platform/attributes \
  -maxdepth 2 -type f -print
sensors
steamosctl get-all-properties
```

Or run the packaged read-only status check:

```bash
claw-platform-controls-status
```

The platform-profile choices should include `custom`; the firmware attributes
should include `ppt_pl1_spl` and `ppt_pl2_sppt`; and SteamOS Manager should
publish its TDP interface. Fan-curve files appear under the matching hwmon
device as `pwm1_auto_point*` and `pwm2_auto_point*`.

## Safety

The EX settings are DMI-gated on both MSI's system vendor and board
`MS-1T91`. The driver constrains TDP values to Valve's device-specific ranges.
Keep automatic fan mode enabled until the factory curves have been captured
and verified. Never create a curve that lowers fan duty at higher
temperatures.

## Remove

```bash
sudo pacman -Rns claw-msi-wmi-platform-dkms
reboot
```

After reboot, the distribution kernel's in-tree module is used again.
