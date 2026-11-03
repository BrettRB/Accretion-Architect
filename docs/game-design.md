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

### Boost
- Space (rebindable) gives a burst of speed toward where the camera looks,
  so the player can reach distant matter and objects.
- Each boost costs mass (default 5) and has a cooldown (default 1.5 s).
- A boost is refused if paying for it would drop mass below the starvation
  threshold, so boosting alone never ends the run.
- Speed above the normal max bleeds off gradually, so a boost carries the
  black hole a long way. Boosting into matter makes head-on impacts likelier.

### Accretion Disk (visual)
- A glowing disk appears around the black hole while it is consuming matter,
  fades in as feeding starts, and fades out about a second after it stops.
- The disk grows with the event horizon and spins. It tips toward the camera
  so it never disappears edge-on.
- Gray-box version is a procedural ring; a Niagara disk is planned.

### Environment: Proxima Centauri (first zone backdrop)
- Black space with a dense starfield and Milky Way band. Stars are the main
  way to see the black hole: it shows as a silhouette blocking them.
- Proxima Centauri (red dwarf) with Proxima b and Proxima d orbiting it,
  plus Alpha Centauri A/B as the brightest pair in the sky.
- Lighting: dim red key light from Proxima, faint fill light from Alpha
  Centauri so backlit matter is still readable. Exposure is fixed (no auto
  exposure) to keep space dark.
- Planets and the star are scenery for now (not consumable).

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
  along the camera's right axis, E/Left Ctrl move along the camera's up axis.
- Space boosts (see Boost). Move Up moved from Space to E to make room for it.
  Older saved bindings without Boost reset to the defaults.
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
