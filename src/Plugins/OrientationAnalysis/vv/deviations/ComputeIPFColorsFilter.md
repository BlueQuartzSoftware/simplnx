# Deviations from DREAM3D 6.5.171: ComputeIPFColorsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`GenerateIPFColors`).

Entries are referenced by stable ID (`ComputeIPFColorsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ComputeIPFColorsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeIPFColorsFilter-D1` |
| **Filter UUID** | `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6` |
| **Status** | active |

**Symptom:** On the TSL color scheme, a very small fraction of cells (14 of 343,963, or 0.004%, on the single-phase cubic `so3_cubic_high_ipf_001` dataset) receive an IPF color that differs from a freshly-run DREAM3D 6.5.171 by exactly ±1 in one of the three 8-bit RGB channels.

**Root cause:** *precision + library.* IPF coloring is delegated to a crystallographic library — legacy DREAM3D uses its in-tree **OrientationLib**, SIMPLNX uses **EbsdLib**. Both implement the same standard TSL coloring and agree on the continuous RGB value to well within 1/255, but for the handful of orientations whose computed channel value lands right on a `×255` quantization boundary (observed at 57↔58, 130↔131, 164↔165, …) the minute intermediate-math differences between the two libraries tip the final `static_cast<uint8_t>` by one integer level. The SIMPLNX orchestration (`Algorithms/ComputeIPFColors.cpp`) is otherwise a line-for-line port of `GenerateIPFColors::execute()`; it does not itself perform the color math. Notably, SIMPLNX reproduces the legacy `IPF Colors` array **stored inside the input file byte-for-byte** — the ±1 cells appear only against a fresh run of one particular 6.5.171 binary, confirming the difference is quantization jitter, not an algorithmic change.

**Affected users:** Anyone diffing SIMPLNX IPF-color output against a specific 6.5.171 build at the single-bit level. Visually, and for every downstream use, the images are identical; a ±1/255 channel difference on 0.004% of cells is imperceptible.

**Recommendation:** *either acceptable within tolerance ±1/255.* Neither result is more correct — both are the same continuous color quantized to 8 bits, differing only in rounding direction at the boundary. No legacy patch is warranted (the 6.5.171 output is not wrong).

---

## Note on color schemes with no legacy equivalent

DREAM3D 6.5.171 `GenerateIPFColors` produced **only** the TSL (EDAX/OIM) color scheme. SIMPLNX adds a `Color Key` choice exposing two additional EbsdLib schemes — **PUCM** (Patala / MTEX-style perceptually-uniform) and **Nolze-Hielscher** (MTEX HSV-style). These have no DREAM3D 6.5.171 counterpart, so no legacy comparison is possible or meaningful for them; they are a new feature, not a deviation. Their per-Laue-class correctness is covered upstream by EbsdLib's `PUCMColorKeyTest` and `NolzeHielscherColorKeyTest`.
