# Resident Evil 0 HD Remaster — UI Speed Tweaks

An ASI mod for Resident Evil 0 HD REMASTER that shortens save/load/settings message delays and adjusts shared screen fades, loading narrative text, the partner-switch screen effect and saved-slot typewriter text. It changes UI timing; it does not increase rendering FPS or disk throughput.

[한국어](README.ko.md)

## Supported environment

The 32-bit Steam PC build identifying itself as `MasterRelease Jan 28 2025 16:45:59`, with an existing 32-bit ASI loader that loads the `scripts` folder. The mod can be used in a Windows or Wine installation with that loader. Other game builds are not supported; mismatched instructions cause the mod to skip its changes.

## Installation

1. Exit the game. Back up your current `scripts` folder.
2. Open the folder containing `re0hd.exe`. In Steam, use **Manage → Browse local files**.
3. Copy `scripts/RE0.UISpeedTweaks.asi` and `scripts/RE0.UISpeedTweaks.ini` from the package into the game's `scripts` folder. Create that folder if necessary. Keep your existing ASI loader and other mod files.
4. Start the game. When updating, retain your existing INI if you have customized it.

If upgrading from the earlier NoUIWait version, remove `scripts/RE0.NoUIWait.asi` and rename its INI to `RE0.UISpeedTweaks.ini` to keep your settings. Do not load both versions together.

The package does not include an ASI loader. If you do not already use one, install the **Win32** `dinput8.dll` from [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) beside `re0hd.exe` before starting the game. Back up any existing `dinput8.dll` instead of overwriting it.

## Settings

Edit `scripts/RE0.UISpeedTweaks.ini`, then restart the game:

```ini
[Timing]
WaitSeconds=0.3
FadeSpeed=2.0
RecapTextSpeed=4.0
PartnerTransitionSpeed=2.0
SaveTextSpeed=4.0
SaveTextHoldSeconds=0.3
```

- `WaitSeconds`: minimum save/load/settings UI delay in seconds. Range `0`–`30`, default `0.3`. Use `3` for the original delay. At `0`, auto-closing notices retain a 1ms timer so they still close. Actual storage completion and errors are still checked.
- `FadeSpeed`: speed multiplier for the shared screen fade effect. Range `0.1`–`20`, default `2`. `1` preserves the original speed, `2` halves the duration, and `3` uses one third. Instant transitions stay instant.
- `RecapTextSpeed`: character-reveal multiplier for loading/intro narrative text (common messages 88–95). Range `0.1`–`20`, default `4`. `1` retains the original speed; `4` uses the normal message interval. Other dialogue and the post-text hold are unchanged.
- `PartnerTransitionSpeed`: multiplier for the partner-switch screen effect. Range `0.1`–`20`, default `2`. `1` preserves its original speed. It scales visual progression, hold and decay while keeping transition readiness checks and cleanup. Actual control/camera handoff does not necessarily speed up by the same factor.

- `SaveTextSpeed`: character-reveal multiplier for the saved-slot typewriter animation after a successful save. Range `0.1`–`20`, default `4`; `1` is the original speed. Applies only to the slot being updated, including both lines. Storage completion and line continuation are retained. Typing sounds keep their original pitch and playback speed, with fewer triggers at higher text speeds (about one per four characters at `4`).
- `SaveTextHoldSeconds`: pause after the saved slot finishes typing. Range `0`–`30` seconds, default `0.3`; `2` restores the original pause. `0` continues on the next UI update. This is independent of `WaitSeconds` and `SaveTextSpeed`.

Missing or invalid values use the defaults. `RE0.UISpeedTweaks.log` in the game folder reports whether the changes were applied and which values were read.

## Scope and removal

Fade speed applies to every caller of the shared black, white and custom-color screen fade effect, including scene transitions that use it. Independent popup, cursor and resource animations are outside this path. Storage, asset loading and other work can still take longer than the configured UI delay.

To restore the original behavior, exit the game, remove `scripts/RE0.UISpeedTweaks.asi`, and restart. The INI and log may also be removed. The mod does not modify the game executable or save files.

## Build

Requires Python 3.10+ and `i686-w64-mingw32-gcc` on the build machine:

```sh
python3 build.py
```

The package is written to `dist/RE0-UI-Speed-Tweaks-1.1.1.zip`. Building does not install or launch the game. The game executable and game assets are not required to build.

## License

[MIT](LICENSE). Resident Evil is owned by Capcom; this is an unofficial mod.
