# Accretion Architect - Game Design Overview

This document is the source of truth for gameplay requirements. Update it as
design decisions are made.

## Concept
A 3D roguelike where the player is a black hole growing by consuming matter
while managing accretion disk pressure. Runs end on disk blowout or starvation.

## Core Loop
1. Fly through a zone and find matter.
2. Pull matter into orbit and feed on it to gain mass.
3. Manage disk pressure while avoiding hazards.
4. Survive the wave and pick a mutation.
5. Advance to the next zone or lose the run.
6. Spend meta-currency on unlocks and start a new run.

## Systems

### Mass
- Mass is the player's score.
- Higher mass increases gravitational pull radius and strength.
- Mass decays slowly over time. Falling below the starvation threshold ends the run.

### Disk Pressure
- Consuming matter adds pressure.
- Objects that spiral in smoothly add less pressure than head-on impacts.
- Pressure vents slowly over time.
- Reaching maximum pressure triggers a blowout and ends the run.

### Gravity Nudges
- The player places temporary gravity wells to steer incoming matter.
- Limited by cooldown or charges (to be decided).

### Hazards
- Stellar flares
- Neutron star beams
- Rival black holes

### Zones
1. Asteroid belt
2. Nebula
3. Binary star system
4. Galactic core

### Mutations (examples)
- **Ergosphere:** Spinning matter feeds 2x but heats 3x.
- **Magnetic jets:** Vent pressure by firing jets that destroy nearby objects.

### Controls
- Keyboard and mouse only for now; controller support may come later.
- Movement is camera-relative and the movement plane follows the camera:
  W/S move along where the camera looks (including up/down pitch), A/D strafe
  along the camera's right axis, Space/Left Ctrl move along the camera's up axis.
- The black hole accelerates and drifts to a stop rather than halting instantly.
- Players can customize controls in a Controls menu (Tab): rebind every
  movement key (keyboard keys or mouse buttons), adjust mouse sensitivity,
  invert mouse Y, and reset to defaults. Preferences save locally.
- Tab (open/close menu) and Esc (cancel a rebind) are reserved.

### Meta-Progression
- Currency earned at run end.
- Unlocks starting black hole types (e.g., fast-spinning Kerr, charged).

## Open Questions
- Exact pressure and decay formulas.
- Gravity nudge limits (charges vs. cooldown).
- Run length target.
- Additional flight/control schemes beyond camera-relative movement.

## First Prototype Milestone (gray-box)
- Black hole pawn with basic 3D movement and camera.
- Matter actors pulled in by a gravity component.
- Mass and disk pressure components with on-screen meters.
- Blowout and starvation fail states that restart the run.
