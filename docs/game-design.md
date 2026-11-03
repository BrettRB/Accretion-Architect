# Accretion Architect - Game Design Overview

This document is the source of truth for gameplay requirements. Update it as
design decisions are made.

## Concept
A 3D roguelike where the player is a black hole growing by consuming matter,
moons, planets, and stars while managing accretion disk pressure. The goal is
to become a **supermassive black hole** like Sagittarius A\*, with a whole
galaxy orbiting it. Runs end in victory, or in defeat by disk blowout or
starvation.

## Core Loop
1. Start as a small black hole in the Proxima Centauri system.
2. Pull matter into orbit and feed on it to gain mass.
3. Grow enough to shred, then swallow, moons, planets, and stars.
4. Manage disk pressure while avoiding hazards.
5. Leave the system for neighboring ones when it can no longer feed you.
6. (Planned) Survive waves and pick mutations; advance through zones.
7. Reach supermassive mass to win, or lose the run.
8. Return to the main menu, choose starting options (planned), start again.

## Win Condition
- The run is won when mass reaches the supermassive threshold
  (`AAccretionGameMode::SupermassiveMass`, default 4,300,000, a nod to Sgr A\*'s
  4.3 million solar masses).
- On victory the black hole freezes, a spiral galaxy forms around it and
  rotates, and the camera pulls back to show it all.
- A victory screen (like Slay the Spire's) shows run stats (final mass, run
  time, stars/planets/moons consumed, systems visited) and a **Return to Menu**
  button.
- The HUD mass bar shows progress toward supermassive on a log scale, so each
  doubling of mass moves it the same amount.

## Main Menu
- The game starts at the main menu (the default game mode). Start Run begins a
  run; Quit exits.
- A "Starting Black Hole" section holds the single starting type (Stellar
  Remnant) for now. More starting options will be added there later with
  meta-progression.
- Losing a run (blowout or starvation) restarts a new run directly after a short
  delay; winning goes to the victory screen and then the menu.

## Systems

### Mass
- Mass is the player's score.
- Higher mass increases gravitational pull radius and strength.
- Mass decays over time. Falling below the starvation threshold ends the run.
- Decay scales with size: at the starting mass it is 1% per second; bigger
  black holes lose more in absolute terms but a smaller share of their mass
  (`loss = rate * reference * (mass / reference) ^ 0.7`).

### Disk Pressure
- Consuming matter adds pressure.
- Objects that spiral in smoothly add less pressure than head-on impacts.
- Pressure vents slowly over time.
- Reaching maximum pressure triggers a blowout and ends the run.
- Pressure is relative to the black hole's size, like the Eddington limit:
  a meal's pressure is divided by (mass / reference mass). A planet strains a
  large black hole about as much as a rock strains a small one.

### Eating Moons, Planets, and Stars
Bodies are eaten in a sequence as the black hole grows:
1. **Too massive:** while the black hole is lighter than the body, it cannot
   eat it. The HUD shows the mass needed.
2. **Tidal shredding:** once the black hole is at least as massive as the body
   (`ShredMassRatio`, default 1x), getting the body's surface inside the pull
   radius tears it apart. Fragments stream off it and spiral into the black
   hole, the body shrinks, and it is gone once almost all its mass is
   stripped. The black hole can strip at most 1% of its own mass per second,
   so shredding something close to your own size takes a while. The stream
   still builds pressure, and the player may need to back off to let the disk
   vent.
3. **Swallow whole:** at 10x the body's mass (`SwallowMassRatio`), the body is
   pulled off its orbit, falls in, and is eaten in one gulp on touching the
   event horizon. A gulp adds a big burst of pressure.
- Bodies keep orbiting until eaten, so the player has to chase them.
- Fragments torn from stars glow like the star.
- Mass ladder (game units): moons 30-150, rocky planets 150-600, terrestrial
  600-2,000, ice giants 5k-15k, gas giants 15k-60k, red dwarfs 40k-150k,
  orange 150k-300k, yellow 300k-450k, F 450k-650k, A 650k-1M, blue giants
  1M-2.5M, blue supergiants 3M-6M, white dwarfs 200k-500k, red giants 300k-800k.
  Body types are defined in `UCelestialCatalog` (a Data Asset with code
  defaults).

