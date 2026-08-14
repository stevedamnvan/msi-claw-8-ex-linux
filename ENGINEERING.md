# Engineering reference

This document formalizes the MSI Claw 8 EX AI+ work as a set of traceable
engineering solutions. It is the starting point for reviewing assumptions,
reproducing results, preparing upstream submissions, or deciding when a local
fix can be retired.

The component READMEs remain the authority for installation details. This file
records the cross-component problem model, evidence boundaries, provenance,
acceptance criteria, and unresolved work.

## Hardware contract

All device-specific behavior must remain conservatively gated. The validated
identity is:

| Field | Required value |
| --- | --- |
| DMI system vendor | `Micro-Star International Co., Ltd.` |
| DMI product name | `Claw 8 EX AI+ CG3EM` |
| DMI board name | `MS-1T91` |
| RT721 SoundWire peripheral | `sdw:0:3:025d:0721:01` |

The Claw 8 AI+ A2VM/non-EX model is not an equivalent test target. A solution
must not broaden its hardware match merely because another device shares a
brand or some components. Add another identity only with that hardware's own
enumeration, before/after evidence, and physical validation.

The local `7.2.0-rc7-1-cachyos-rc` validation environment booted with
`fred=off`. No before/after evidence currently connects FRED to any solution in
this repository, so that command-line option is an environment qualifier, not
an installation requirement. Record its state when reproducing kernel-level
results.

## Status language

Use these terms consistently in documentation and change descriptions:

- **Observed** — captured directly from the affected machine.
- **Reproduced** — repeated under stated conditions with the same result.
- **Implemented** — code exists and builds; this alone is not a hardware test.
- **Verified** — the stated acceptance test passed on the exact device.
- **Pending** — a named test or dependency is still outstanding.
- **Upstream** — accepted into the relevant upstream project, not merely
  posted, present on a development branch, or packaged locally.

Do not turn a compile check, source inspection, or background-idle sample into
a runtime-verification claim.

## Solution index

| ID | Layer | Problem | Local implementation | Current state |
| --- | --- | --- | --- | --- |
| `EX-AUD-001` | RT721 audio | ALSA devices enumerate, but speakers and microphone are silent | [One-shot DKMS workaround](fixes/audio-rt721/) | Verified fallback; power limitation remains |
| `EX-AUD-002` | RT721 audio | Vendor analog gates are not integrated with codec runtime PM | [In-driver D0/D3 patch](patches/rt721-power-management/) | Runtime and speaker resume verified; clean microphone resume pending |
| `EX-PWR-001` | MSI firmware controls | In-tree `msi-wmi-platform` lacks the exact EX quirk and interfaces | [Valve integration backport](fixes/platform-controls/) | Core sysfs interfaces verified |
| `EX-TEL-001` | Battery telemetry | Wrapped discharge current produces impossible watts and time | [MangoHud battery patch](fixes/mangohud-telemetry/0001-battery-fix-msi-claw-8-ex-current.patch) | Unplugged samples verified |
| `EX-TEL-002` | Power telemetry | Integrated Xe exposes no DRM hwmon energy sensor | [MangoHud RAPL fallback](fixes/mangohud-telemetry/0002-gpu-power-use-intel-rapl-uncore.patch) | CPU package and GPU uncore samples verified |
| `EX-TEL-003` | Memory telemetry | GameScope's VRAM row ignores focused Xe GTT residency | [MangoHud shared-memory patch](fixes/mangohud-telemetry/0003-intel-shared-memory-vram.patch) | Translation path verified; native foreground capture pending |
| `EX-TEL-004` | Fan/thermal telemetry | MangoHud cannot see the two MSI fans or integrated Panther Lake GPU temperature | [MangoHud sensor-map patch](fixes/mangohud-telemetry/0004-map-fans-and-panther-lake-gpu-temperature.patch) | Live source mapping verified; native foreground capture pending |

## `EX-AUD-001`: packaged RT721 recovery workaround

### Problem statement

On the tested kernel, SOF, SoundWire, UCM, PipeWire, and WirePlumber expose
normal PCM devices, but playback and capture return silence. The RT721 vendor
analog power gates remain off while streams are active.

### Evidence and interpretation

The observed hidden control values before recovery were:

