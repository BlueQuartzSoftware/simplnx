# V&V Report: ConvertOrientationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| SIMPLNX Human Name | Convert Orientation Representation |
| DREAM.3D v6.5.171 equivalent | `ConvertOrientations` (SIMPL UUID `e5629880-98c4-5656-82b8-c9fe2b9744de`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-30; second engineer: Jared Duffey, 2026-07-14 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `ConvertOrientations` in DREAM.3D v6.5.171. SIMPLNX adds Stereographic, accepts only float32 arrays, and converts Euler inputs without the legacy normalization. |
| Oracle (confirmed) | Class 1 checks the stereographic formula; Class 2 reference values check dispatch and striding in `test/ConvertOrientationsTest.cpp`. The reference values come from EbsdLib and do not independently check its transform equations. |
| Code paths enumerated | 15 of 27 exercised. Gaps include guards, cancellation, progress messages, empty input, and I/O failures. |
| Tests today | 6 test cases: 5 ordinary cases cover conversions and validation, and 1 hidden case checks a 4,096-tuple boundary. The ordinary cases pass in CI. |
| Exemplar archive | None. Expected values and synthetic inputs are in `test/ConvertOrientationsTest.cpp`. |
| Legacy comparison | Not run for the current algorithm; the saved DREAM.3D v6.5.171 comparison is dated 2026-09-17. The algorithm changed after that run. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`ConvertOrientationsFilter` converts float32 orientation arrays between eight representations. Three distinct orientations check all 56 cross-type conversions and component strides against EbsdLib reference values; a separate closed-form calculation checks Quaternion-to-Stereographic output. All pass in CI.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; eight EbsdLib member-function dispatch groups replace seven legacy converter classes, with differences documented in D2 through D5.

## Oracle

*Class:* 1 (Analytical), with 2 (Reference implementation) for dispatch and striding.

*Applied:* Calculate Stereographic as `(qx, qy, qz)/(1 + qw)`; compare each conversion pair with three orientations expressed in all eight representations in `k_Ref`.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ConvertOrientationsTest.cpp::OrientationAnalysis::ConvertOrientations: Stereographic closed form (Class 1)` checks three quaternion fixtures at tolerance `1e-5`; `Dispatch and striding (8x8 matrix)` checks the same three orientations across 56 pairs at `1e-4`. All pass in CI.

*Second-engineer review:* Jared Duffey, 2026-07-14.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.1 contains the fix for `ConvertOrientationsFilter-D5`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ConvertOrientationsFilter-D5` | Euler normalization can change the represented rotation, including at the float32 `Phi = pi` boundary. DREAM.3D also overwrites the stored input. | DREAM3D-NX v7.0.0 through v7.4.0 applied the normalization to a local copy. DREAM.3D v6.5.171 was affected. | Convert the supplied Euler tuple without the modulo and sign-flip normalization; preserve the stored input. |

## Code path coverage

15 of 27 paths exercised. Each uncovered path has a row below.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ConvertOrientations.cpp` (436 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ConvertOrientationsFilter.cpp` (178 lines).

Test names below omit the `OrientationAnalysis::ConvertOrientations: ` prefix. Each output representation has seven tested input representations.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Equal input and output types return `-67005` | `Equal Representations` — all eight types |
| 2 | Preflight | Multidimensional component shape returns `-67003` | *Not directly tested. No fixture supplies a multidimensional component shape.* |
| 3 | Preflight | Wrong component count returns `-67004` | `Invalid preflight` — three components declared as Quaternion |
| 4 | Preflight | Out-of-range input or output index fails parameter validation | `Invalid preflight` — index 8 for each parameter |
| 5 | Preflight | Valid input creates an output with the required tuple and component counts | `Dispatch and striding (8x8 matrix)` |
| 6 | Dispatch | Convert seven input types to Euler | `Dispatch and striding (8x8 matrix)` — `-> Euler` sections |
| 7 | Dispatch | Convert seven input types to Orientation Matrix | `Dispatch and striding (8x8 matrix)` — `-> OrientationMatrix` sections |
| 8 | Dispatch | Convert seven input types to Quaternion | `Dispatch and striding (8x8 matrix)` — `-> Quaternion` sections |
| 9 | Dispatch | Convert seven input types to Axis-Angle | `Dispatch and striding (8x8 matrix)` — `-> AxisAngle` sections |
| 10 | Dispatch | Convert seven input types to Rodrigues | `Dispatch and striding (8x8 matrix)` — `-> Rodrigues` sections |
| 11 | Dispatch | Convert seven input types to Homochoric | `Dispatch and striding (8x8 matrix)` — `-> Homochoric` sections |
| 12 | Dispatch | Convert seven input types to Cubochoric | `Dispatch and striding (8x8 matrix)` — `-> Cubochoric` sections |
| 13 | Dispatch | Convert seven input types to Stereographic | `Dispatch and striding (8x8 matrix)`; `Stereographic closed form (Class 1)` |
| 14 | Dispatch | Same-type conversion arms | *Not directly tested. Preflight rejects equal types.* |
| 15 | Dispatch | Unknown input type exits the switch | *Not directly tested. Parameter validation rejects this index.* |
| 16 | Conversion | Read, convert, and write distinct tuples with different component strides | `Dispatch and striding (8x8 matrix)` — three distinct tuples per pair |
| 17 | Cancellation | Stop before a chunk when cancellation is requested | *Not directly tested. No fixture injects cancellation.* |
| 18 | Progress | Send the initial message and chunk progress | *Not directly tested. No test asserts messages.* |
| 19 | Conversion | Convert distinct directions at tuples 4,095 and 4,096 | `real HDF5 4096-block stereographic oracle` — expects `[0,0,1/3]` and `[1/3,0,0]` |
| 20 | Preflight | Missing input array fails parameter validation | `Invalid preflight` |
| 21 | Preflight | Non-float32 input fails parameter validation | *Not directly tested. All input arrays in this file use float32.* |
| 22 | I/O | Store an input read error and return it | *Not directly tested. No fixture injects a read failure.* |
| 23 | I/O | Store an output write error and return it | *Not directly tested. No fixture injects a write failure.* |
| 24 | Conversion | Stop a worker after another worker reports an error | *Not directly tested. No fixture injects a concurrent worker failure.* |
| 25 | Conversion | Empty input skips the tuple loop | *Not directly tested. Every execution fixture has tuples.* |
| 26 | Dispatch | Unknown output type reaches the final return | *Not directly tested. Parameter validation rejects this index.* |
| 27 | Conversion | Convert negative Euler angles or float32 `Phi = pi` without normalization | *Not directly tested. The current Euler fixtures use interior angles; D5 has separate saved comparison evidence.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ConvertOrientations: Invalid preflight` | kept | Checks a missing input array, input/output index 8, and a Quaternion input with only three components. |
| `OrientationAnalysis::ConvertOrientations: Dispatch and striding (8x8 matrix)` | new-for-V&V | Checks 56 cross-type pairs on three distinct tuples, output shapes, and all output components against `k_Ref` at `1e-4`. Replaces `Valid filter execution`. |
| `OrientationAnalysis::ConvertOrientations: Stereographic closed form (Class 1)` | new-for-V&V | Checks nine output components against `(qx,qy,qz)/(1+qw)` at `1e-5`, without an EbsdLib call for expected values. |
| `OrientationAnalysis::ConvertOrientations: Equal Representations` | kept | Checks error `-67005` for each of the eight equal input/output types. |
| `OrientationAnalysis::ConvertOrientations: real HDF5 4096-block stereographic oracle` | new-for-V&V | Hidden. Constructs 4,097 tuples and checks the two boundary outputs, an identity component, store format, and tuple dimensions. |
| `OrientationAnalysis::ConvertOrientationsFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check the filter UUID, comments, and all four converted arguments. |

## Exemplar archive

- **Archive:** None. Fixtures are constructed in `src/Plugins/OrientationAnalysis/test/ConvertOrientationsTest.cpp`.
- **SHA512:** Not applicable. `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` registers no archive for this filter.
- **Provenance:** Inline `k_Ref` comments identify EbsdLib 3.0.0 as the source of the dispatch reference values; no archive sidecar applies.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ConvertOrientationsFilter.md` for the root cause, affected users, and recommendation for each Part 1 entry.

| Deviation | Observed difference |
|-----------|---------------------|
| `ConvertOrientationsFilter-D1` | Retired: the claimed numerical difference used a local proof version, not DREAM.3D v6.5.171. |
| `ConvertOrientationsFilter-D2` | SIMPLNX accepts only float32 orientation arrays; DREAM.3D also accepts float64. |
| `ConvertOrientationsFilter-D3` | SIMPLNX adds Stereographic input and output. |
| `ConvertOrientationsFilter-D4` | Invalid inputs return different error codes. |
| `ConvertOrientationsFilter-D5` | SIMPLNX preserves Euler input and omits the legacy normalization that can change the rotation. |
