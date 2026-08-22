# Install Spectral Relief

Ready-made VST3 packages are available on the
[Spectral Relief releases page](https://github.com/nijatburjiyev/spectral-relief/releases).
You do not need CMake, JUCE, or a compiler to use them.

> **Unsigned alpha:** The macOS package is ad-hoc signed only—not Developer ID
> signed or notarized—and the Windows package is not Authenticode signed.
> Download only from the official repository above and verify the checksum when
> possible.

## macOS — Apple Silicon and Intel

1. Quit Ableton Live.
2. Download `Spectral-Relief-<version>-macOS-universal.zip` and extract it.
3. In Finder, choose **Go → Go to Folder…** and enter:

   ```text
   ~/Library/Audio/Plug-Ins/VST3
   ```

4. Create the `VST3` folder if it does not exist, then copy
   `Spectral Relief.vst3` into it.
5. Because this alpha is not notarized, open Terminal and remove the quarantine
   attribute from this plug-in only:

   ```bash
   xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/Spectral Relief.vst3"
   ```

6. Open Ableton Live. Under **Settings → Plug-Ins**, enable **Use VST3 Plug-In
   System Folders**, then hold **Option** while clicking **Rescan** if Spectral
   Relief does not appear immediately.

The universal package contains native `arm64` and `x86_64` code. Do not run the
quarantine command against your entire Downloads, Library, or plug-ins folder.

## Windows — 64-bit

1. Quit Ableton Live.
2. Download `Spectral-Relief-<version>-Windows-x64.zip` and extract it.
3. Copy the complete `Spectral Relief.vst3` bundle into:

   ```text
   C:\Program Files\Common Files\VST3
   ```

   Windows may request administrator permission for this copy.
4. Open Ableton Live. Under **Settings → Plug-Ins**, enable **Use VST3 Plug-In
   System Folders**, then hold **Alt** while clicking **Rescan** if necessary.

Use the 64-bit VST3 system folder above, not `Program Files (x86)` and not a
VST2 folder.

## Verify the download

Each GitHub Release includes `SHA256SUMS.txt`. Run the command from the folder
containing the downloaded ZIP and compare the result with that file.

macOS:

```bash
shasum -a 256 Spectral-Relief-<version>-macOS-universal.zip
```

Windows PowerShell:

```powershell
Get-FileHash .\Spectral-Relief-<version>-Windows-x64.zip -Algorithm SHA256
```

## Updating or uninstalling

Quit Ableton before replacing the bundle. To uninstall, remove only
`Spectral Relief.vst3` from the platform folder above and rescan plug-ins.

## Troubleshooting

- Confirm the whole `.vst3` bundle was copied, not a file from inside it.
- Confirm VST3 system folders are enabled in Ableton.
- Perform a deep rescan with **Option/Alt + Rescan**.
- Check the live diagnostics described in the
  [Ableton smoke test](https://github.com/nijatburjiyev/spectral-relief/blob/main/docs/testing/ableton-smoke-test.md).
- Report reproducible problems through
  [GitHub Issues](https://github.com/nijatburjiyev/spectral-relief/issues).

Ableton's official platform guides document the same system folders and scan
settings:

- [Using AU and VST plug-ins on macOS](https://help.ableton.com/hc/en-us/articles/209068929-Using-AU-and-VST-plug-ins-on-macOS)
- [Using VST plug-ins on Windows](https://help.ableton.com/hc/en-us/articles/209071729-Using-VST-plug-ins-on-Windows)