| Target | Register | Observed value | Controlled D0 mask |
| --- | --- | --- | --- |
| Codec | `0x05f00000` | `0x0092` | `0xf60c` |
| Amplifier | `0x05f00020` | `0x01c2` | `0xfe35` |
| Microphone | `0x05f00030` | `0x01c2` | `0xfe35` |

Applying the model-specific D0 sequences restored audible speaker playback and
changed microphone captures from exact digital zero to real samples. The full
register notes are in
[`fixes/audio-rt721/TECHNICAL.md`](fixes/audio-rt721/TECHNICAL.md).

### Implementation boundary

`claw_rt721_amp` is a DMI- and SoundWire-gated DKMS helper. A systemd one-shot
loads it with codec, amplifier, and microphone application enabled, verifies
the controlled bits, and unloads it. A sleep hook reruns the one-shot after
resume.

This is a recovery mechanism, not a replacement audio stack. It must not carry
firmware blobs, change ALSA topology, or paper over a missing SoundWire device.

### Acceptance and retirement

Acceptance requires a successful service result, no verification mismatch,
audible internal-speaker playback, nonzero internal-microphone capture, and a
repeat after suspend/resume.

The workaround leaves selected D0 bits enabled until hardware reset. Retire it
only after an in-driver solution passes equivalent cold-boot, runtime-PM,
playback, capture, and helper-independent resume tests in a deployed kernel.

## `EX-AUD-002`: RT721 runtime-power integration

### Problem statement

The generic RT721 SDCA power-state requests do not operate this model's vendor
analog gates. A boot-time D0 write restores audio but cannot power unused blocks
down or reliably own transitions across runtime PM and suspend.

### Implementation boundary

The patch changes only:

- `sound/soc/codecs/rt721-sdca-sdw.c`;
- `sound/soc/codecs/rt721-sdca.c`; and
- `sound/soc/codecs/rt721-sdca.h`.

It adds an exact three-field DMI quirk, marks vendor power registers volatile,
runs verified codec D0/D3 sequences around SoundWire runtime PM, and ties
amplifier and microphone sequences to their DAPM events. Its base is upstream
Linux `v7.2-rc7`; the tested CachyOS RT721 sources were byte-identical at
`7.2.0-rc7-1-cachyos-rc`.

### Verified and pending evidence

Verified evidence includes cold boot, complete HiFi enumeration, playback,
capture, repeated codec/amplifier/microphone D0-to-D3 transitions, and three
s2idle cycles with speaker recovery. No MBQ verification, SoundWire bus-clash,
or retry error appeared in those tests.

The old post-resume helper still ran during those cycles. It cannot explain the
observed codec or speaker transitions, but it can mask microphone recovery.
The remaining release gate is an internal-microphone capture after s2idle with
both the workaround service and sleep hook bypassed.

### Upstream target

Rebase onto the receiving ASoC tree, rerun the full hardware matrix, and send
the change to the maintainers returned by
`scripts/get_maintainer.pl`. Add a `Signed-off-by` trailer only after the sender
reviews the patch and can make the Developer Certificate of Origin statement.
Preserve the structured assistance disclosure required by the receiving
project.

## `EX-PWR-001`: MSI firmware-control integration

### Problem statement

The tested `7.2.0-rc7-1-cachyos-rc` distribution kernel's in-tree
`msi-wmi-platform` module does not contain the exact `MS-1T91` integration
needed by SteamOS Manager. Without that kernel half, the EX-specific profile,
firmware-attribute power limits, two-fan hwmon surface, and charge control are
incomplete.

### Provenance and implementation

The DKMS package pins three Valve development commits by immutable hash and
checksum:

