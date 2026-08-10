# Deviations from DREAM3D 6.5.171: GroupMicroTextureRegionsFilter

This file lists every behavioral difference between the released SIMPLNX filter and its DREAM3D legacy equivalent (`Source/Plugins/Reconstruction/ReconstructionFilters/GroupMicroTextureRegions.{h,cpp}`, which inherits from the `GroupFeatures` base). It describes the **current state of the shipped code**, not the development history of either implementation.

Entries are referenced by stable ID (`GroupMicroTextureRegionsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Which legacy build is the reference.** In DREAM3D 6.5.171 this filter is *unregistered* — it is listed in `_PrivateFilters`, so `ADD_SIMPL_FILTER(... FALSE)` emits no `fm->addFilterFactory()` call and the filter cannot be instantiated from the GUI or from a pipeline file. The same is true of every other tagged 6.x release. The filter **is** public and runnable in **DREAM3D 6.6.379**, which is therefore the legacy build used for all behavioral comparisons recorded here. The algorithm in 6.6.379 is unchanged from 6.5.171 apart from the EbsdLib math-API migration, so these entries describe the 6.5.171 algorithm as well; they simply could not have been observed there. See the *Availability across DREAM3D versions* section of the V&V report.

---

## GroupMicroTextureRegionsFilter-D1

| Field            | Value                                             |
|------------------|---------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D1`               |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`            |
| **Status**       | closed — no deviation                             |

Reserved ID, retained so cross-references do not dangle. The released SIMPLNX filter matches legacy behaviour for both settings of `UseNonContiguousNeighbors`: the non-contiguous neighbor list is required only when the user opts into it, and the *use* of that list is gated the same way legacy `GroupFeatures::execute()` gates it. No migration impact.

---

## GroupMicroTextureRegionsFilter-D2

| Field            | Value                                                        |
|------------------|--------------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D2`                          |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                       |
| **Status**       | active — intentional design difference                       |

**Deviation:** Parent-ID *labelling* differs. Legacy always randomizes parent IDs, using a clock-derived seed, and offers the user no control over it; consecutive legacy runs on identical input therefore produce different parent-ID values. SIMPLNX exposes a `RandomizeParentIds` parameter that **defaults to `false`**, so by default parent IDs are assigned sequentially in BFS-discovery order and are reproducible run to run.

**What is and is not affected:** the *grouping* is identical — the partition of features into regions, the number of regions, and which features share a region all match. Only the integer label attached to each region differs. Output is permutation-equivalent, never bit-identical, unless randomization is disabled on the SIMPLNX side and the comparison is made on the partition rather than on raw values.

**SIMPLNX options:**

- `RandomizeParentIds=false` (default) → reproducible sequential parent IDs. Suitable for diff testing and exemplar comparison.
- `RandomizeParentIds=true, UseSeed=false` → legacy-like behaviour: shuffled IDs from a nondeterministic seed.
- `RandomizeParentIds=true, UseSeed=true, SeedValue=<n>` → shuffled IDs that are still reproducible.

**Why it matters downstream:** parent IDs are frequently fed straight into a color map. Sequential IDs mean adjacent regions get adjacent colors, which can make neighbouring regions hard to distinguish; that is precisely what legacy's unconditional shuffle was for. Users who relied on that visual behaviour should set `RandomizeParentIds=true`.

**A secondary, unavoidable consequence:** legacy constructs a fresh RNG inside `getSeed()` on every call (`SIMPL_RANDOMNG_NEW()`), whereas SIMPLNX uses a single class-level generator seeded once. The order in which unparented features are selected as region seeds therefore differs between the two implementations. This does not change the resulting partition — the grouping criterion is symmetric and the neighbor graph is fixed — but it does change which region is discovered first, and hence the sequential labels under `RandomizeParentIds=false`.

**Recommendation:** Trust SIMPLNX. Compare partitions (equivalence classes), not parent-ID values, when validating against a legacy run.

---

## GroupMicroTextureRegionsFilter-D3

| Field            | Value                                                                          |
|------------------|--------------------------------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D3`                                            |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                                         |
| **Status**       | active — legacy bug, present in all legacy releases including DREAM3D 6.6.379   |

**Deviation:** With `UseRunningAverage=true`, legacy validates the crystal structure of only **one** of the two features under consideration; SIMPLNX validates **both**. Consequently, on data containing more than one Laue class, legacy can place a non-hexagonal feature and a hexagonal feature in the same microtexture region. SIMPLNX cannot.

**Mechanism.** The acceptance test in `determineGrouping` is:

```cpp
if(phase1 == phase2 && (phase1 == EbsdLib::CrystalStructure::Hexagonal_High))
```

In legacy, `phase1` (the reference feature's crystal structure) is assigned only inside the `if(!m_UseRunningAverage)` branch, so with the running average enabled it retains its initial value of `0`. Because `Hexagonal_High` is itself `0`:

```cpp
// EbsdLib EbsdConstants.h (legacy) / EbsdLibConstants.h (modern)
const unsigned int Hexagonal_High = 0;  //!< Hexagonal-High 6/mmm
const unsigned int Cubic_High     = 1;
```

the test does not fail — it degenerates from *"are both features hexagonal-high?"* to *"is the candidate feature hexagonal-high?"*:

```
phase1 == phase2 && phase1 == Hexagonal_High
  ->  0 == phase2 && 0 == 0
  ->  phase2 == Hexagonal_High
```

That is a strictly **weaker** condition than the intended one, so the legacy defect can only ever *add* grouping; it can never prevent grouping that should occur. SIMPLNX assigns both `phase1` and `phase2` unconditionally, restoring the intended two-sided check.

**Observable scope.** None on single-phase hexagonal data — the ordinary MTR case — where the two conditions are equivalent and legacy and SIMPLNX agree exactly. The deviation appears only when a feature whose Laue class is *not* Hexagonal_High touches one that is. Two distinct *phases* that both resolve to Hexagonal_High (e.g. primary alpha and transformed beta) group together in **both** implementations; that is intended behaviour and was confirmed in external review.

**Measured on DREAM3D 6.6.379** (`CAxisTolerance = 10°`, hand-built legacy fixtures):

| Fixture | legacy `UseRunningAverage=false` | legacy `UseRunningAverage=true` | SIMPLNX (either) |
|---|---|---|---|
| 5 hexagonal features, pure-Bunge Φ = 0/5/60/63/25°, chain neighbours F1–F2–F3–F4, F5 isolated | 3 groups: {F1,F2} {F3,F4} {F5} | 3 groups: {F1,F2} {F3,F4} {F5} | 3 groups: {F1,F2} {F3,F4} {F5} |
| Same, but the hexagonal phase is ensemble index 2 and a Cubic_High phase occupies index 1 | 3 groups (unchanged) | 3 groups (unchanged) | 3 groups (unchanged) |
| 20 independent touching pairs, each one Cubic_High feature + one Hexagonal_High feature, c-axes exactly aligned | 0 / 20 pairs merged | **19 / 20 pairs merged** | 0 / 20 pairs merged |

The first two rows establish that grouping is unaffected on hexagonal data regardless of which ensemble index the hexagonal phase occupies. The third isolates the deviation. (19 rather than roughly half of 20 because `getSeed()` scans forward from a random start index, so the lower-indexed cubic member of each pair is usually reached first and becomes the region seed.)

**Affected users:** Anyone running legacy `UseRunningAverage=true` on a scan that contains a non-hexagonal indexed phase adjacent to the hexagonal phase of interest. Users whose scans contain a single hexagonal phase — or several phases all of which are hexagonal — are unaffected and their historical results are correct.

**Recommendation:** Trust SIMPLNX. Migrating an existing legacy MTR pipeline should produce the same regions unless the scan contains a genuinely non-hexagonal phase touching the hexagonal one, in which case SIMPLNX will correctly decline merges that legacy made.
