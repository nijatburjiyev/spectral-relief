# Ableton Live Smoke Test

## Install and Discover

1. Quit Ableton Live before replacing an existing local build.
2. Copy `Spectral Relief.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
3. Open Ableton Live 12 Suite.
4. Open Settings → Plug-Ins, enable VST3 system folders, and rescan.
5. Confirm `Spectral Relief` appears under Plug-Ins → VST3 → Nijat Burjiyev.

## Audio Transparency

1. Place a flute clip or instrument on a track.
2. Insert Spectral Relief after the audio source.
3. Toggle Ableton's device activator while listening at matched gain.
4. Confirm there is no audible level, stereo, timing, or tonal change.
5. Confirm Ableton reports no added device latency.

## Spectral Accuracy

1. Set `AVERAGE` to OFF and `SMOOTH` to 0%.
2. Play a steady sine and confirm one narrow ridge appears at its frequency.
3. Play the flute and confirm the fundamental plus separate harmonic ridges remain visible.
4. Confirm frequency increases from bottom to top and history progresses from left to right.
5. Confirm the surface is viewed from above with a slight tilt that reveals peak height.
6. Switch to `2D` and confirm time is exactly left-to-right and frequency exactly bottom-to-top.
7. Set Tilt to 90° and Orbit to 0° in 3D; confirm the axes match the 2D orientation.
8. Confirm logarithmic guides align with sine tones at 50, 100, 200, 500 Hz, 1, 2, 5, and 10 kHz.
9. Change `AVERAGE` while playing a steady tone and confirm its ridge remains at exactly the same frequency.

## Perceptual Averaging

1. Set `SMOOTH` to 0% so only analyzer averaging is under test.
2. Play transient material and compare `AVERAGE` at OFF, 80 ms, and 600 ms.
3. Confirm OFF shows the fastest attacks and decays, 80 ms gives gentle stability, and 600 ms retains energy visibly longer.
4. Confirm rising energy reacts faster than falling energy at the same Average value.
5. Play a sustained flute note and confirm its fundamental and harmonic ridges remain narrow at every setting.
6. Return Average to OFF and confirm temporal detail returns immediately without a stale tail.

## Controls and State

1. Adjust `HEIGHT` and confirm only vertical displacement changes.
2. Set `SMOOTH` to 100% and confirm display motion stabilizes; return it to 0% and confirm display detail returns immediately.
3. Set `HISTORY` to 2 s and 8 s and confirm the visible time span changes.
4. Toggle `HOLD`; audio must continue while the display freezes.
5. Unhold and confirm the display resumes at current audio rather than replaying stale rows.
6. With `AVERAGE` above OFF, press `RESET` and confirm the surface clears and new audio starts with no retained pre-reset loudness.
7. Resize the editor, save the Live Set, close it, reopen it, and confirm parameters and size restore while history starts empty.
8. Cycle `RESOLUTION` through Normal, High, and Ultra and confirm each continues updating.
9. Cycle `RANGE` through Full, Low, Mid, and High and confirm guides and frequency content update.
10. Raise `CONTRAST` and confirm loud/quiet separation increases without moving mountain peaks.
11. Save and reopen the Live Set with a non-default `AVERAGE` value and confirm it restores.

## Lifecycle and Load

1. Open and close the plug-in editor repeatedly during playback; audio must remain uninterrupted.
2. Stop, seek, and restart Ableton's transport; the analyzer must recover immediately.
3. Duplicate the device, then remove both copies during playback; Ableton must remain stable.
4. Observe Ableton CPU usage with the editor open and closed; closing the editor should remove rendering load.
5. Observe Activity Monitor GPU usage while changing editor size; usage should remain bounded and fall when the editor closes.
