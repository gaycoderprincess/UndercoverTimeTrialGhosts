# Undercover Time Trial Ghosts

A mod for Need for Speed: Undercover that replaces opponents with TrackMania-like replays

## Installation

- Make sure you have v1.0.0.1 of the game, as this is the only version this plugin is compatible with. (exe size of 10584064 or 10589456 bytes)
- Plop the files into your game folder.
- Start the game and press F5 to open the mod's menu and play the included Challenge Series.
- Enjoy, nya~ :3

Please note that due to the broken nature of this game and its lack of quick races, it's recommended to use a completed savegame, and to be ready for the game potentially crashing or otherwise corrupting itself every few races!

## Features

- A built-in set of events in the Challenge Series to compete with other players on
- Ghosts are timed based on game ticks, meaning the timebug no longer applies anymore
- Ghosts include a checksum of the player's game files, allowing you to check if someone's made any potentially unfair VLT modifications
- Ghosts include player inputs, and while they're not directly usable for playback (the game is not deterministic) it can still be used to verify legitimacy
- Support for Undercover Reformed, cleanly separating ghosts on the vanilla game and ghosts on Reformed, with the Most Wanted and The Run handling options further being separated
- The mod includes a basic anti-cheat, preventing common methods of cheating
- The physics tickrate has been locked to 120, meaning Widescreen Fix no longer gives you an unfair advantage

## Building

Building is done on an Arch Linux system with CLion and vcpkg being used for the build process. 

Before you begin, clone [nya-common](https://github.com/gaycoderprincess/nya-common), [nya-common-nfsuc](https://github.com/gaycoderprincess/nya-common-nfsuc) and [CwoeeMenuLib](https://github.com/gaycoderprincess/CwoeeMenuLib) to folders next to this one, so they can be found.

Required packages: `mingw-w64-gcc`

You should be able to build the project now in CLion.
