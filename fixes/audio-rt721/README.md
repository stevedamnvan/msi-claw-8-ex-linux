# RT721 SoundWire audio power workaround

This temporary DKMS module restores the internal speakers and microphone on
the MSI Claw 8 EX AI+ CG3EM when the normal Linux audio devices exist but
playback and capture are silent.

## Exact hardware scope

The module refuses to operate unless all of the following match:

- DMI product: `Claw 8 EX AI+ CG3EM`
- DMI board: `MS-1T91`
- SoundWire device: `sdw:0:3:025d:0721:01` (Realtek RT721)

It is **not** for the Claw 8 AI+ A2VM/non-EX model. That device is Lunar Lake
based and uses a different ALSA topology.

## What was verified

On CachyOS kernel `7.2.0-rc7-1-cachyos-rc`, stock SOF/SoundWire drivers exposed
valid PCM devices but returned silence. The hidden RT721 power controls were
off even while streams were active:

- codec `0x05f00000`: `0x0092`
- amplifier `0x05f00020`: `0x01c2`
- microphone `0x05f00030`: `0x01c2`

After applying the exact model-specific D0 sequences:

- codec controlled bits: `0xf60c`
- amplifier controlled bits: `0xfe35`
- microphone controlled bits: `0xfe35`

Speaker playback was audibly confirmed. Microphone captures changed from exact
digital zero to real samples around -17 dB RMS.

## Requirements

- The exact hardware above
- Arch Linux or CachyOS
- `dkms`, `clang`, `make`, and headers matching `uname -r`
- The normal SOF, ALSA UCM, PipeWire, and WirePlumber packages

This does not replace firmware or PipeWire. The tested system used
`sof-firmware`, `linux-firmware-intel`, `alsa-ucm-conf`, `pipewire-audio`,
`pipewire-alsa`, `pipewire-pulse`, and `wireplumber`.

## Install as an Arch package

From this directory:

```bash
makepkg -si
sudo systemctl enable --now claw-rt721-fix.service
```

DKMS rebuilds the module when a kernel with matching headers is installed.

## Install directly

The direct installer is useful for testing without creating a pacman package:

```bash
sudo ./install.sh
```

Do not mix the direct and `makepkg` installation methods. The direct installer
copies files outside pacman's database.

## Verify

```bash
systemctl --no-pager --full status claw-rt721-fix.service
sudo journalctl -b -k --no-pager | grep claw_rt721_amp
```

The service should finish with `status=0/SUCCESS`. Its kernel messages should
show the codec, amplifier, and microphone masks applied without a verification
mismatch. The helper module then unloads; that is expected.

## Suspend/resume

The included system-sleep hook reruns the one-shot service after resume when
the service is enabled. This is needed because the codec may lose the hidden
power state across a sleep transition.

## Remove

For the Arch package:

```bash
sudo systemctl disable --now claw-rt721-fix.service
sudo pacman -Rns claw-rt721-fix-dkms
```

For a direct installation:

```bash
sudo ./uninstall.sh
```

## Important limitation

This workaround writes the D0 sequence and leaves the selected hidden power
bits enabled until the hardware resets. Idle battery impact has not yet been
measured. A proper upstream solution should connect both the D0 and D3
sequences to RT721 runtime power management so unused blocks can power down.

An experimental implementation of that integration is available as the
[RT721 in-driver power-management patch](../../patches/rt721-power-management/).
Cold boot, playback, capture, and runtime D0/D3 transitions are verified;
the driver also restored codec and speaker power before suspend exit across
three s2idle cycles. A helper-independent microphone resume test is still
pending.

See [TECHNICAL.md](TECHNICAL.md) for register-level notes and the upstreaming
direction.
