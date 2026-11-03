# Accretion Architect

A 3D space roguelike where you are a growing black hole. Start in the Proxima
Centauri system, consume matter, moons, planets, and eventually stars, and
travel through a generated universe of star systems. Become a supermassive
black hole like Sagittarius A\*, with a whole galaxy orbiting you, to win.
Feed too fast and the disk overheats into a blowout. Feed too slowly and you
starve. Either way, the run ends and you start again.

Built in **Unreal Engine 5** for **Windows** and **macOS**.

## Core Gameplay

- **Mass:** Your score and your power. More mass means stronger gravity and
  bigger targets.
- **Disk pressure:** Rises as you feed. Smooth orbital feeding is safe;
  head-on impacts spike pressure. Max it out and your run ends.
- **Starvation:** Mass slowly decays. Fall too low and you evaporate.
- **Eat the universe:** Once you outweigh a moon, planet, or star you can
  tidally shred it; at 10x its mass you swallow it whole.
- **Victory:** Reach supermassive mass and a galaxy forms around you.
- **Boost:** Spend mass for a burst of speed (Space) to reach distant matter.
- **Gravity nudges:** Place temporary gravity wells to steer matter into
  safe spiral orbits.
- **Hazards:** Stellar flares, neutron star beams, and rival black holes.

## Roguelike Structure

- Each run starts from the main menu and takes place in a universe generated
  from a random seed, following realistic rules for star systems.
- Winning (supermassive) shows a victory screen that returns to the main menu;
  losing (blowout or starvation) restarts a new run.
- Planned: zones (asteroid belt, nebula, binary star system, galactic core),
  waves, and a choice of three random **mutations** between waves.
- Planned: currency earned when a run ends unlocks new starting black hole
  types, chosen from the main menu.

## Current Status

The game is a playable gray-box prototype. All visuals are built in C++ from
built-in engine content (no custom art assets yet).

**Implemented**
- Main menu (Start Run, Quit, a starting-black-hole slot for future options).
- Black hole movement, third-person camera, and customizable controls.
- Gravity that pulls matter into spiral orbits; mass, decay, and starvation.
- Disk pressure with smooth vs. head-on feeding, venting, and blowout.
- Boost ability (Space) that spends mass.
- Accretion disk visual that fades in while feeding and out after.
- Dark space backdrop: starfield and Milky Way band; the black hole is seen
  as a silhouette against the stars.
- Streaming, procedurally generated universe of star systems, starting at
  Proxima Centauri (with Proxima d, Proxima b, and a debris belt) next to
  Alpha Centauri A/B.
- Consumable moons, planets, and stars: tidal shredding once you outweigh
  them, swallowing whole at 10x their mass.
- Rules that scale with the black hole's size (speed, decay, pressure).
- Win condition at supermassive mass (4.3 million, Sgr A\*-class): a galaxy
  forms around the black hole, then a victory screen with run stats.
- HUD: mass and progress to supermassive, disk pressure, boost status, and
  the nearest body with what it takes to eat it.

**Planned**
- Gravity nudges, hazards (stellar flares, neutron star beams, rival black
  holes), zones, waves, mutations, and meta-progression.
- Final visuals: Niagara accretion disk and jets, gravitational lensing
  shader, and art for planets and stars.

See [`docs/game-design.md`](docs/game-design.md) for the full design and
tuning details.

## How to Play

Default controls (all movement keys and Boost can be rebound in the Controls
menu):

| Action | Key |
| --- | --- |
| Move forward / back (toward where the camera looks) | W / S |
| Strafe left / right | A / D |
| Move up / down (camera-relative) | E / Left Ctrl |
| Look | Mouse |
| Boost (costs 5% of mass) | Space |
| Controls menu (rebind keys, mouse sensitivity, invert Y) | Tab |

Tips:
- Feed on matter that orbits in smoothly; head-on impacts spike pressure.
- Watch the HUD's nearest-body readout: it tells you the mass needed to shred
  or swallow a body.
- Shredding a body close to your own mass builds pressure fast. Back off to let
  the disk vent.
- When a system runs dry, boost toward the bright stars of neighboring systems.

## Tech

- Unreal Engine 5 (C++ with Blueprints)
- Large World Coordinates for solar-system- and galaxy-scale distances
- Procedural geometry (ProceduralMeshComponent) for the starfield, accretion
  disk, and victory galaxy