1. [`1cd45521751f`](https://github.com/evlaV/linux-integration/commit/1cd45521751f918b7ce1cf73871c72c452dba36b)
   — device cleanup, distinct PL1/PL2 minimums, and WMI write-buffer repair.
2. [`014b12dc04ab`](https://github.com/evlaV/linux-integration/commit/014b12dc04ab2a46c3c2db4bff6f29e2bd58e71e)
   — firmware-attribute TDP support and custom-shift infrastructure.
3. [`002ae467a84c`](https://github.com/evlaV/linux-integration/commit/002ae467a84c83b07727e38511d3ce9c6ce3b603)
   — the exact `MS-1T91` Claw 8 EX quirk.

SteamOS Manager's exact
[`msi-claw-8ex.toml`](https://gitlab.steamos.cloud/holo/steamos-manager/-/blob/main/data/devices/msi-claw-8ex.toml)
selects `firmware_attribute`, `msi-wmi-platform`, and the `custom` performance
profile. That userspace profile and this kernel integration are separate
dependencies; the existence of one does not prove that a packaged image
contains the other.

On 2026-08-14, the profile was present at SteamOS Manager `main` commit
`7de5caad5e4d4f3bd9f179ddd7acc94e1cb2d677`, while the newest official
`holo-main` package was `26.4.1-2` and did not contain it. Recheck both source
and package contents before updating that status; do not infer package support
from a source-branch commit.

### Acceptance and safety

Acceptance requires the active module path to contain `updates/dkms`, a
`custom` platform-profile choice, `ppt_pl1_spl` at 8–35 W,
`ppt_pl2_sppt` at 9–45 W, both fan tachometers and curves, and a working charge
threshold interface. A native Gaming Mode slider additionally requires a
compatible packaged SteamOS Manager profile.

Never hot-unload the active module during evaluation. Preserve automatic fan
control until both factory curves are captured. Reject any proposed curve with
decreasing fan duty at increasing temperature.

Retire the DKMS package after the required exact-device changes ship in the
running distribution kernel and the in-tree module passes the same status
check after reboot.

## `EX-TEL-001`: battery-current normalization

### Problem statement and model

During discharge, the firmware can expose a value such as `-62980000` microamps.
Interpreting its absolute value produces impossible power draw. The evidence
fits a signed presentation of a wrapped 16-bit milliamps counter:

```text
-62980 mA + 65536 mA = 2556 mA
```

The patch adds `65536000` microamps only when the exact three-field DMI match,
battery state is `Discharging`, and the reported value lies in the impossible
wrapped range from `-65536000` through `-32768000` microamps. Charging and
ordinary signed values remain untouched. Both battery watts and remaining-time
paths consume the normalized value.

Acceptance requires capturing the raw sysfs current, normalized current,
voltage, computed watts, charge state, and a plausibility comparison while
unplugged. A firmware update that reports ordinary current again should pass
through unchanged and is the preferred eventual retirement path. Otherwise,
submit the narrowly gated normalization to MangoHud with raw samples from more
than one discharge level; retire the local patch after it appears in the
packaged upstream release.

## `EX-TEL-002`: Intel RAPL power fallback

### Problem statement and implementation

Integrated Xe does not expose the discrete-GPU DRM hwmon energy source MangoHud
normally expects. The patch uses Intel's `uncore` RAPL `energy_uj` counter only
when no DRM hwmon power source is available, handles counter wrap, and leaves
the existing source preferred when present. MangoHud's existing `package-0`
path supplies CPU package power.

This is a MangoHud `0.8.4` backport of the approach in
[MangoHud PR #2080](https://github.com/flightlessmango/MangoHud/pull/2080).
The packaged service grants group-readable access only to local `package-0` and
`uncore` energy files after an exact DMI and zone-name check. It changes no
power limit and must not be broadened to world-readable access.

Acceptance requires nonzero package and uncore deltas under load, correct unit
conversion to watts, counter-wrap handling by inspection or test, and no
regression when a native hwmon source exists. Retire this backport when a
packaged MangoHud release contains the equivalent upstream RAPL support and
the restricted energy-counter access still passes locally.

## `EX-TEL-003`: focused Xe shared-memory reporting

### Problem statement and implementation

Xe uses system memory, and the focused DRM client reports its resident graphics
allocations as `drm-resident-gtt`. MangoHud reads that into the per-process VRAM
field, while GameScope presets display the separate system-VRAM field. The
patch mirrors the focused-client value into that displayed row only on
integrated Intel GPUs and retries fdinfo discovery while the value is zero.

This value is not total RAM, a firmware UMA allocation, or aggregate GPU
memory. The patch also does not change GPU utilization: that remains the
focused client's render-cycle delta divided by its total-cycle delta.

Acceptance requires a known Xe rendering client, its fdinfo residency sample,
the translated MangoHud value, and a foreground GameScope capture. A low GPU
percentage while a game is idle or frame-limited is not evidence of failure.
Use that foreground capture as the release gate for an upstream MangoHud
proposal, and keep the shared-memory semantic explicit in its change log.

## `EX-TEL-004`: MSI fan and Panther Lake thermal mapping

### Problem statement and observed surfaces

MangoHud `0.8.4` restricts its system-fan element to
`steamdeck_hwmon/fan1_input`. On the exact EX, the platform driver instead
exports two live tachometers through `msi_wmi_platform`; both were observed at
approximately 3,500–3,900 RPM in automatic mode. Its `pwm*_auto_point*` files
are fan-curve thresholds, not current temperatures, and must never be relabeled
as thermal telemetry.

CPU package temperature already has a standard source: `coretemp` with label
`Package id 0`. No CPU-temperature fallback is introduced.

The integrated Arc B390 (`8086:b080`, Xe) exposes no DRM hwmon directory on the
tested kernel. It does expose Intel PMT normal P-unit telemetry with GUID
`0x03086000` and size 3,352 bytes. Intel's public Panther Lake definition names
bits 56–63 of container 15 `GT_MAX` and describes them as the graphics maximum
temperature. Its datatype is signed 8-bit Celsius, placing it at byte 127.
The immutable source references are Intel-PMT commit
[`0fe763db930d`](https://github.com/intel/Intel-PMT/commit/0fe763db930d74ce6494ea5ed65ce866ad13613f),
[`ptl_aggregator.xml`](https://github.com/intel/Intel-PMT/blob/0fe763db930d74ce6494ea5ed65ce866ad13613f/xml/PTL/0/ptl_aggregator.xml#L1069-L1136),
and
[`ptl_common.xml`](https://github.com/intel/Intel-PMT/blob/0fe763db930d74ce6494ea5ed65ce866ad13613f/xml/PTL/0/ptl_common.xml#L84-L91).

### Implementation boundary

The patch:

- discovers `msi_wmi_platform` by hwmon name and reads `fan1_input` plus
  `fan2_input` into a combined `fan1/fan2 RPM` row;
- preserves the original Steam Deck single-fan path;
- enables that row in MangoHud's level-3/extended preset, where CPU/GPU
  temperature is already enabled;
- uses PMT only for an integrated Xe GPU on the exact three-field DMI match,
  only when native Xe hwmon temperature is absent;
- selects PMT by GUID and minimum size rather than an unstable `telemN` index;
  and
- validates the signed sample as greater than 0 and less than 125 C.

The access service changes the matching PMT file from `root:root 0440` to
`root:video 0440` after the same exact DMI check. Sysfs permissions cover the
entire telemetry block, not only the `GT_MAX` byte; this scope and its security
tradeoff must remain documented. No WMI write, fan-mode change, curve edit, or
thermal-policy change belongs to this solution.

### Acceptance, upstream split, and retirement

Acceptance requires the status helper to report both live fan values, a
plausible `coretemp Package id 0`, a readable matching PMT GUID, and a plausible
`GT_MAX` that responds to graphics load. A native GameScope level-3 or level-4
capture must then show `FAN 1/2`, CPU temperature, and GPU temperature. Compare
the PMT byte and displayed value in the same interval; a successful build alone
does not verify the live decoder.

On 2026-08-14, the installed `0.8.4-3` package selected PMT GUID `0x03086000`
at `/sys/class/intel_pmt/telem2/telem`, selected
`coretemp/temp1_input`, and reported both MSI fans. During an uncapped Vulkan
probe, raw `GT_MAX` rose from 44 C to 53 C while MangoHud reported the same
49–53 C progression, 75–83% focused Xe load, a 2,300 MHz graphics clock, and
roughly 11–12 W uncore power. This verifies the live reader and source
agreement, but not yet the native GameScope rendering of the three rows.

For upstream review, split the generic dual-fan MangoHud work from the Intel
PMT fallback, provide the Intel schema provenance and live before/after data,
and preserve native hwmon precedence. Retire the fan portion when packaged
MangoHud can select both named MSI hwmon channels. Retire the PMT fallback when
the running Xe driver exports a correct native integrated-GPU temperature
through hwmon and packaged MangoHud consumes it.

## Reproduction record

Attach this minimum environment record to every hardware result:

```bash
cat /sys/class/dmi/id/{sys_vendor,product_name,board_name}
uname -a
git -C ~/msi-claw-8-ex-linux rev-parse HEAD
pacman -Q sof-firmware linux-firmware-intel alsa-ucm-conf \
  pipewire-audio pipewire-alsa pipewire-pulse wireplumber \
  steamos-manager gamescope mangohud-claw 2>&1
dkms status
modinfo -n msi_wmi_platform
systemctl is-active claw-rt721-fix.service \
  claw-mangohud-rapl-access.service
```

Also record AC connected/disconnected state, Steam Gaming Mode versus desktop
session, performance profile, TDP limits, whether the game was foreground and
under load, and whether a suspend/resume cycle occurred. Do not publish serial
numbers, UUIDs, account names, or an unreviewed full system journal.

## Validation matrix for release claims

| Area | Required conditions | Required result |
| --- | --- | --- |
| Build | Clean package build with pinned sources | Package completes and installed files verify |
| Cold boot | Power off, boot target kernel without stale helper state | Expected devices and interfaces enumerate |
| Audio playback | Internal speaker selected, known signal | Audible output without codec errors |
| Audio capture | Internal microphone selected, fixed-duration sample | Nonzero real samples without verification errors |
| Runtime PM | Close streams and observe D3, reopen and observe D0 | Clean transition and working next stream |
| Suspend | At least three s2idle cycles | Speaker and microphone both work after every resume |
| Platform | DKMS module selected after normal reboot | Profiles, both TDP attributes, fans, curves, and charge interface present |
| Battery | Unplugged with raw current retained | Corrected amps, watts, and time are plausible |
| CPU/GPU power | Sustained known load | Nonzero package/uncore deltas with correct units |
| VRAM/GPU load | Foreground native GameScope game | Overlay agrees with focused Xe fdinfo semantics |
| Fan/thermal telemetry | Foreground native GameScope game under changing load | Both MSI RPM values plus CPU package and PMT `GT_MAX` temperatures agree with their source sensors |

State exactly which rows passed. A release does not inherit a row merely
because an earlier revision passed it.

## Upstream submission discipline

For each subsystem change:

1. Reduce the report to one reproducible symptom and preserve the raw before
   state.
2. Identify the owning subsystem and avoid combining unrelated fixes in one
   patch series.
3. Use the narrowest hardware match supported by physical evidence.
4. Separate hardware-tested claims from compile-only or source-inspection
   claims.
5. Rebase onto the maintainer's requested tree, build with warnings enabled,
   and run the subsystem's documented checks.
6. Explain the failure mechanism, why the chosen hook owns the transition, and
   how failures are unwound.
7. Include measured before/after evidence and explicitly list remaining tests.
8. Follow the Linux kernel
   [submission process](https://docs.kernel.org/process/submitting-patches.html),
   DCO rules, maintainer routing, and the disclosure requirements in the
   target tree's `Documentation/process/generated-content.rst` and
   `Documentation/process/coding-assistants.rst`.
9. Have the human sender review, understand, and take responsibility for every
   line and claim.

The user-provided
[HP OmniBook audio series](https://lore.kernel.org/lkml/20260809101439.4798-1-wiza@saarinenkoti.fi/)
is not a technical source for this MSI device. It is a useful process example:
it states the observed root cause, splits changes by dependency and subsystem,
uses conservative board gating, distinguishes hardware tests from compile
tests, discloses coding assistance, and assigns responsibility to the sender.

## Issue or test-report template

Use this structure in a GitHub issue or engineering handoff:

```text
Solution ID:
Device vendor/product/board:
Repository commit:
Distribution and kernel:
Matching headers installed:
Relevant package versions:
DKMS status:
Session (Gaming Mode/desktop):
Power state and performance profile:
Cold boot or resume:

Expected result:
Observed result:
Exact reproduction steps:

Status-helper output:
Relevant, reviewed journal lines:
Raw measurement before transformation:
Measurement after transformation:
Physical confirmation (heard/captured/observed):

Workaround service active during test:
Known-good comparison:
Remaining uncertainty:
```

Reference the stable solution ID in commits, issues, and test notes so later
results remain connected even if directories or package versions change.
