# Jovian Harmony - Six Moons of Jupiter (v0.1 prototype)

Cross-platform JUCE VST3 instrument and MIDI generator for REAPER (Windows and macOS).

## Voices and orbital periods

| Moon | Mean orbital period (Earth days) | Rotation |
|---|---:|---|
| Io | 1.76914 | Synchronous / tidally locked |
| Europa | 3.55118 | Synchronous |
| Ganymede | 7.15455 | Synchronous |
| Callisto | 16.68902 | Synchronous |
| Amalthea | 0.498179 | Synchronous |
| Himalia | about 250.56 | Approximate rotation 7.8 hours |

Io, Europa, and Ganymede have an approximately 4:2:1 **orbital frequency** relationship (Laplace resonance). Their resonance is not mathematically exact 4:2:1. The orbital and spin parameters are approximations, not an ephemeris.

## What the keyboard does

Holding a MIDI key turns on all six moon voices. **Io receives the played pitch** (Jupiter is the visual reference body, not a sounding oscillator). The other five moon pitches derive from `Io_period / moon_period`, with True Ratios, Octave Folded and Compressed modes. Releasing the last key stops the notes. Most-recent held key takes priority.

### Orbital controls

- Each moon has its own MIDI channel (1-16), on/off and level.
- Four gates per orbit, with adjustable note length. Periods reflect actual orbital ratios; **pulse timing is uniform mean anomaly**, not four equal geometric arcs.
- Orbital distance (proximity to Jupiter), orbital position (pan), and rotation (sidereal spin) each have a separate on/off switch **and CC number selector 1-119**. Defaults: CC11, CC10, CC74.
- These CC messages continue between note pulses, while a keyboard key is held. The same CC number on the same channel for two sources will conflict. Use separate MIDI channels for independent pitch bend.
- Jupiter itself has no selectable oscillator or global rotation CC in this prototype.
- Ellipse exaggeration is a visualization/mapping choice, not an alteration of the actual orbit.
- Fast sidereal rotations can alias at the CC update rate (~50Hz). Note scheduling is block-rate; this early prototype is not sample-accurate.

## Build and install

Upload the contents of this project's root (including hidden `.github`) to a **new repository** named e.g. `Jovian-Harmony`. Do not overwrite the original Solar System project. The GitHub Actions workflow uses Windows 2022 and macOS 14 runners, uploads `.vst3` binaries as artifacts.

Install Windows VST3 to `C:\Program Files\Common Files\VST3\` or macOS VST3 to `~/Library/Audio/Plug-Ins/VST3/`, then rescan REAPER. Mac unsigned plugin installation may need security approval.

**Status:** Buildable source prototype; the package structure was checked, but compilation inside JUCE and playback inside REAPER were not tested here. Test each platform before distributing.
