# RT721 in-driver power-management patch

This patch integrates the MSI Claw 8 EX AI+ RT721 analog power sequences into
the existing codec driver. Unlike the one-shot DKMS workaround, it powers the
codec, speaker amplifier, and microphone up for use and back down when idle.

## Hardware scope

The quirk is gated by all of these DMI identifiers:

- system vendor: `Micro-Star International Co., Ltd.`
- product: `Claw 8 EX AI+ CG3EM`
- board: `MS-1T91`

It is not intended for the Claw 8 AI+ A2VM/non-EX model.

## Patch

- [`0001-ASoC-rt721-sdca-add-MSI-Claw-8-EX-power-sequencing.patch`](0001-ASoC-rt721-sdca-add-MSI-Claw-8-EX-power-sequencing.patch)
- base: upstream Linux `v7.2-rc7`
- tested kernel: CachyOS `7.2.0-rc7-1-cachyos-rc`

The upstream and CachyOS RT721 source files were byte-identical for that
release. Apply the patch from the root of a matching kernel tree:

```bash
git am /path/to/0001-ASoC-rt721-sdca-add-MSI-Claw-8-EX-power-sequencing.patch
```

For another kernel release, inspect and resolve any conflicts rather than
assuming the codec's runtime-PM paths are unchanged.

## Validation status

Verified on the exact hardware above:

- cold boot with the patched `snd_soc_rt721_sdca` module
- complete SOF HiFi topology with speaker and SoundWire microphone nodes
- audible internal-speaker playback
- four-second internal-microphone capture at 48 kHz stereo, with real samples
  (mean `-13.9 dB`, peak `-4.5 dB` in the first run)
- codec runtime D3 followed by clean D0 reactivation
- amplifier D0 at playback start and D3 after the stream closed
- microphone D0 at capture start and D3 after the stream closed
- no MBQ access, power-sequence, verification, SoundWire bus-clash, or retry
  errors during those tests

System suspend/resume without the legacy post-resume helper is still pending.
Treat the patch as experimental until that test passes.

## Upstream submission note

The exported patch intentionally has no `Signed-off-by` trailer. Add your own
only after reviewing the change and confirming that you can make the Linux
Developer Certificate of Origin certification. The structured `Assisted-by`
trailer records the AI assistance used while developing and testing it.

## Relationship to the workaround

The [DKMS workaround](../../fixes/audio-rt721/) remains the recovery path. Its
boot service and post-resume hook are redundant with this patch and can mask a
resume regression, so disable them for a clean patched-kernel suspend test.
