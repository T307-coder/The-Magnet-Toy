THE ELECTRIC-MAGNET TOY — Classical Electromagnetism for The Powder Toy
======================================================================

By netherpro
GitHub: https://github.com/T307-coder/The-Magnet-Toy
Download: https://github.com/T307-coder/The-Magnet-Toy/releases/tag/v1.0.0


WHY I MADE THIS
----------------------------------------------------------------------

The Powder Toy dev team once rejected magnetic fields:

  "Realistic magnets must simulate a magnetic field with poles.
   It is unclear if this is doable in a simulation like Powder Toy.
   What is absolutely rejected is just making a copy+paste of Newtonian
   Gravity and making it only apply to metallic powders. This is boring
   and uninteresting."

I read this and thought: challenge accepted.

This is not a copy-paste of Newtonian gravity. It is a full classical
electromagnetism simulation with FFT-based Poisson field solvers, real
physical coupling between electric and magnetic phenomena. If the
official game won't do it, the modding community will.


WHAT IT DOES
----------------------------------------------------------------------

Magnetic Fields
  - FFT field solver: real-time Poisson equation in frequency domain
    (3x zero-padded, fftw3f)
  - Permanent magnets (MAGN), electromagnets (ELMG), monopoles (MGPN)
  - 13 conductors detect changing flux and spark (Faraday induction)
  - 4 ferromagnetics become magnetized by contact with internal diffusion
  - B-field breakage: strong fields shatter BMTL into BRMT

Electric Fields
  - Second FFT solver (ElecFFT) for electric potential
  - Electrode plates (POSC): single element, temp polarity
  - Fixed charges (FIXC): persistent charge sources
  - 24 conductors accept charge from POSC, FIXC, ELEC, PROT
  - Charge diffusion between neighbors: a charged wire charges the whole
    circuit

Electro-Magnetic Coupling
  - Coulomb force: F = -q * grad(V), matching ELEC/PROT coefficient
  - Lorentz force: charged particles rotate in B-field, energy-conserving
  - Dielectrophoresis: uncharged conductors pulled toward stronger |E|
  - Gravity-weighted: light particles respond more than heavy ones

Tools
  - PMAG / NMAG: paint positive/negative magnetic field sources
  - PELC / NELC: paint positive/negative electric field sources
  - Sidebar toggle buttons for all field displays


TECHNICAL NOTES
----------------------------------------------------------------------

Both MagFFT and ElecFFT solve the Poisson equation in frequency domain:

  laplacian(phi) = -source   -->   phi_hat(k) = source_hat(k) / (k^2 + 1)

Using a 3x zero-padded grid to eliminate wraparound artifacts. Same
approach used in real computational physics, adapted for a cellular
automaton.

This is not Newtonian gravity repurposed. It is a genuine electromagnetic
field simulation with divergence-free magnetic fields, scalar electric
potential with proper gradient forces, Lorentz coupling, and Faraday
induction.


LINKS
----------------------------------------------------------------------

Source Code:  https://github.com/T307-coder/The-Magnet-Toy
Download:     https://github.com/T307-coder/The-Magnet-Toy/releases/tag/v1.0.0
Branch:       the-electric-magnet-toy
License:      GPLv3 (inherited from TPT)


BUILD IT YOURSELF
----------------------------------------------------------------------

  git clone https://github.com/T307-coder/The-Magnet-Toy.git
  cd The-Magnet-Toy
  git checkout the-electric-magnet-toy
  meson setup build-static -Dstatic=prebuilt --buildtype=release
  ninja -C build-static

Requires: fftw3f, meson, ninja, MSVC or GCC.


To the TPT dev team: it turns out realistic electromagnetism IS doable
in The Powder Toy. And it's anything but boring.
