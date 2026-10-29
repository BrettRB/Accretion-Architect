# Accretion Architect

A 3D space roguelike where you are a growing black hole. Travel through solar
systems, consume matter to gain mass, and keep your accretion disk stable.
Feed too fast and the disk overheats into a blowout. Feed too slowly and you
starve. Either way, the run ends and you start again.

Built in **Unreal Engine 5** for **Windows** and **macOS**.

## Core Gameplay

- **Mass:** Your score and your power. More mass means stronger gravity and
  bigger targets.
- **Disk pressure:** Rises as you feed. Smooth orbital feeding is safe;
  head-on impacts spike pressure. Max it out and your run ends.
- **Starvation:** Mass slowly decays. Fall too low and you evaporate.
- **Gravity nudges:** Place temporary gravity wells to steer matter into
  safe spiral orbits.
- **Hazards:** Stellar flares, neutron star beams, and rival black holes.

## Roguelike Structure

- Runs move through zones: asteroid belt, nebula, binary star system, and
  galactic core.
- Choose one of three random **mutations** between waves to build synergies.
- Earn currency when a run ends to unlock new starting black hole types.

## Tech

- Unreal Engine 5 (C++ with Blueprints)
- Large World Coordinates for solar-system-scale distances
- Niagara for accretion disk, jets, and matter effects
- Custom material shader for gravitational lensing
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
3. Open `AccretionArchitect.uproject` in Unreal Engine 5. If prompted to
   rebuild modules, choose **Yes**.
4. To edit C++, right-click the `.uproject` file and choose
   **Generate Visual Studio project files** (Windows) or
   **Generate Xcode project** (Mac).

### First-Time Project Creation

If the Unreal project has not been created yet:

1. In Unreal, choose **Games > Blank**.
2. Set the project type to **C++**, target platform to **Desktop**, quality
   preset to **Maximum**, Starter Content **off**, and raytracing **off**.
3. Name the project `AccretionArchitect` and create it inside this
   repository folder so the `.uproject` sits next to this README.

## Testing

Core gameplay logic is tested with Unreal Automation tests, run locally:

- In the editor: **Tools > Session Frontend > Automation**, then run tests
  under `AccretionArchitect`.

All builds and tests run locally. This project does not use hosted CI/CD.

## Repository Layout

```
AccretionArchitect/
├── Config/          Project settings
├── Content/         Unreal assets (Git LFS)
├── Source/          C++ gameplay code
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