- Planned: Niagara for accretion disk, jets, and matter effects
- Planned: custom material shader for gravitational lensing
- Git + Git LFS for version control

## Getting Started

### Requirements

- Unreal Engine 5 (installed through the Epic Games Launcher)
- **Windows:** Visual Studio with the "Game development with C++" workload
- **Mac:** Xcode
- Git and [Git LFS](https://git-lfs.com)

### Setup

1. Install Git LFS once on your machine:
   ```
   git lfs install
   ```
2. Clone the repository:
   ```
   git clone <repository-url>
   ```
3. Open `Accretion_Architect.uproject` in Unreal Engine 5. If prompted to
   rebuild modules, choose **Yes**.
4. To edit C++, right-click the `.uproject` file and choose
   **Generate Visual Studio project files** (Windows) or
   **Generate Xcode project** (Mac).

### Running the Game

- The default map is the engine's empty `Entry` map, and the default game mode
  is the main menu (`AAccretionMenuGameMode`). Standalone runs open the menu;
  **Start Run** begins a run.
- Pressing **Play** in the editor plays the currently open level and also
  starts at the menu. For a clean space view, play from an empty level
  (**File > New Level > Empty Level**). A level with its own landscape or
  terrain still shows it; daytime sky, sun, fog, and clouds are switched off
  automatically.
- To launch straight into a run from the command line:
  ```
  UnrealEditor.exe "%CD%\Accretion_Architect.uproject" "/Engine/Maps/Entry?game=/Script/Accretion_Architect.AccretionGameMode" -game
  ```

### Development Console Commands

Available in non-Shipping builds (open the console with the `~` key):

- `AccretionAddMass <amount>`: add mass, e.g. `AccretionAddMass 4300000` to
  see the victory sequence.
- `AccretionGoTo <name>`: jump next to a loaded planet or star whose name
  contains `<name>`, e.g. `AccretionGoTo Proxima b`.

### First-Time Project Creation

If the Unreal project has not been created yet:

1. In Unreal, choose **Games > Blank**.
2. Set the project type to **C++**, target platform to **Desktop**, quality
   preset to **Maximum**, Starter Content **off**, and raytracing **on**.
3. Name the project `Accretion_Architect` and create it inside this
   repository folder so the `.uproject` sits next to this README.

## Testing

Core gameplay logic is tested with Unreal Automation tests, run locally:

- In the editor: **Tools > Session Frontend > Automation**, then run tests
  under `AccretionArchitect`.
- From the command line (Windows, adjust the engine path as needed):
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Accretion_ArchitectEditor Win64 Development -Project="%CD%\Accretion_Architect.uproject" -WaitMutex
  "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\Accretion_Architect.uproject" -ExecCmds="Automation RunTests AccretionArchitect;Quit" -unattended -nullrhi -nosplash
  ```

All builds and tests run locally. This project does not use hosted CI/CD.

## Repository Layout

```
Accretion_Architect/
├── Config/          Project settings
├── Content/         Unreal assets (Git LFS)
├── Source/Accretion_Architect/   C++ gameplay code
│   ├── Components/  Gravity, mass, disk pressure, boost, accretion disk
│   ├── Core/        Run and menu game modes, shared math, engine content paths
│   ├── Environment/ Space backdrop (starfield, lighting) and victory galaxy
│   ├── Input/       Rebindable controls and saved settings
│   ├── Matter/      Consumable matter and its spawner
│   ├── Player/      Black hole pawn, run and menu player controllers
│   ├── Tests/       Unreal Automation tests
│   ├── UI/          HUD, main menu, victory screen, controls menu
│   └── Universe/    Star system generation, celestial bodies, sector streaming
├── docs/            Design and project documentation
├── AGENTS.md        Instructions for AI coding agents
├── CLAUDE.md        Points Claude Code to AGENTS.md
└── README.md
```

## Notes

- **Git LFS storage:** GitHub's free plan includes a limited amount of LFS
  storage and bandwidth. Avoid committing large unused assets. This project
  does not use paid storage.
- **AI interaction log:** AI coding sessions are logged locally in
  `.ai-log/`, which is excluded from the repository by `.gitignore`.
- **Mac distribution:** Sharing Mac builds with other players without
  security warnings requires Apple code signing and notarization. This is
  not needed for local development.
