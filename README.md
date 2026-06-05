# The Electric-Magnet Toy

*A classical electromagnetism mod for [The Powder Toy](https://powdertoy.co.uk)*

---

## Overview

This mod adds a complete **classical electromagnetism simulation** to The Powder Toy: electric fields, magnetic fields, magnetizable materials, chargeable conductors, dielectric polarization, Lorentz force, dielectrophoresis, and particle-sourced gravity. Fields are computed via asynchronous FFT-based Poisson solvers in real time, with visual overlays and sidebar controls.

---

## New Elements

### Magnetic (3 new elements)
| Element | Menu | Description |
|---|---|---|
| **MAGN** | `SC_SPECIAL` | Permanent magnet. `tmp` = polarity/strength (positive=N/red, negative=S/blue). |
| **ELMG** | `SC_POWERED` | Electromagnet. SPRK to activate (PSCN=on, NSCN=off). Field strength proportional to temperature. |
| **MGPN** | `SC_NUCLEAR` | Magnetic monopole. `tmp` = polarity. Same polarity repels, opposites attract. |

### Electric (2 new elements)
| Element | Menu | Description |
|---|---|---|
| **POSC** | `SC_POWERED` | Electrode plate. Temp > 0C = positive (yellow), < 0C = negative (blue). Single element replaces old POSC/NEGC. |
| **FIXC** | `SC_SPECIAL` | Fixed charge. Like MAGN for E-field. `tmp` = charge strength and polarity. |

### Tools (4 new brushes)
| Tool | Color | Effect |
|---|---|---|
| **PMAG** | Red | Paint positive (N) magnetic field source. |
| **NMAG** | Blue | Paint negative (S) magnetic field source. |
| **PELC** | Gold | Paint positive electric field source. |
| **NELC** | Blue | Paint negative electric field source. |

---

## Key Features

### Electric Field & Charge
- **20 conductors** accept charge by contact with POSC, FIXC, ELEC (electrons), or PROT (protons). Charge stored in `tmp4` (or `tmp3` for LITH).
- **Charge diffusion**: DEUT-style random trade between any `PROP_CONDUCTS` neighbors. A charged wire charges the whole circuit.
- **Coulomb force**: Charged particles experience `F = -q grad(V)` with coefficient 0.5, matching native ELEC/PROT behavior.
- **Dielectrophoresis (DEP)**: Uncharged conductors are pulled toward stronger |E| regions. Polar liquids (WATR, SLTW) respond strongly. Force scales with particle mass via `Gravity` property.
- **Dielectric polarization** (U key): Conductors develop opposite surface charges along the external potential gradient, like real dielectric polarization. Electrons drift toward higher potential (+grad V). Rate-limited at 20% per frame to allow natural depolarization.
- **Free charge fields** (Q key): Controls field production for all non-solid charged particles (ELEC, PROT, powders, liquids, gases, plasma). Solids always produce fields when electricity is enabled.
- **Async E-field solver**: Electric potential computed on a dedicated worker thread via FFT Poisson solver, running in parallel with the B-field solver.

### Magnetic Field & Magnetism
- **13 conductors** detect changing magnetic flux and spark (dB/dt induction): METL, GOLD, TUNG, PTNM, IRON, BMTL, TTAN, TESC, INWR, INST, MERC, BRMT, BREC.
- **4 ferromagnetics** become permanently magnetized near MAGN/ELMG: IRON, BMTL, TTAN, BRMT. Magnetization spreads via DEUT-style diffusion. BMTL shatters into BRMT under strong B-fields.
- **Async B-field solver**: Magnetic field computed on a dedicated worker thread via FFT Poisson solver.

### Electro-Magnetic Coupling
- **Lorentz force**: Charged moving particles deflect in magnetic fields. `dtheta = Bz * q * 0.05 / mass`. Pure rotation preserves kinetic energy. Applied via shared header to all charged conductors.
- **Biot-Savart effect (J key)**: Moving charges (ELEC, PROT, charged conductors) produce their own magnetic field circling around their velocity vector. Solids pushed by PSTN (with Realistic PSTN enabled) also contribute via `tmp5`/`tmp6` effective velocity.
- **SPRK current effect (K key)**: Each SPRK conduction event acts as a current element, producing a magnetic field around the wire. Strength scales with SPRK life. Induced SPRK tags (tmp3=1) are skipped in Biot-Savart to prevent feedback.
- **Magnetic induction (I key)**: Conductors detect changing magnetic flux (dB/dt) and spark. Induced SPRK tags propagate through all 5 conduction paths, preventing feedback loops.

### Particle Gravity Field (L key)
- All particles with `Gravity > 0` contribute mass to the gravitational field proportional to their Gravity property. Heavier elements produce stronger gravity wells.
- Works alongside Newtonian Gravity (N key) for particle-to-particle attraction.

### Plasma & Ionised Fluid EM
- **PLSM** (always) and hot gases (**HYGN**>2000°C, **NBLE**>2000°C, **OXYG**>3000°C, **CO2**>4000°C) exhibit full electromagnetic response: polarisation, charge diffusion, Coulomb force, and Lorentz deflection.
- **LAVA** (>1500°C) acts as an ionic liquid with the same EM behaviour (no SPRK conduction or ferromagnetism).
- **THRM** (>2000°C) conducts charge and inherits it from upstream reactions.
- All five plasma elements accept charge from ELEC/PROT contact, enabling ion thruster configurations.

### Charge Heredity & Universal Force
- Charge (`tmp4`) is preserved across all phase transitions (melt, solidify, shatter) via `part_change_type`.
- Centralised eSrc, Coulomb force, and Lorentz force pass in `BeforeSim` applies to ALL charged particles — even non-conductors that inherited charge from LAVA solidification (enabling the electret effect).
- Particles using `tmp4` for other purposes (PLNT, SEED, STOR, VIRS, ARAY) are excluded from the universal pass.

### Visual Overlays
- **B-field display** (M key): Red=N, Blue=S, gradient dot trails showing field direction.
- **E-field display** (E key): Yellow=positive potential, Cyan=negative potential, gradient dot trails.
- **Gravity field display** (G key): Colored grid overlay showing gravitational potential.
- **Debug HUD** (H key): Shows `GX/GY` (gravity), `Bz` (magnetic), `EX/EY` (electric vector) at mouse position.

### Code Architecture
- **Shared headers**: `ElectricityCommon.h` provides `electricity_chargeContact`, `electricity_diffuseCharge`, `electricity_applyForce`, `electricity_applyLorentz`, `electricity_polarizeCharge`. `MagnetismCommon.h` provides `magnetism_tryInduction`, `magnetism_addBiotSavart`. All 20 conductor elements use these shared functions, eliminating ~770 lines of duplicate code.
- **prevEField**: Previous-frame electric potential saved before each Poisson solve, used by polarization for clean external gradient direction (avoids self-field feedback).

---

## Controls

Sidebar buttons (right column):
| Key | Action |
|---|---|
| **B** | Toggle magnetism simulation |
| **Y** | Toggle electricity simulation |
| **I** | Toggle magnetic induction (dB/dt sparking) |
| **J** | Toggle current magnetic field (Biot-Savart, moving charges) |
| **K** | Toggle SPRK current magnetic field |
| **Q** | Toggle free charge fields (ELEC/PROT produce fields) |
| **U** | Toggle dielectric polarization (E-field gradient) |
| **L** | Toggle particle gravity field (all Gravity>0 particles) |
| **R** | Toggle realistic PSTN (gives velocity to pushed particles) |

Keyboard shortcuts:
| Key | Action |
|---|---|
| **M** | Toggle magnetic field display |
| **E** | Toggle electric field display |
| **G** | Toggle gravity field display |
| **H** | Toggle debug HUD |
| **N** | Toggle Newtonian gravity |

---

## Technical Notes

- **FFT Solvers**: `MagFFT` and `ElecFFT` use `fftw3f` with 3x zero-padded grids. Poisson equation solved in frequency domain with `1/(k^2+1)` kernel. Both solvers run asynchronously on worker threads via the `AsyncFieldSolver::Exchange()` pattern.
- **Biot-Savart**: All three current-to-field paths (moving charges, solids via PSTN, SPRK conduction) share a single `magnetism_addBiotSavart()` function in `MagnetismCommon.h`.
- **Particle fields**: `tmp2` = B-field history, `tmp3` = magnetization / LITH charge / induced-flag, `tmp4` = electric charge (conductors), `tmp5`/`tmp6` = solid effective velocity (PSTN).
- **Force separation**: Charged particles (`tmp4 != 0`) receive pure Coulomb force. Uncharged particles receive pure dielectrophoresis. No mixing.
- **Solids do not move**: Walls accept charge and produce fields but never receive motion forces.
- **Gravity weighting**: `F_effective = F_raw / (Gravity + 0.05)`. Light particles (WATR: 0.10) respond much faster than heavy ones (MERC: 0.30).
- **Polarization**: Direction determined by dominant axis of potential gradient (`prevEField`). Electrons drift toward higher potential. Saturation capped at `|E| * 2.0`, threshold `|E| >= 3.0`, rate-limited to 20% per frame.

---

## Build

```bash
meson setup build-debug --buildtype=debug
ninja -C build-debug

# Portable static exe:
meson setup build-static -Dstatic=prebuilt
ninja -C build-static
```

Requires: `fftw3f`, meson + ninja, MSVC or GCC.

---

## Author

**netherpro** — *"Classical electromagnetism, fully simulated inside a falling-sand game."*

---

## License

Derivative work of [The Powder Toy](https://github.com/The-Powder-Toy/The-Powder-Toy), licensed under GPLv3. This mod inherits the same license.
