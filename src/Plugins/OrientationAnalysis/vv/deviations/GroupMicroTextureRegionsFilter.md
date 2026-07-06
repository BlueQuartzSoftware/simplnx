# Deviations from DREAM3D 6.5.171: GroupMicroTextureRegionsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`Source/Plugins/Reconstruction/ReconstructionFilters/GroupMicroTextureRegions.{h,cpp}`, which inherits from `GroupFeatures` base).

Entries are referenced by stable ID (`GroupMicroTextureRegionsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Root-cause confirmation used a surgically patched local build of the legacy source that mirrors the SIMPLNX design fixes onto the legacy codebase — a 2025-10-23 fix titled `BUG: GroupMicrotextureRegions bug fixes, expose as usable filter`. Contact the DREAM3D team for the legacy-parity patch.

---

## GroupMicroTextureRegionsFilter-D1

| Field           | Value                                                              |
|-----------------|--------------------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D1`                               |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                            |
| **Status**       | active (fix landed 2026-06-11 on `vv/group_microtexture_regions`) |

**Symptom:** Prior to 2026-06-11, running the filter with `UseNonContiguousNeighbors=false` (the documented and default mode) always returned error `-99345` with message `"There was an error getting the Non-contiguous neighborlist from the DataStructure"`. The filter was effectively unusable except via the non-default `UseNonContiguousNeighbors=true` path.

**Root cause:** Bug. In `GroupMicroTextureRegions::execute()` (algorithm class), the null-pointer guard on `nonContigNeighListPtr` was placed *outside* the conditional that populates the pointer:

```cpp
NeighborList<int32>* nonContigNeighListPtr = nullptr;
if(m_InputValues->UseNonContiguousNeighbors)
{
  nonContigNeighListPtr = m_DataStructure.getDataAs<NeighborList<int32>>(...);
}
if(nullptr == nonContigNeighListPtr)   // <-- always triggers when UseNonContiguousNeighbors=false
{
  return MakeErrorResult(-99345, "...");
}
```

Legacy 6.5.171 `GroupFeatures::execute()` gates the *use* of `nonContigNeighList` behind `if(m_UseNonContiguousNeighbors)`, not the existence check — so legacy handles the default-mode case correctly. The SIMPLNX port preserved the use-site guard but introduced an unconditional existence check.

**Fix:** Moved the null check inside the `if(m_InputValues->UseNonContiguousNeighbors)` block, so it only fires when the user opted into the non-contiguous-neighbor path and the data store does not have the requested array. Pinned by `OrientationAnalysis::GroupMicroTextureRegionsFilter: Regression — runs in default UseNonContiguousNeighbors=false mode`.

**Affected users:** Anyone who tried to run the filter before 2026-06-11 in its documented default mode. (Realistically: nobody, given the filter was shipped behind a `[.][UNIMPLEMENTED][!mayfail]` test and a preflight warning that it is "untested, unverified, unvalidated".)

**Recommendation:** Trust SIMPLNX post-fix. The pre-fix SIMPLNX output was an unconditional error; no real-world results were produced from the broken code path.

---

## GroupMicroTextureRegionsFilter-D2

| Field           | Value                                                              |
|-----------------|--------------------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D2`                               |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                            |
| **Status**       | active (fix landed 2026-06-11 on `vv/group_microtexture_regions`) |

**Symptom:** Prior to 2026-06-11, SIMPLNX never randomized parent IDs even though the seed-array machinery, the `UseSeed` parameter, and the `SeedValue` parameter were all present. Legacy 6.5.171 randomizes parent IDs by default with a clock-derived seed (irreproducible). Output parent IDs from SIMPLNX were therefore monotonically assigned in BFS-walk order (1, 2, 3, …); output from 6.5.171 was the same equivalence classes under a random permutation.

**Root cause:** Bug. In `GroupMicroTextureRegions::operator()` (algorithm class), the `RandomizeFeatureIds` call was a commented-out block annotated `// !!! COMMENT OUT FOR DEMONSTRATION !!!`. The seed-array output was still written, but the randomization step was a no-op. Additionally, the algorithm's RNG (`m_Generator`) was initialized with `std::mt19937::default_seed` rather than `m_InputValues->SeedValue`, so the user-supplied seed never reached the seed-selection loop in `getSeed()`.

**Fix:** Adopted the patched-legacy design (the 2025-10-23 fix applied to a local build of the legacy source): added a new user parameter `RandomizeParentIds` (default `false`, so default behaviour is reproducible parent-id assignment). Restored a `randomizeParentIds(totalPoints, totalParentIds)` helper that performs a Fisher-Yates shuffle using `m_Generator` already seeded by `operator()`. Fixed the `operator()` RNG initialization to use `m_InputValues->SeedValue`. `parametersVersion()` bumped from 1 to 2 to flag the new parameter for SIMPL-conversion JSON.

**Affected users:** Anyone migrating a pipeline from 6.5.171 that depended on randomized parent IDs (e.g., feeding the parent IDs straight into a color-mapped visualization where adjacent groups should not share the same color by accident). Post-fix:
- `RandomizeParentIds=false` (default) → reproducible parent IDs, suitable for diff testing and exemplar comparisons.
- `RandomizeParentIds=true, UseSeed=false` → 6.5.171-like behaviour (clock-derived seed, irreproducible).
- `RandomizeParentIds=true, UseSeed=true, SeedValue=<n>` → reproducible randomization for users who want both shuffled IDs *and* run-to-run reproducibility.

**Recommendation:** Trust SIMPLNX. The pre-fix SIMPLNX output was deterministic but unintentionally so; the post-fix output gives users explicit control over both randomization and seed.

---

## GroupMicroTextureRegionsFilter-D3

| Field           | Value                                                                 |
|-----------------|-----------------------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D3`                                  |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                               |
| **Status**       | active (legacy bug; corrected in original 2024-01-08 SIMPLNX port)   |

**Symptom:** On legacy DREAM3D 6.5.171 with `UseRunningAverage=true`, the filter produces *no* feature groupings — every feature receives a unique parent ID equal to its own iteration index. SIMPLNX with `UseRunningAverage=true` produces the expected grouping behaviour.

**Root cause:** Bug in 6.5.171 `determineGrouping`. The local `uint32_t phase1` is declared with default value 0 and only assigned inside the `if(!m_UseRunningAverage)` branch:

```cpp
uint32_t phase1 = 0, phase2 = 0;
...
if(!m_UseRunningAverage)
{
  ...
  phase1 = m_CrystalStructures[m_FeaturePhases[referenceFeature]];   // <-- only here
  ...
}
phase2 = m_CrystalStructures[m_FeaturePhases[neighborFeature]];
if(phase1 == phase2 && (phase1 == Ebsd::CrystalStructure::Hexagonal_High))   // <-- phase1=0 when UseRunningAverage=true
{
  ...
}
```

When `UseRunningAverage=true`, `phase1` stays at 0; the subsequent `phase1 == Hexagonal_High` check fails for every candidate; no grouping ever occurs. Bug introduced upstream on 2014-01-30 by J. Tucker (commit `7e49e52f362005e44ea9bf21b7a717277b2af04e` in the original DREAM3D repository) and never caught.

**Fix in SIMPLNX:** The 2024-01-08 initial port (`ca6d0aa`) corrected the bug by assigning `phase1` outside the conditional, before the Hex_High check. The same fix was *deliberately applied to a local build of the legacy source* (2025-10-23) — confirmed by inspecting the pre-fix legacy source, which still has the buggy `phase1` declaration. The legacy-side fix renames the variable to `phase1Xtal`, hoists the assignment out of the `if(!m_UseRunningAverage)` block, and ships with a developer comment that explicitly names the J. Tucker 2014-01-30 introduction as the bug source. The 6.5.171 release line was never patched.

**Affected users:** Anyone running 6.5.171 with `UseRunningAverage=true` saw degenerate output (one group per feature) without warning. Users migrating that workflow to SIMPLNX will see *real* groupings for the first time, and downstream filters that consumed the degenerate output may behave differently.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result with `UseRunningAverage=true` was mathematically incorrect; SIMPLNX produces the result the filter has always claimed to produce.
