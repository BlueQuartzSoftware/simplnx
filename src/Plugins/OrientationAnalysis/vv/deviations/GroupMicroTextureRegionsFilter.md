# Deviations from DREAM.3D 6.5.171: GroupMicroTextureRegionsFilter

This file lists every behavioral difference between the released SIMPLNX filter and its DREAM3D legacy equivalent (`Source/Plugins/Reconstruction/ReconstructionFilters/GroupMicroTextureRegions.{h,cpp}`, which inherits from the `GroupFeatures` base). It describes the **current state of the shipped code**, not the development history of either implementation.

Entries are referenced by stable ID (`GroupMicroTextureRegionsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Legacy comparison target.** DREAM.3D 6.5.171 contains this filter in `_PrivateFilters`. The release compiles the source but does not register the filter for GUI or pipeline use. Therefore, a direct pipeline comparison is not possible. The empirical A/B comparison used DREAM.3D 6.6.382, built from commit `107b8d51b`, because that version registers the filter. Both implementations ran only Group MicroTexture Regions on byte-identical input. The independent Class 1 and Class 4 oracle establishes correctness. The DREAM.3D 6.6.382 comparison provides deviation and migration evidence.

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
- `RandomizeParentIds=true, UseSeed=false` → legacy-like behaviour: shuffled IDs from a nondeterministic seed. Note `UseSeed` now defaults to `true`, so this combination must be selected deliberately.
- `RandomizeParentIds=true, UseSeed=true, SeedValue=<n>` → shuffled IDs that are still reproducible.

**Why it matters downstream:** parent IDs are frequently fed straight into a color map. Sequential IDs mean adjacent regions get adjacent colors, which can make neighbouring regions hard to distinguish; that is precisely what legacy's unconditional shuffle was for. Users who relied on that visual behaviour should set `RandomizeParentIds=true`.

**A secondary consequence:** legacy constructs a fresh RNG inside `getSeed()` on every call (`SIMPL_RANDOMNG_NEW()`), whereas SIMPLNX uses one generator seeded once. The seed order does not change the partition when `UseRunningAverage=false`. It can change the partition when `UseRunningAverage=true` because each accepted feature changes the comparison target. See D4 and D6.

**Recommendation:** Trust SIMPLNX. Compare partitions (equivalence classes), not parent-ID values, when validating against a legacy run.

---

## GroupMicroTextureRegionsFilter-D3

| Field            | Value                                                                          |
|------------------|--------------------------------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D3`                                            |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                                         |
| **Status**       | active — bug present in DREAM.3D 6.6.382                                      |

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

**Analytical discriminator.** The unit test contains 20 isolated touching pairs. Each pair contains one Cubic_High Feature and one Hexagonal_High Feature with aligned c-axes. The correct two-sided check rejects every pair. A temporary restoration of the one-sided condition caused 14 pair assertions to fail.

**Confirmed on real two-phase data with DREAM.3D 6.6.382 (`107b8d51b`).** The result was reproduced at scale
on an alpha/beta titanium scan (`915C_50red`, Oxford `.h5oina`, 2,326,310
cells; Phase 1 `Ti-Hex` → `Hexagonal_High`, Phase 2 `Titanium cubic` → `Cubic_High`). Both
implementations ran on a byte-identical shared input of **1,840,838 features** containing **106,731
hexagonal/cubic touching pairs**, at `CAxisTolerance = 20°`:

- Legacy merged **907 hexagonal/cubic pairs** that SIMPLNX refused. SIMPLNX merged none that legacy
  refused.
- The effect is **structural, not stochastic**: varying only the RNG seed produces exactly **zero**
  cross-Laue disagreements, so these 907 merges cannot be attributed to region-growth order.
- Applying the one-line `phase1` correction to a local build of the legacy source removed **all 907**
  merges, leaving zero cross-Laue disagreements in every configuration tested.

This is the first observation of D3 on production data, and it accounts for the 884-group difference
between the two as-shipped runs. With `UseRunningAverage=false` the same input yields **identical
partitions** in both implementations (746,877 groups, zero differing pairs), confirming the defect is
confined to the running-average path. The archived comparison evidence contains the full method and controls.

**Affected users:** Anyone running legacy `UseRunningAverage=true` on a scan that contains a non-hexagonal indexed phase adjacent to the hexagonal phase of interest. Users whose scans contain a single hexagonal phase — or several phases all of which are hexagonal — are unaffected and their historical results are correct.

**Recommendation:** Trust SIMPLNX. Migrating an existing legacy MTR pipeline should produce the same regions unless the scan contains a genuinely non-hexagonal phase touching the hexagonal one, in which case SIMPLNX will correctly decline merges that legacy made.

---

## GroupMicroTextureRegionsFilter-D4

| Field            | Value                                                     |
|------------------|-----------------------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D4`                       |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`                    |
| **Status**       | active — intentional default difference                   |

**Deviation:** The two implementations ship different defaults for `UseRunningAverage`. Legacy defaults to `false` (`m_UseRunningAverage(false)`); SIMPLNX defaults to **`true`**, and has since the initial port. Because that flag selects what a candidate feature is compared against, a pipeline run on defaults does not produce the same regions in the two implementations.

**Why it matters.** The flag chooses the comparison target for each candidate feature:

- `false` — the candidate is compared against the feature it touches (the current BFS frontier). Grouping is the transitive closure of the pairwise tolerance test along neighbour chains, so a region can drift arbitrarily far from its seed orientation one step at a time. At a 20° tolerance, features at 0°, 15°, and 30° all merge.
- `true` — the candidate is compared against the region's running volume-weighted average c-axis, which anchors the region and bounds that drift. On the same three features, 0° and 15° merge and 30° does not.

Both behaviours are correct and intended; they answer different questions. External review (A. Pilchak, Pratt & Whitney, 2026-08-11) confirmed both should be offered as user-selectable options, that neighbour-to-neighbour is appropriate when enforcing a misorientation requirement between adjacent features, and that the running average is appropriate when the region's ensemble orientation feeds downstream structure–property modelling. The reviewer's own practice is to use the running average.

**Affected users:** Anyone migrating a legacy pipeline that left "Group C-Axes With Running Average" at its default. Legacy ran the neighbour-to-neighbour path; SIMPLNX will run the running-average path unless the flag is explicitly cleared. Expect fewer, tighter regions in SIMPLNX on the same data — the anchored comparison rejects chains the legacy default would have accepted.

**Reproducibility consequence.** Because the running-average comparison target is order-dependent, its result changes with the RNG seed: on the two-phase dataset above, varying only the seed moved 192,496 of 3,710,475 touching pairs. SIMPLNX therefore defaults `UseSeed` to `true` so the shipped configuration is deterministic. Legacy has no such parameter — its seed is always clock-derived — so legacy runs with the running average enabled are not reproducible run to run.

**Recommendation:** Set `UseRunningAverage` explicitly rather than relying on either default. To reproduce a legacy run exactly, set it to `false`. Note that this deviation is independent of D3: D3 concerns *which Laue classes* are validated when the running average is enabled, whereas D4 concerns *which comparison target* is selected by default.

---

## GroupMicroTextureRegionsFilter-D5

| Field            | Value                                      |
|------------------|--------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D5`        |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`     |
| **Status**       | active — intentional output removal        |

**Deviation:** DREAM.3D creates `NewGrainData/Active` and initializes every value to `true`. DREAM3D-NX does not create this array. The parent Attribute Matrix is retained and its tuple count records the number of microtexture regions.

**Reason for removal:** `GroupMicroTextureRegionsFilter` never reads an `Active` value. The SIMPLNX port created the array with `false` values and never changed them. No dedicated downstream consumer of this output was identified. The array therefore did not communicate usable state.

**Compatibility:** Existing DREAM3D-NX pipelines that contain the removed `active_array_name` argument still load. The pipeline reader reports a warning for the unknown argument. The SIMPL 6.4 and 6.5 conversion fixtures also load successfully because the obsolete legacy `ActiveArrayName` value is ignored.

**Affected users:** A workflow is affected only if it explicitly selects the generated `Active` array. Use the parent Attribute Matrix tuple count or the parent-ID arrays instead.

**Recommendation:** Trust DREAM3D-NX. Remove references to the retired `Active` array from migrated pipelines.

---

## GroupMicroTextureRegionsFilter-D6

| Field            | Value                                      |
|------------------|--------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D6`        |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`     |
| **Status**       | active — defect fixed in the verified branch |

**Deviation:** Released DREAM3D-NX versions derived the grouping seed from the system clock by default. With `UseRunningAverage=true`, two runs of the same pipeline could produce different region partitions.

**Evidence:** Two preserved runs differed on 194,658 of 3,710,475 touching pairs. A second fixed-input seed comparison differed on 192,496 pairs. All differences were hexagonal/hexagonal. No cross-Laue differences occurred.

**Affected users:** DREAM3D-NX v7.0.0 through v7.4.1 users who used the running-average comparison and did not provide a fixed seed.

**Resolution:** The verified branch enables `UseSeed` by default. The seed value is stored in a top-level array so the run can be repeated.

**Recommendation:** Trust the verified branch. Keep `UseSeed=true` for reproducible grouping.

---

## GroupMicroTextureRegionsFilter-D7

| Field            | Value                                      |
|------------------|--------------------------------------------|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D7`        |
| **Filter UUID**  | `3f695987-81b1-47c3-8cff-b49cfa219be0`     |
| **Status**       | active — precision difference              |

**Deviation:** The legacy source converts the tolerance from degrees with a double-precision pi constant. DREAM3D-NX uses the float32 pi constant that matches the float32 input and angular calculation. A value at the acceptance boundary can therefore fall on different sides of the comparison.

**Evidence:** At seed 5489 and a 20 degree tolerance, the implementations disagreed on two of 3,710,475 touching pairs. Changing only the local legacy tolerance conversion to the float32 pi constant removed both differences. Changing vector-normalization precision did not remove them. At seed 1234, no boundary value produced a difference.

**Affected users:** Only comparisons that contain a candidate at the float32 tolerance boundary. The measured effect was two touching pairs in the production fixture.

**Recommendation:** Either result is acceptable within float32 boundary precision. Use DREAM3D-NX output for current workflows.
