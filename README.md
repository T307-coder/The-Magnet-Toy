# The Electric-Magnet Toy

*A classical electromagnetism mod for [The Powder Toy](https://powdertoy.co.uk)*

---

## Overview

This mod adds a complete **classical electromagnetism simulation** to The Powder Toy: magnetic fields, electric fields, magnetizable materials, chargeable conductors, Lorentz force, and dielectrophoresis. Both fields are computed via FFT-based Poisson solvers in real time, with visual overlays and sidebar controls.

---

## New Elements

### Magnetic (3 new elements)
| Element | Menu | Description |
|---|---|---|
| **MAGN** | `SC_SPECIAL` | Permanent magnet. `tmp` = polarity/strength (positive=N/red, negative=S/blue). |
| **ELMG** | `SC_POWERED` | Electromagnet. SPRK to activate (PSCN=on, NSCN=off). Field strength ∝ temperature. |
| **MGPN** | `SC_NUCLEAR` | Magnetic monopole. `tmp` = polarity. Same polarity repels, opposites attract. |

### Electric (2 new elements)
| Element | Menu | Description |
|---|---|---|
| **POSC** | `SC_POWERED` | Electrode plate. Temp > 0°C = positive (yellow), < 0°C = negative (blue). Single element replaces old POSC/NEGC. |
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

### 🧲 Magnetization & Induction
- **13 conductors** detect changing magnetic flux and spark (dB/dt induction): METL, GOLD, TUNG, PTNM, IRON, BMTL, TTAN, TESC, INWR, INST, MERC, BRMT, BREC.
- **4 ferromagnetics** become permanently magnetized near MAGN/ELMG: IRON, BMTL, TTAN, BRMT. Magnetization spreads via DEUT-style diffusion. BMTL shatters into BRMT under strong B-fields.

### ⚡ Electrification & Charge
- **24 conductors** accept charge by contact with POSC, FIXC, ELEC (electrons), or PROT (protons). Charge stored in `tmp4` (or `tmp3` for LITH).
- **Charge diffusion**: DEUT-style random trade between any `PROP_CONDUCTS` neighbors. A charged wire charges the whole circuit.
- Charged solids produce their own electric field (eSrc contribution).

### 🔁 Electro-Magnetic Coupling
- **Lorentz force**: Charged moving particles deflect in magnetic fields. `dθ = Bz × q × 0.05 / mass`. Pure rotation preserves kinetic energy.
- **ELEC/PROT standard**: All Coulomb forces use coefficient 0.5, matching the native ELEC/PROT behavior exactly.

### 💧 Dielectrophoresis
- Uncharged conductors are pulled toward stronger |E| regions (polarization force).
- Polar liquids (WATR, SLTW) respond strongly — water bends toward charged objects.
- Force automatically scales with particle mass via `Gravity` property: light particles move faster.

### 📊 Visual Overlays
- **B-field display** (E button): Red=N, Blue=S, gradient dot trails.
- **E-field display** (E button): Yellow=positive, Cyan=negative, gradient dot trails.
- **Debug HUD** (H key): Shows `GX/GY` (gravity), `Bz` (magnetic), `EX/EY` (electric vector) at mouse position.

---

## Controls

| Key | Action |
|---|---|
| **M** | Toggle magnetism simulation |
| **B** | Toggle magnetic field display |
| **E** | Toggle electric field display |
| **Y** | Toggle electricity simulation |
| **H** | Toggle debug HUD |

---

## Technical Notes

- **FFT Solvers**: `MagFFT` and `ElecFFT` use `fftw3f` with 3× zero-padded grids. Poisson equation `∇²φ = -source` solved in frequency domain with `1/(k²+1)` kernel.
- **Force Separation**: Charged particles (`tmp4 ≠ 0`) → pure Coulomb. Uncharged → pure dielectrophoresis. No mixing.
- **Solids Don't Move**: Walls accept charge and produce fields but never receive motion forces.
- **Gravity Weighting**: `F_effective = F_raw / (Gravity + 0.05)`. Light particles (WATR: 0.10 → 6.7×) respond much faster than heavy ones (MERC: 0.30 → 2.9×).

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
