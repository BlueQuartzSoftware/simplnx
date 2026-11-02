# V&V Report: ComputeIPFColorsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6` |
| SIMPLNX Human Name | Compute IPF Colors |
| DREAM.3D v6.5.171 equivalent | `GenerateIPFColors` (OrientationAnalysis plugin, SIMPL UUID `a50e6532-8075-5de5-ab63-945feb0de7f7`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-09; second engineer: Matthew Marine, 2026-07-13 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of DREAM.3D v6.5.171 `GenerateIPFColors`. SIMPLNX adds two color keys, uint8 masks, negative-phase validation and storage-based execution. |
| Oracle (confirmed) | Classes 1 and 4 check analytical colors and invariants; Class 2 checks routing through EbsdLib on a six-cell fixture. All pass in CI. |
| Code paths enumerated | 19 of 34 exercised. Gaps include cancellation, I/O failures, boundary output checks and concurrent error counting. |
| Tests today | 10 test cases: 9 regular cases and 1 hidden HDF5 boundary case. The regular cases check colors, masks, normalization, input errors and SIMPL conversion. |
| Exemplar archive | `so3_cubic_high_ipf_001.tar.gz` supplies the legacy comparison input. This filter's tests use inline fixtures instead of its stored colors. |
| Legacy comparison | Not run for the current algorithm; the last comparison was on 2026-09-17, before the algorithm changed. |
| Bug flags | None |
| V&V phase | Analytical checks, routing checks and error regressions are encoded. All pass in CI. |

## Summary

`ComputeIPFColorsFilter` assigns RGB inverse-pole-figure colors from each cell's Euler angles and phase, with an optional mask and three color keys. Analytical expectations, invariants and direct EbsdLib calls check the filter's routing and output. The V&V records two fixed defects: concurrent error counting and negative phase indices.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; the per-cell mask, crystal-structure and RGB operations follow `GenerateIPFColors`, with EbsdLib replacing OrientationLib.

## Oracle

*Class:* 1 (Analytical), 2 (Reference — EbsdLib routing) and 4 (Invariant).

*Applied:* A six-cell cubic/hexagonal fixture checks black masked and invalid-structure cells, the cubic red corner, exact EbsdLib RGB values, output shape and direction normalization.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeIPFColorsTest.cpp::OrientationAnalysis::ComputeIPFColorsFilter: Class 1/2/3 Oracle (inline analytical dataset)` — one six-cell fixture with four sections, plus mask, normalization and error variants. All pass in CI.

*Second-engineer review:* The recorded technical-authority sign-off is Michael Jackson, 2026-07-16.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fix for `ComputeIPFColorsFilter-D2`. The fix for `ComputeIPFColorsFilter-D3` will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeIPFColorsFilter-D2` | Parallel workers incremented a shared non-atomic phase-warning counter, which caused a data race. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Direct execution uses an atomic counter; Scanline execution counts within its serial loop. |
| `ComputeIPFColorsFilter-D3` | An enabled negative phase index could read before the Crystal Structures array. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | Both executors return `-48001` for an enabled negative phase and ignore a masked negative phase. |

## Code path coverage

19 of 34 paths exercised. The uncovered rows identify gaps in the mapped assertions.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeIPFColors.cpp` (36 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeIPFColorsDirect.cpp` (276 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeIPFColorsDirect.hpp` (85 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeIPFColorsScanline.cpp` (189 lines), and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeIPFColorsFilter.cpp` (210 lines).

Test names below omit the common `OrientationAnalysis::ComputeIPFColorsFilter:` prefix. Shared per-cell rows describe the logical operation, not every executor and storage combination.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Dispatch | Boolean mask selects boolean mask access | `Class 1/2/3 Oracle (inline analytical dataset)` |
| 2 | Dispatch | UInt8 mask selects byte mask access | `uint8 mask array drives the black-out path` |
| 3 | Dispatch | Disabled mask colors valid cells without mask access | `no-mask path colors every valid cell` |
| 4 | Setup | Normalize the reference direction | `reference direction is normalized` |
| 5 | Direct | Stop the cell loop on cancellation | *Not directly tested. Requires cancel-signal injection.* |
| 6 | Per-cell | Initialize each RGB tuple to black | `Class 1/2/3 Oracle (inline analytical dataset)` |
| 7 | Per-cell | Read an enabled mask | `Class 1/2/3 Oracle (inline analytical dataset)`; `uint8 mask array drives the black-out path` |
| 8 | Per-cell | Count a phase at or above the ensemble limit | `phase index out of range returns -48000` |
| 9 | Per-cell | Valid cubic and hexagonal phases call EbsdLib and write RGB | `Class 1/2/3 Oracle (inline analytical dataset)` — four exact reference colors |
| 10 | Per-cell | A masked cell stays black | `Class 1/2/3 Oracle (inline analytical dataset)`; `uint8 mask array drives the black-out path` |
| 11 | Per-cell | An invalid crystal structure stays black | `Class 1/2/3 Oracle (inline analytical dataset)` |
| 12 | Per-cell | A phase above the ensemble limit stays black | *Not directly tested. The upper-bound test checks the error code, not the RGB tuple.* |
| 13 | Finalize | A nonzero phase-warning count returns `-48000` | `phase index out of range returns -48000` |
| 14 | Finalize | Valid inputs return success | `Class 1/2/3 Oracle (inline analytical dataset)` |
| 15 | Filter | Color Key 0 selects TSL | `Class 1/2/3 Oracle (inline analytical dataset)` |
| 16 | Filter | Invalid Color Key reaches fallback error `-23510` | *Not directly tested. ChoicesParameter rejects values outside 0 through 2 before execution.* |
| 17 | Preflight | Different Euler/phase tuple counts return `-651` | `Preflight Error - Cell array tuple count mismatch (-651)` |
| 18 | Preflight | Create a three-component uint8 array with the input tuple count | `Class 1/2/3 Oracle (inline analytical dataset)` |
| 19 | Scanline | Process a 65,536-cell block and a one-cell tail | *Not directly tested. The dedicated `real HDF5 65536-block color oracle` case is hidden from the default CI selection.* |
| 20 | Validation | An enabled negative phase returns `-48001` | `negative Phase bounds` / `Enabled negative Phase returns an error` |
| 21 | Validation | A masked negative phase does not cause an error | `negative Phase bounds` / `Masked negative Phase is ignored` |
| 22 | Filter | Color Key 1 selects PUCM | `ColorKey choice reaches algorithm` — output differs from TSL |
| 23 | Filter | Color Key 2 selects Nolze-Hielscher | `ColorKey choice reaches algorithm` — output differs from TSL |
| 24 | Scanline | Stop before a block on cancellation | *Not directly tested. Requires cancel-signal injection.* |
| 25 | Scanline I/O | Propagate a Crystal Structures read failure | *Not directly tested. Requires an injected store read failure.* |
| 26 | Scanline I/O | Propagate an Euler Angles read failure | *Not directly tested. Requires an injected store read failure.* |
| 27 | Scanline I/O | Propagate a Phases read failure | *Not directly tested. Requires an injected store read failure.* |
| 28 | Scanline I/O | Propagate a boolean mask read failure | *Not directly tested. Requires an injected store read failure.* |
| 29 | Scanline I/O | Propagate a uint8 mask read failure | *Not directly tested. Requires an injected store read failure.* |
| 30 | Scanline I/O | Propagate an RGB write failure | *Not directly tested. Requires an injected store write failure.* |
| 31 | Loop | Zero cells skip the cell or block loop | *Not directly tested. No empty fixture exists.* |
| 32 | Per-cell | An upper-bound phase still causes `-48000` when masked | *Not directly tested. No fixture combines these conditions.* |
| 33 | Preflight | An enabled mask with a different tuple count returns `-651` | *Not directly tested. The tuple-count fixture disables the mask.* |
| 34 | Direct | Multiple workers increment the atomic phase-warning count | *Not directly tested. The error fixture contains only one invalid phase and does not check the count.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeIPFColorsFilter: real HDF5 65536-block color oracle` | new-for-V&V | Hidden. Uses 65,537 cells with boolean and uint8 masks; checks black, cubic red and hexagonal green at indices 0, 65,535 and 65,536, plus storage and dispatch assertions. |
| `OrientationAnalysis::ComputeIPFColorsFilter: Class 1/2/3 Oracle (inline analytical dataset)` | new-for-V&V | Four sections check the cubic red corner, four EbsdLib reference colors, masked black and invalid-structure black; also checks output shape. |
| `OrientationAnalysis::ComputeIPFColorsFilter: uint8 mask array drives the black-out path` | new-for-V&V | Checks a black masked cell and a non-black enabled cell with a byte mask. |
| `OrientationAnalysis::ComputeIPFColorsFilter: no-mask path colors every valid cell` | new-for-V&V | Checks an exact EbsdLib color for the formerly masked cell and black for the invalid-structure cell. |
| `OrientationAnalysis::ComputeIPFColorsFilter: reference direction is normalized` | new-for-V&V | Compares all RGB bytes for reference directions [0,0,1] and [0,0,5]. |
| `OrientationAnalysis::ComputeIPFColorsFilter: phase index out of range returns -48000` | new-for-V&V | Sets phase 5 with three ensemble entries; checks execution error `-48000`. |
| `OrientationAnalysis::ComputeIPFColorsFilter: negative Phase bounds` | kept | Checks `-48001` for an enabled negative phase and success for a masked negative phase; covers D3. |
| `OrientationAnalysis::ComputeIPFColorsFilter: ColorKey choice reaches algorithm` | kept | Uses the inline fixture; checks that PUCM and Nolze-Hielscher each differ from TSL, without comparing them with each other. |
| `OrientationAnalysis::ComputeIPFColorsFilter: Preflight Error - Cell array tuple count mismatch (-651)` | kept | Uses 10 Euler tuples and 9 phase tuples; checks preflight error `-651`. |
| `OrientationAnalysis::ComputeIPFColorsFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check the converted UUID, comments and arguments. |

## Exemplar archive

- **Archive:** `so3_cubic_high_ipf_001.tar.gz` — retired as this filter's test oracle; used for the legacy comparison. Current tests construct their inputs in C++.
- **SHA512:** `dfe4598cd4406e8b83f244302dc4fe0d4367527835c5ddd6567fe8d8ab3484d5b10ba24a8bb31db269256ec0b5272daa4340eedb5a8b397755541b32dd616b85`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeIPFColorsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeIPFColorsFilter.md` for the root cause, affected users and recommendation. Only Part 1 entries appear here.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeIPFColorsFilter-D1` | Released DREAM3D-NX TSL colors can differ from DREAM.3D v6.5.171 by one level in an 8-bit channel. |