### Universe Generation
- Space is divided into cubic sectors (8 km by default), streamed around the
  player. Each sector holds at most one star system, generated
  deterministically from the run seed (random per run) and the sector, so
  returning to a sector shows the same system.
- Eaten bodies stay eaten. Partly shredded bodies keep their remaining mass
  when their sector unloads and reloads.
- Hand-made systems: **Proxima Centauri** (start: red dwarf, Proxima d, Proxima
  b, and a debris belt) and its real neighbor **Alpha Centauri** (A/B binary
  with the candidate gas giant Alpha Centauri Ab).
- Generated systems follow simple astrophysical rules:
  - Mostly single stars, ~30% binaries, ~5% hierarchical triples. Never more,
    and never a star orbiting a planet.
  - Star types weighted by real frequency (red dwarfs most common).
  - Companion stars are no heavier than the primary. Close binaries host
    circumbinary planets far outside the pair; wide binaries host planets
    around the primary only. Pairs too big to fit become single stars.
  - Rocky worlds form inside the star's frost line, giants mostly beyond it.
    Red giants have lost their inner planets; white dwarfs keep only rocky
    cores.
  - Planets are at most 10% of their star's mass; moons at most 20% of their
    planet's. Moons orbit planets, never stars.
  - Orbits are spaced outward geometrically (1.4-1.9x) and stay inside the
    system's radius. Orbital periods follow Kepler's third law.
  - Some systems have an asteroid belt at the frost line, spawned while the
    player is nearby.
- Each star lights its own system; a faint global fill light keeps backlit
  objects readable.

### Scaling With Size
Rules scale with the black hole so the late game plays like the early game:
- Movement speed, acceleration, boost speed, gravity pull strength, and
  ambient matter spawn distances scale with the event horizon size.
- Ambient rock masses scale with mass ^ 0.7 (matching decay).
- Boost costs 5% of current mass.
- Decay and disk pressure scale as described above.

### Boost
- Space (rebindable) gives a burst of speed toward where the camera looks,
  so the player can reach distant matter and other star systems.
- Each boost costs 5% of current mass and has a cooldown (default 1.5 s).
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

### Space Backdrop
- Black space with a dense starfield and Milky Way band. Stars are the main
  way to see the black hole: it shows as a silhouette blocking them.
- Exposure is fixed (no auto exposure) to keep space dark.

### Gravity Nudges (planned)
- The player places temporary gravity wells to steer incoming matter.
- Limited by cooldown or charges (to be decided).

### Hazards (planned)
- Stellar flares
- Neutron star beams
- Rival black holes

### Zones (planned)
1. Asteroid belt
2. Nebula
3. Binary star system
4. Galactic core

### Mutations (planned, examples)
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

### Development Console Commands (not in Shipping builds)
- `AccretionAddMass <amount>`: add mass (e.g. `AccretionAddMass 4300000` to
  see the victory sequence).
- `AccretionGoTo <name>`: jump next to a loaded body whose name contains
  `<name>` (e.g. `AccretionGoTo Proxima b`).

### Meta-Progression (planned)
- Currency earned at run end.
- Unlocks starting black hole types (e.g., fast-spinning Kerr, charged),
  chosen in the main menu.

## Open Questions
- Gravity nudge limits (charges vs. cooldown).
- Run length target; tuning of decay, pressure, shred rate, and travel speed
  between systems.
- How zones, waves, and mutations fit the open, generated universe.
- What meta-currency a victory awards versus a loss.
- Additional flight/control schemes beyond camera-relative movement.

## Milestones

### First Prototype Milestone (gray-box) - done
- Black hole pawn with basic 3D movement and camera.
- Matter actors pulled in by a gravity component.
- Mass and disk pressure components with on-screen meters.
- Blowout and starvation fail states that restart the run.
- Customizable controls (Tab menu).

### Second Milestone (gray-box) - done
- Dark space backdrop with starfield; boost; accretion disk visual.
- Consumable moons, planets, and stars (shred, then swallow whole).
- Streaming, rule-based generated universe starting at Proxima Centauri.
- Size scaling of decay, pressure, movement, and boost.
- Supermassive win condition with galaxy, victory screen, and main menu.

### Not yet implemented
- Gravity nudges, hazards, zones, waves, mutations, meta-progression and
  starting options.
- Final visuals (Niagara disk and jets, lensing shader, planet/star art).
