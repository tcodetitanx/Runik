# Runik - Steam Release Checklist

The project ships Steam-ready in configuration; finishing the release is
paperwork plus swapping one AppId.

## Already configured

- `Runik.uproject` enables the **OnlineSubsystemSteam** plugin.
- `Config/DefaultEngine.ini` sets Steam as the default platform service with
  `SteamDevAppId=480` (Valve's public test AppId - the game boots with or
  without Steam running).
- `steam_appid.txt` (repo root) - copy it next to the packaged executable for
  non-Steam-launched test runs.
- Packaging script: `Tools/package_win64.bat` (BuildCookRun, Shipping, pak).
- Raw art/audio directories are staged via `DirectoriesToAlwaysStageAsNonUFS`.

## To ship

1. **Partner setup**: create the app in Steamworks; note the real AppId.
2. Replace `480` in `Config/DefaultEngine.ini` and `steam_appid.txt`.
3. Package: `Tools\package_win64.bat "C:\Program Files\Epic Games\UE_5.8"`.
   Output lands in `Packaged/Windows`.
4. Upload with SteamPipe (`steamcmd` + app/depot vdf, content folder =
   `Packaged/Windows`). Set the launch option to `Runik.exe`.
5. Steam Overlay works out of the box with the OnlineSubsystemSteam plugin;
   verify with a Steam-launched build.
6. **Do not ship `steam_appid.txt`** in the depot (dev convenience only).

## Suggested achievements (hooks already in the save system)

| API name | Trigger (from FRunikSave) |
| --- | --- |
| ACH_FIRST_MARK | first puzzle solved |
| ACH_CHAPTER_1..5 | all puzzles of chapter N solved |
| ACH_AETHERSTONE | c5p3 solved |
| ACH_UNDER_PAR | any puzzle solved at or under par cost |
| ACH_MINIMALIST | any puzzle solved with runes <= par runes |
| ACH_ARCHIVIST | a GIF exported |

Wire them in `SPuzzleBoard::RecordWin()` via the OnlineSubsystem achievements
interface once the real AppId exists (stats are already tracked per puzzle).

## Store asset notes

- Capsule art: the gem sheet + rune glyphs composite well on the dark hex
  board (see `Content/SourceArt/board.png`).
- Trailer: use the in-game GIF exporter output as animatic material
  (`Saved/Gifs/*.gif`).
