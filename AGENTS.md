# AGENTS.md - Accretion Architect

## Project Overview
Accretion Architect is a 3D space roguelike built in Unreal Engine 5 for
PC (Windows) and Mac.
Read the README and relevant documentation in `docs/` before substantial changes.

## Project Context
The player controls a growing black hole that travels through solar systems,
consuming matter to gain mass. Feeding raises accretion disk pressure. If the
disk overheats into a runaway blowout, or the black hole starves and
evaporates, the run ends and the player starts over.

Primary project goals include:

- A black hole the player steers through 3D space with a readable
  third-person camera.
- Orbital/gravity simulation: matter is pulled in along spiral orbits
  rather than snapping to the player.
- A mass system: mass is the player's score and increases gravitational pull.
- A disk pressure system: feeding raises pressure, smooth orbital feeding
  is safer than head-on impacts, and pressure slowly vents over time.
- Fail states: disk blowout (pressure maxes out) and starvation (mass decays
  below a threshold).
- Gravity nudges/wells the player places to steer incoming matter.
- Hazards that can end a run, such as stellar flares, neutron star beams,
  and rival black holes.
- Roguelike run structure: zones (asteroid belt, nebula, binary system,
  galactic core), waves, and a choice of random mutations between waves.
- Meta-progression: currency earned on run end that unlocks new starting
  black hole types.
- High-quality space visuals, including a gravitational lensing shader,
  accretion disk effects (Niagara), and jets.
- Support for very large distances using Large World Coordinates.

Prefer implementing gameplay systems in a modular way so additional
mutations, hazards, zones, bosses, and black hole types can be added later.

## Unreal Engine Development Rules

- This is an Unreal Engine 5 C++ project created from the Blank template.
- Target platforms are Windows and macOS. Do not add mobile, console, or VR
  support unless explicitly requested.
- Prefer C++ for core gameplay systems that benefit from maintainability,
  reusable architecture, or source control (gravity, mass, disk pressure,
  run state, spawning).
- Blueprints may be used for configuration, level setup, visual scripting,
  UI, VFX hookups, and designer-adjustable values.
- Expose useful gameplay parameters (pressure rates, gravity strength, decay
  rates, mutation values) to Blueprints using `UPROPERTY` and `UFUNCTION`.
- Prefer Data Assets or Data Tables for mutation, hazard, and black hole
  type definitions so new content does not require new code.
- Follow Unreal naming and class conventions (`A` for Actors, `U` for
  UObjects/components, `F` for structs, `E` for enums, `I` for interfaces).
- Keep code cross-platform: avoid Windows-only or Mac-only APIs unless they
  are wrapped behind platform checks.
- Do not make unnecessary changes to project settings.

### Unreal Generated Files

Do not manually modify generated Unreal Engine files or build artifacts.

Generally do not edit or commit files from:

- `Binaries/`
- `DerivedDataCache/`
- `Intermediate/`
- `Saved/`
- `.vs/`
- Generated IDE files (`*.sln`, `*.xcworkspace`, `.vscode/`)

Do not delete these directories unless specifically needed to resolve an
Unreal build/project-generation issue.

### Unreal Assets
- `.uasset` and `.umap` files are binary Unreal assets tracked with Git LFS.
- Never attempt to edit them as text.
- If a task requires changes that must be performed inside the Unreal Editor,
  explain what needs to be changed in the editor if it cannot be performed
  safely through project source code.
- Do not replace or regenerate unrelated assets.
- Do not add large marketplace packs or Starter Content without approval.

### Gameplay Architecture
Prefer reusable gameplay components rather than placing unrelated logic
directly into the player Pawn class.

Examples:

- Black hole Pawn: movement and input coordination
- Gravity component: pulling and orbiting nearby matter
- Mass component: mass gain, decay, and starvation checks
- Disk pressure component: heating, venting, and blowout checks
- Matter/consumable actors: asteroids, gas clouds, stars, fragments
- Hazard actors: flares, beams, rival black holes
- Mutation system: applying and stacking run upgrades
- GameMode: run rules, zones, waves, and fail state handling
- GameState: current run data (zone, wave, active mutations)
- PlayerController: player-specific control logic and gravity nudges
- SaveGame: meta-progression and unlocks

