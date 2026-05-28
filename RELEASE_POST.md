# The Electric-Magnet Toy — Classical Electromagnetism for The Powder Toy

**By netherpro** | [GitHub](https://github.com/T307-coder/The-Magnet-Toy) | [Download](https://github.com/T307-coder/The-Magnet-Toy/releases)

---

## Why I Made This

The Powder Toy dev team once rejected magnetic fields with this reasoning:

> *"Realistic magnets must simulate a magnetic field with poles. It is unclear if this is doable in a simulation like Powder Toy. What is absolutely rejected is just making a copy+paste of Newtonian Gravity and making it only apply to metallic powders. This is boring and uninteresting."*

I read this and thought: challenge accepted.

This is not a copy-paste of Newtonian gravity. It is a full classical electromagnetism simulation with FFT-based Poisson field solvers, Maxwell's equations in 2D, and genuine physical coupling between electric and magnetic phenomena. If the official game won't do it, the modding community will.

---

## What It Does

### Magnetic Fields — *Not just "attract metal"*
- **FFT field solver**: real-time Poisson equation in frequency domain (3× zero-padded, `fftw3f`)
- **Permanent magnets** (MAGN), **electromagnets** (ELMG), **magnetic monopoles** (MGPN)
- **13 conductors** detect changing flux and spark: Faraday's law of induction
- **4 ferromagnetics** become magnetized by contact, with internal diffusion
- **B-field breakage**: strong fields shatter BMTL → BRMT

### Electric Fields — *Charge, not cheerios*
- **Second FFT solver** (ElecFFT) for electric potential
- **Electrode plates** (POSC): single element, temp polarity
- **Fixed charges** (FIXC): persistent charge sources
- **24 conductors** accept charge from POSC, FIXC, ELEC, PROT
- **Charge diffusion** between neighbors — a charged wire charges the whole circuit

### Electro-Magnetic Coupling — *They talk to each other*
- **Coulomb force**: $F = -q\nabla V$, matching ELEC/PROT coefficient exactly
- **Lorentz force**: charged particles rotate in B-field, energy-conserving
- **Dielectrophoresis**: uncharged conductors pulled toward stronger |E|
- **Gravity-weighted**: light particles respond more than heavy ones

### Tools
- `PMAG` / `NMAG` — paint magnetic sources
- `PELC` / `NELC` — paint electric sources
- Sidebar toggle buttons for all field displays

---

## Technical Implementation

The magnetic field `MagFFT` and electric field `ElecFFT` both solve:

$$\nabla^2 \phi = -\rho \quad \text{→} \quad \hat{\phi}(k) = \frac{\hat{\rho}(k)}{k^2 + 1}$$

in the frequency domain with a 3× zero-padded grid to eliminate wraparound artifacts. This is the same mathematical approach used in real computational physics, adapted for a cellular automaton.

**This is not Newtonian gravity repurposed.** It is a genuine electromagnetic field simulation with:
- Divergence-free magnetic fields ($\nabla \cdot B = 0$, approximated via FFT)
- Scalar electric potential with proper gradient forces
- Lorentz coupling between E and B fields on charged particles
- Induction (time-varying magnetic flux → electric current)

---

## Links

| Resource | URL |
|---|---|
| **Source Code** | https://github.com/T307-coder/The-Magnet-Toy |
| **Download** | https://github.com/T307-coder/The-Magnet-Toy/releases/tag/v1.0.0 |
| **Branch** | `the-electric-magnet-toy` |
| **License** | GPLv3 (inherited from TPT) |

---

## Build It Yourself

```bash
git clone https://github.com/T307-coder/The-Magnet-Toy.git
cd The-Magnet-Toy
git checkout the-electric-magnet-toy
meson setup build-static -Dstatic=prebuilt --buildtype=release
ninja -C build-static
```

Requires: `fftw3f`, meson, ninja, MSVC or GCC.

---

*To the TPT dev team: it turns out realistic electromagnetism IS doable in The Powder Toy. And it's anything but boring.*
