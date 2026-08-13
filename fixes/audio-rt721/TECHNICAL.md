# Technical notes

## Root cause

SOF firmware, topology, ALSA UCM, PipeWire, and WirePlumber all loaded
successfully. The correct `sof-soundwire` card, speaker sink, and microphone
source were selected. Direct ALSA playback completed without an error, while
microphone captures contained only zero samples.

Active-stream register reads showed that Linux did not execute the MSI-specific
RT721 analog power-up sequence. The register sequence was recovered from the
audio package for PCI subsystem `1462:1510`; no vendor binary or decoded data is
included here.

## D0 sequence represented by the module

The codec-wide sequence sets these controls in vendor order:

- `0x05f00000`: bits 15, 14, 13, 12, 3, 10, 2, and 9
- `0x00100011`: bit 15
- `0x0010000c`: bit 15
- `0x00100013`: bit 6
- `0x0010000e`: bit 3
- `0x0610001a`: toggle bit 15 on, then off

The amplifier register `0x05f00020` and microphone register `0x05f00030` use
the ordered bit sequence 11, 10, 5, 0, 9, 4, 2, 12, 13, 14, 15. The resulting
controlled mask is `0xfe35`.

The HID trigger is hardware-owned and may reassert immediately, so it is
executed but not treated as persistent state during verification. All other
checks compare only the controlled mask, preserving unrelated status bits.

## Runtime behavior

The module:

1. Checks the exact DMI product and board.
2. Locates the exact RT721 SoundWire device.
3. Runtime-resumes it.
4. Creates a temporary MBQ regmap.
5. Applies and verifies the codec, amplifier, and microphone sequences.
6. Releases runtime-PM and device references when unloaded.

The systemd service loads the module with all apply parameters, then unloads
it immediately. A post-resume hook repeats that one-shot operation.

## Upstream direction

The standalone module is deliberately narrow and temporary. The maintainable
fix is a DMI/subsystem-specific quirk in the RT721 codec path that runs the D0
sequence on activation and the matching D3 sequence on power-down. That would
restore normal runtime power management and avoid a possible idle-power cost.

That design is implemented in the experimental
[RT721 power-management patch](../../patches/rt721-power-management/). It has
passed cold-boot and runtime-PM testing on the Claw 8 EX; a clean
suspend/resume test remains before it can replace this fallback.
