# Deviations from DREAM.3D v6.5.171: ComputeIPFColorsFilter

Part 1 records the TSL color difference from DREAM.3D v6.5.171 (D1). Part 2 records two fixed implementation defects (D2 and D3).

Entries use the stable ID sequence `ComputeIPFColorsFilter-D<N>`. The Filter UUID is the permanent cross-reference anchor. Source and comparison evidence are in `../provenance/ComputeIPFColorsFilter.md`.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeIPFColorsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeIPFColorsFilter-D1` |
| **Filter UUID** | `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6` |
| **Status** | active |

**Symptom:** On the 343,963-cell cubic fixture, DREAM3D-NX v7.4.1 and v7.4.2 each differ from DREAM.3D v6.5.171 at 14 cells. Each difference is one level in one 8-bit RGB channel.

**Root cause:** Precision and library. DREAM.3D v6.5.171 uses OrientationLib; SIMPLNX uses EbsdLib. Both calculate TSL colors and pack them into integer RGB channels. Small differences in the library calculation can change the integer at a quantization boundary. Both tested DREAM3D-NX releases match the embedded legacy color array exactly.

**Affected users:** Users who compare TSL color arrays byte for byte with DREAM.3D v6.5.171 output.

**Recommendation:** Either result is acceptable within one 8-bit channel level on this fixture. Use that tolerance for this comparison; do not treat the stored legacy colors as an independent correctness oracle.

**Evidence:** Run against DREAM.3D v6.5.171 on 2026-09-17: 343,949 of 343,963 RGB tuples match exactly for each tested DREAM3D-NX release; all tuples match within one channel level. See `vv_work:ComputeIPFColorsFilter/README.md` and `vv_work:ComputeIPFColorsFilter/results/release_matrix/comparison.json`. These results describe v7.4.1 and v7.4.2, not the current algorithm.

DREAM.3D v6.5.171 supports only TSL. SIMPLNX also supplies PUCM and Nolze-Hielscher color keys. The current `ColorKey choice reaches algorithm` test checks that each additional key differs from TSL.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeIPFColorsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeIPFColorsFilter-D2` |
| **Filter UUID** | `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Parallel processing of multiple out-of-range phase IDs caused a data race in the error counter and could lose increments.

**Root cause:** Bug. Workers called `incrementPhaseWarningCount()` on one shared `int32_t`. The increment had no synchronization. `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeIPFColorsDirect.hpp` now declares `std::atomic_int32_t`; the Direct worker increments that atomic counter. Scanline counts invalid phases in its serial loop.

**Affected users:** Users with phase IDs at or above the ensemble tuple count during parallel execution.

**Recommendation:** Use DREAM3D-NX v7.4.2 or later. Correct the invalid phase IDs reported by error `-48000`.

**Evidence:** The released source uses a plain counter through v7.4.1 and an atomic counter in v7.4.2. DREAM.3D v6.5.171 also increments a plain counter from parallel workers. The current `phase index out of range returns -48000` test checks the error code for one invalid cell; it does not test concurrent increments.

---

### ComputeIPFColorsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeIPFColorsFilter-D3` |
| **Filter UUID** | `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** An enabled cell with a negative phase ID could cause an out-of-bounds read from Crystal Structures.

**Root cause:** Bug. The coloring guard checked only `phase < numPhases` before indexing `crystalStructures[phase]`. A negative value passed this check. Both current executors reject an enabled negative phase with `-48001` before the lookup. A masked negative phase does not participate.

**Affected users:** Users whose Phases array contains negative values at enabled cells, including all cells when masking is disabled.

**Recommendation:** Use the corrected filter and replace invalid phase IDs with valid ensemble indices. A mask can exclude cells that must not participate.

**Evidence:** All released DREAM3D-NX sources from v7.0.0 through v7.4.2 contain the unchecked lower bound. DREAM.3D v6.5.171 has the same guard. `src/Plugins/OrientationAnalysis/test/ComputeIPFColorsTest.cpp::OrientationAnalysis::ComputeIPFColorsFilter: negative Phase bounds` checks `-48001` without a mask and success when a boolean mask excludes the negative phase.