## Engineering Process
1. Inspect relevant code before editing.
2. Identify material ambiguity.
3. For non-trivial changes, propose a short plan first.
4. Make the smallest reasonable change satisfying approved requirements.
5. Add or update tests (Unreal Automation tests for core gameplay logic).
6. Build and test locally.
7. Review `git status` and `git diff`.
8. Commit the changes locally (see the Git section).
9. Summarize changes and verification.

## Requirements and Scope
- Treat documented requirements as authoritative.
- Do not silently add, remove, or weaken requirements.
- Do not add unrelated features.
- Ask for clarification when ambiguity could materially affect implementation.
- Do not refactor unrelated code unless requested.

## Build, Test, and CI Policy
All development builds and tests must be run locally.

- Run build, test, lint, and formatting commands locally.
- Do not use GitHub Actions or another remote CI/CD service merely to verify changes.
- Do not create, modify, enable, or trigger GitHub Actions workflows unless explicitly requested.
- Do not add CI/CD configuration as part of a feature or bug-fix task unless explicitly requested.
- Existing GitHub Actions workflows are not permission to run or modify them.
- Prefer documented local build and test commands.
- Before reporting completion, run appropriate tests locally and report commands and results.
- If a build or test cannot be run in the current environment, say so
  clearly and list the steps the user should run instead.

## External Services and Resource Usage
This project uses free tools only. Do not create, configure, or use external
or paid services without explicit user approval.

This includes:
- GitHub Actions and hosted CI/CD
- cloud computing resources
- hosted databases
- paid APIs
- paid marketplace assets or plugins
- deployment and distribution platforms
- external monitoring or analytics services
- purchasing additional Git LFS storage or bandwidth

If a task appears to require an external or paid service, explain why and
ask for approval first. Prefer a free or local alternative when one exists.

## AI Interaction Log
Maintain a local interaction log at `.ai-log/ai-interaction-log.md`.

This log is local only. The `.ai-log/` folder is listed in `.gitignore`
and must never be committed or pushed. Do not remove it from `.gitignore`,
and do not move the log into a tracked folder. If the file or folder does
not exist, create it.

For every development-related user request, append:
1. Date and time, if available.
2. The user's prompt exactly as provided.
3. A concise interpretation of the request.
4. Requirements or acceptance criteria.
5. Actions performed.
6. Files created or modified.
7. Commands/tests executed.
8. Verification results.
9. Notes or follow-up work, if applicable.
10. A concise summary of the final response.

Use this format:

---
## Interaction <number>

**Date/Time:** <date and time>

### User Prompt
<exact user prompt>

### Interpretation
<what you understood>

### Requirements / Acceptance Criteria
- <what must be true for the request to be considered complete>

### Actions Taken
<summary>

### Files Changed
- file1
- file2

### Verification
- command/test and result

### Notes / Follow-up
- <remaining work, editor steps, limitations, or future improvements>

### Response Summary
<summary>
---

Append new interactions. Never overwrite or delete earlier entries.
Continue numbering from the last entry in the file.

This is an agent-maintained engineering record, not a guaranteed verbatim
transcript of hidden reasoning, tool internals, or every token produced.

## Git
- Inspect `git status` and `git diff` before finishing.
- Confirm `.ai-log/` does not appear in `git status`.
- Do not modify unrelated files.
- At the end of every task that changes tracked files, stage all files
  changed for that task and create a new local commit so the user can push it.
  - Only commit after the build and tests pass, or after clearly reporting
    any build/test step that could not be run.
  - Use a short summary line followed by a body describing what changed
    and how it was verified.
  - Never stage `.ai-log/`, generated folders, or files listed in `.gitignore`.
  - Do not stage unrelated files. If the working tree contains changes the
    agent did not make, leave them out and mention them to the user.
  - Create a new commit each time; do not amend existing commits unless
    explicitly instructed.
- Do not push to any remote unless explicitly instructed. The user pushes.
