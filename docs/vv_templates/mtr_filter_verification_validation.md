# Verifying a DREAM3D-NX Filter 

## Goal 

The goal of the code review is to verify and validate that the filter is producing data correctly and we have a firm understanding of the algorithm that is being used. 

Outputs should be compared against 6.5.172 branches on all Legacy DREAM3D Repositories
    - CMP
    - SIMPL
    - SIMPLView
    - DREAM3D

- Use the skill `compare-legacy-dream3d` to start this comparison

## Step 0: Establishing Correctness — The Oracle

> This step happens **before** any comparison against legacy DREAM3D 6.5.172.

### Why an independent oracle

DREAM3D 6.5.172 is trusted by reputation, not by proof. It has been in the field long enough that users rely on its numbers, but it was never subjected to the V&V rigor we are applying to DREAM3DNX. We therefore cannot treat "matches 6.5.172" as a correctness check — it is a **diff-explanation** check. Its purpose is user-facing accountability: when a user migrates from DREAM3D to DREAM3DNX and sees different numbers, we can name the reason — legacy algorithm bug, single-precision arithmetic, different order of operations, different neighbor definition, different library — rather than leave the user guessing or erode their trust in the new tool.

That means every filter must first be shown correct **independently** of 6.5.172, and only then diffed against legacy to produce the user-facing change narrative.

### Oracle classes

Every filter must be verified against at least one of the following, in descending order of strength:

1. **Analytical** — closed-form expected output computable on toy input. Examples: threshold, crop, rotate, color-space conversion, histogram, array arithmetic. Oracle = hand calculation or spreadsheet.
2. **Reference implementation** — a trusted external library produces the expected output on the same input. Examples: NumPy/SciPy for statistics and FFTs, MTEX/OrientationLib for crystallography, Eigen for matrix math.
3. **Paper-based** — the filter reproduces a published figure, table, or numerical example from a named reference. Examples: EBSD cleanup algorithms, grain growth models, synthetic microstructure builders.
4. **Invariant-based** — no closed-form output exists, but outputs must satisfy derivable properties. Examples: FeatureId arrays start at 1 and are contiguous; sum of phase fractions = 1; grown-grain IDs ⊇ seed-grain IDs; mass conservation; monotonicity under a parameter sweep.
5. **Expert-visual** — a domain expert inspects output on canonical test cases in DREAM3D-NX and signs off. The honest last resort for simulation-like filters with no closed-form or paper-based reference.

### Policy

- Every filter under verification must have at least one oracle of class **1–4**, **OR** an explicit, documented justification — recorded in the filter-verification archive ReadMe — for why only class 5 is feasible.
- "Legacy produced this output" is **never** a valid oracle for correctness.
- The oracle and its justification **really should** be reviewed by a second engineer before it is used to validate the filter. A wrong oracle will silently "confirm" a buggy filter, and the filter author is the least likely person to notice — test cases unconsciously get designed around the code that already exists. Skip this review only when no second engineer is realistically available, and when you do skip it, record the skip and the reason in the archive ReadMe.

### Step-ordered workflow

a. **Classify.** Name the filter's oracle class (1–5) and record it in the archive ReadMe.
b. **Design toy data.** Build the minimum-size synthetic inputs that exercise each code path *and* for which the chosen oracle can produce an expected output.
c. **Compute the expected output independently.** Do not run any DREAM3D version. Save the oracle computation — script, spreadsheet, paper reference with figure/table number, or annotated expert notes — in the verification archive so the next engineer can reproduce it.
d. **Run DREAM3DNX/SIMPLNX** on the toy data and compare against the oracle. Resolve any discrepancy in SIMPLNX before moving on.
e. **Run DREAM3D 6.5.172** on the same toy data and diff against the now-verified SIMPLNX output. Any remaining differences are the **diff-explanation** content: each one needs a named root cause (bug, precision, order of operations, library, etc.) captured in the comparison report.

Only after a–e are complete does it make sense to invest in readability, performance, or memory refactors — those require a correctness safety net to refactor against.

### Where each oracle lives

Different oracle classes produce different kinds of artifacts. The table below maps each class to its natural home. A one-page engineer-facing version of this mapping lives at `.claude/oracle_classes_quick_reference.md`.

| Class | Lives in | Form |
|---|---|---|
| 1 Analytical | Unit test code | `REQUIRE(result == X)` with a comment showing the hand derivation |
| 2 Reference implementation | Verification archive (script) + unit test (cached exemplar `.dream3d`) | Script generates the exemplar once; test compares SIMPLNX output against the cached exemplar |
| 3 Paper-based | Unit test code (numeric assertions) + archive (paper reference) | Expected numbers in `REQUIRE`s with citation comment; DOI and figure/table/equation number in archive ReadMe |
| 4 Invariant | Unit test code | `REQUIRE(feature_ids.min() == 1); REQUIRE(is_contiguous(feature_ids));` |
| 5 Expert-visual | Verification archive (screenshots + sign-off) + unit test (cached exemplar `.dream3d`) | Screenshots are the oracle record; test compares against a cached exemplar the expert approved |

**Summary:** classes 1, 3, and 4 produce inline test-code assertions. Classes 2 and 5 produce cached exemplar files. The "exemplar `.dream3d` file downloaded from the Data_Archive" mechanism already used by simplnx tests is exactly the right home for Class 2 and Class 5 exemplars — the oracle policy just upgrades the provenance story for how those files earn their status.

### Oracle provenance record (archive ReadMe)

Classes 2, 3, and 5 have non-trivial drift risk — library versions change, journal links rot, experts leave. For these, the filter-verification archive ReadMe must include an Oracle Provenance block so a future engineer can tell whether the oracle is still reproducible.

Required fields per class:

- **Class 2 (reference implementation):** library name + exact version + random seed (if any) + script filename. Example record:
  > *Oracle: MTEX 5.9.0 (released 2023-05-14) on Octave 8.2.0, seed = 42, script `orcl_mtex.m` in this archive.*

  Optional but recommended: a hash of the script's output, so a future engineer can re-run the script and detect drift by hash comparison without having to read the numbers.

- **Class 3 (paper-based):** DOI + edition + figure/table/equation number + page number. Embed a copy of the paper (or the specific figure) in the archive — journal paywalls and URL rot are real. Example record:
  > *Oracle: Smith & Jones, "…", J. Materials Sci., vol. 34 no. 2, pp. 410–417, 1998. DOI 10.xxx/yyy. Equation 14 on p. 413. Figure 5b reproduced in `notes/smith_fig5b.png`.*

- **Class 5 (expert-visual):** named expert + date of approval + signed-off screenshots of the expected output. Example record:
  > *Oracle: M. Jackson, 2026-04-23. Approved outputs captured in `notes/expert_signoff/*.png`. Class-5-only justification: no closed-form or paper-based reference exists for this Monte-Carlo grain-growth filter.*

Classes 1 and 4 need no provenance block — the oracle lives in the test code directly and travels with the repository.

## Legacy Comparison — Diff Explanation

> After step (e) of the Oracle workflow produces a verified SIMPLNX output and a 6.5.172 output on identical toy data, this section governs how the resulting differences are written up. Use the `compare-legacy-dream3d` skill to execute the comparison; use this policy to structure the report.

### Purpose

We do not compare against 6.5.172 to establish correctness — that was Step 0's job. We compare so that when a user migrates from DREAM3D to DREAM3DNX and sees different numbers, we can hand them a definitive explanation rather than leave them guessing or erode their trust. The resulting Deviation entries are shipped to:

- The DREAM3D website (public migration / upgrade guide)
- MTR SBIR deliverables (per-filter evidence of V&V rigor)
- The verification archive (permanent record for future engineers)

Because these entries end up in external documents, they must be written to a consistent template from day one.

### Algorithm Relationship — opening line of every comparison report

Every comparison report opens with a single-line classification of how SIMPLNX relates to the legacy implementation. This sets reader expectations before any array diffs appear:

- **Port** — line-by-line translation of the legacy algorithm. Differences should be minor and confined to type precision, library calls, or parallelization strategy.
- **Minor changes** — same algorithm intent with small deliberate improvements (e.g., `float` → `double`, corrected boundary handling, Eigen instead of hand-rolled matrix math).
- **Rewrite** — the implementation is substantially different (new library, new data structures, new parallelization strategy), but the filter UUID was preserved because both implementations are intended to produce equivalent results for the same inputs. **A rewrite that produces materially different outputs is a red flag**: either the rewrite has a bug, or the filter should have been shipped as a new filter with a new UUID. Either way, the divergence requires a full Deviation entry and a design-level justification in the report. There is no "different algorithm, different output, case closed" shortcut — keeping the UUID is a claim of functional equivalence, and that claim has to be defended.
- **New filter, no legacy equivalent** — step (e) is N/A; skip legacy comparison and proceed to documentation.

Example opening lines:

> *Algorithm Relationship: **Port** — line-by-line translation from the SIMPL `FindEuclideanDistMap` filter, with scalar variables promoted from `float` to `double`.*

> *Algorithm Relationship: **Minor changes** — neighbor-iteration loop rewritten to use Eigen for the 3×3 symmetric matrix solve. No intended change in output.*

> *Algorithm Relationship: **Rewrite** — the SIMPL `ClusterDetection` filter used a hand-rolled k-means implementation; SIMPLNX uses DBSCAN. The UUID was preserved because both implementations are intended to produce equivalent cluster assignments on typical microstructure inputs. Boundary-point divergences are documented as Deviation 1 below.*

### Deviation Template

Every array-level difference uncovered in step (e) is written up using the following template. One entry per deviation:

> **Deviation ID:** `<FilterName>-D<N>` (e.g., `SegmentFeatures-D2`). N is a per-filter counter starting at 1. The human name portion must use the *current canonical SIMPLNX name*. The ID is stable and must be used everywhere this deviation is referenced — website migration guide, MTR SBIR deliverables, verification archive, release notes, user support threads.
> **Filter UUID:** `<SIMPLNX UUID>` — the permanent cross-reference anchor. Human names and class names can change over time; the UUID cannot. If a filter is ever renamed, the old Deviation ID retires and a new one is issued under the new name, but the UUID in this field links the two.
> **Symptom:** *<one-sentence user-visible symptom>*
> **Root cause:** *<bug / precision / order of operations / library / algorithmic choice>*
> **Affected users:** *<who actually sees this — e.g., "anyone with features spanning image boundaries," "only Hex-symmetry datasets," "datasets larger than 10M voxels">*
> **Recommendation:** *<trust SIMPLNX; trust 6.5.172; either is acceptable within tolerance X; see quick-patch link for legacy-parity option>*

Root-cause categories (use one, or a short compound like "precision + library"):
- **Bug** — one implementation is mathematically wrong.
- **Precision** — different floating-point width or intermediate-math type.
- **Order of operations** — associativity differences in parallel reductions, different loop order, different accumulation.
- **Library** — Eigen vs. hand-rolled, different trig/random implementations, different HDF5 versions.
- **Algorithmic choice** — deliberate change in method (used only for **Rewrite** relationships).

### Example Deviation entries

These are meant as starting points the engineer can adapt — keep the structure, swap in the filter-specific facts.

**Precision example:**

> **Deviation ID:** `ComputeEulerAngles-D1`
> **Filter UUID:** `aaaa1111-0000-0000-0000-000000000001` *(illustrative)*
> **Symptom:** Euler-angle output differs from 6.5.172 by up to 0.003° in orientations near grain boundaries.
> **Root cause:** Precision. SIMPLNX performs the internal orientation-matrix operations in `double`; 6.5.172 performed the same operations in `float`.
> **Affected users:** Workflows that compute orientation statistics on features larger than ~10⁴ voxels, where accumulated float32 round-off becomes visible at the 10⁻³ degree level. Users who only visualize IPF colors will not notice.
> **Recommendation:** Trust SIMPLNX. The 6.5.172 output was limited by float32 round-off and is not materially more correct for any downstream calculation.

**Legacy-bug example:**

> **Deviation ID:** `SegmentFeatures-D2`
> **Filter UUID:** `aaaa2222-0000-0000-0000-000000000002` *(illustrative)*
> **Symptom:** FeatureId count on a 50×50×50 block test pattern is 27 in SIMPLNX and 26 in 6.5.172.
> **Root cause:** Bug in 6.5.172. The outer segmentation loop used `< dimZ` where it should have used `<= dimZ`, silently dropping features that touched the +Z boundary. Corrected in SIMPLNX; a minimal quick patch is available on the internal 6.5.172 branch (`<patch PR link>`) for users who need to reproduce corrected legacy runs.
> **Affected users:** Anyone who ran `SegmentFeatures` on datasets where a feature touched the +Z volume boundary. The missing feature was always the one nearest +Z.
> **Recommendation:** Trust SIMPLNX. The 6.5.172 result was mathematically incorrect.

**Rewrite example:**

> **Deviation ID:** `ClusterDetection-D1`
> **Filter UUID:** `aaaa3333-0000-0000-0000-000000000003` *(illustrative)*
> **Symptom:** Cluster assignments on the IN100 benchmark dataset differ in 47 of 3,842,528 voxels (~0.001%).
> **Root cause:** Algorithmic choice. 6.5.172 used an in-house k-means implementation; SIMPLNX uses DBSCAN. Both are intended under the shared UUID to produce equivalent cluster assignments, but DBSCAN handles density-discontinuity boundary points differently from k-means — the 47 divergent voxels all lie on cluster boundaries where neither method is definitively correct.
> **Affected users:** Users reprocessing 6.5.172 clustering results and comparing voxel-by-voxel. Bulk cluster statistics (cluster count, centroids, volume fractions) are unchanged.
> **Recommendation:** Trust SIMPLNX for new work. Users reproducing a specific 6.5.172 figure should note that boundary-voxel cluster assignments may shift by ~0.001%; bulk statistics will agree.


### 1. Scope of "Verified"

There is some mix of 
- (a) bit-exact match
- (b) within floating-point tolerance, 
- (c) "documented, understood deviation is acceptable," or some mix depending on category?

### 2. Deviation Taxonomy 

When outputs differ, what are the legal outcomes? I'd expect four: 
(i) legacy bug →  patch legacy; 
(ii) NX bug → fix NX; 
(iii) NX intentionally improved (e.g., double precision, Eigen, better boundary handling) → document as expected deviation, no legacy patch;
(iv) both wrong → fix NX, decide on legacy. 

Is that your taxonomy, or something different?

### 3. "Surgical" constraint on the legacy branch. 

compare-legacy-dream3d already enforces "only patch legacy if its output is demonstrably wrong, never for style/quality"
    - Is that the right rule for MTR, or do you want a stricter one (e.g., patches must be minimal diffs, must be diff-reviewed before landing on the internal branch)?

### 4. What's the deliverable? 

A comparison report? A diff patch against the legacy branch? A unit test that pins the output? All three? The answer determines whether the doc needs an "Artifacts" section.

### 5. What parts of the broader V&V do you still want? 

Algorithm line-by-line review and documentation updates are independent of the comparison goal — they apply to any verified filter. Or do you want MTR to be purely "compare + explain + patch," leaving the other reviews as separate policies?

## Algorithm Review 

- For given inputs, expected outputs are generated 
- The algorithm is reviewed line-by-line to ensure the following: 
    - Code is commented to addresses any subtle or potentially easily misunderstood code 
    - Variable names are well defined and precisely describe the values that are being held 
    - Memory leaks are removed 
    - Memory use is reasonable. If a filter is going to create large temporary memory allocations, then this should be noted somewhere in the code with an appropriate comment and possibly mentioned in the documentation as a warning to the user. 
    - Progress Message optimizations are used. Don’t check every 1 second for progress. There are dedicated classes for that. 
    - The algorithm is checking for cancel at reasonable points of time 

## Unit Test Review

> Correctness is established in **Step 0: Establishing Correctness — The Oracle**. Legacy-comparison write-up is governed by **Legacy Comparison — Diff Explanation**. This section covers only the *test-code mechanics* that encode those results into the repository so they run on every build.

- Every code path through the algorithm must be exercised by at least one unit test derived from the Step 0 toy data. New synthetic datasets may need to be generated to exercise specific code paths.
- Unit test data sets should be small enough to easily debug but large enough to test the data combinations that the code logic is testing.
- Final exemplar data sets must be hand verified (or verified through an oracle of class 1–4 per Step 0) before being published to the data archive.
- DREAM3D files that contain data from both 6.5.171/172 and NX should have internal data sets labeled so engineers can distinguish which version created each data set. Use the "6_5_" prefix for legacy-generated data; NX-generated data is unprefixed.
- For each new test-data-archive that is created, a "ReadMe.md" file should be included that contains a short but descriptive narrative with:
    - How was the data generated: DREAM3D itself? Excel? Python script?
    - Who generated it and when (initials and date are fine)
    - References to papers that were used to generate data
    - What were the inputs to generate the exemplar data? (Small IN100? Other data file? CSV?)
    - DREAM3D-NX Pipeline(s) used to produce the exemplar data.

## Documentation Updates 

- The documentation for the filter should be updated if possible/needed 
- Before and After images should be used if those images will help to explain what the filter is doing 
    - Images should be well annotated. There are enough tools in NX to help do this properly. 
- References to papers that the algorithm is based on should also be used where possible or needed. 
- Inforgraphics should also be generated if that would make understanding the filter or algorithm easier.


## Archiving Everything When Finished 

- As a standard operating procedure, I am uploading all data files and pipelines (for both 6.5.171 and NX) up to OneDrive into a folder specific to that filter verification. This folder can contain even more data and notes that you may have used to verify the filter. 
- The idea is that another engineer should be able to pull that folder and use the data to recreate the exemplar data and tests if needed.

## Retroactive Application

This section governs the retroactive V&V of filters that have been informally reviewed — filters marked as "done" or shipped without producing the Step 0 / Legacy Comparison artifacts defined above. The first cut of the list is the filters cited in the MTR SBIR proposal (see table below). A broader pass across the rest of the simplnx filter catalog is a longer-term goal.

### Deliverable per filter

Two bars, pick explicitly per filter:

- **Minimum (required for all retroactive filters):** a filter-specific set of user-facing Deviation entries using the Deviation Template, ID'd as `<FilterName>-D<N>`. Published to the DREAM3D website migration guide and included in the MTR SBIR report.
- **Ideal (target for SBIR-cited filters):** the full Step 0 workflow (oracle classification, toy data, independent oracle computation, SIMPLNX verified against oracle, legacy comparison) plus an archive ReadMe with the Oracle Provenance block and published exemplar archives updated to the new styling.

Updating already-published data archives to the new Deviation styling is a heavy lift and should not be gated on every filter — target SBIR-cited filters first, extend opportunistically.

### Promote existing work product — don't restart

Before designing new oracles or toy data, check what already exists for the filter:

- Hand-verified exemplars in the Data_Archive
- Paper references in filter or algorithm header comments
- Prior comparison notes on OneDrive, in email, or in Slack
- Unit tests with informative assertions that encode implicit invariants

Any of the above can be retrofit-promoted into the new format. Examples:
- A paper reference in a header comment becomes a Class-3 oracle entry in the archive ReadMe (once DOI / figure / page are filled in).
- A hand-verified exemplar `.dream3d` file becomes a Class-5 oracle, with the original engineer named as the "expert" and the exemplar's commit date as the sign-off date — *provided the original engineer confirms* that this matches their memory of what they verified.
- An existing invariant-style assertion (`REQUIRE(feature_ids.min() == 1)`) counts as a Class-4 oracle once the invariant is documented.

Starting from scratch is the wrong default for retroactive work. The effort is to make existing evidence legible and citeable, not to redo every verification from zero.

### Scope and triage

**Tier 1 — SBIR-cited filters.** The list below. Full Step 0 + Legacy Comparison + published Deviation entries. Oracle classification done as a shared review (not solo) so class decisions are consistent across the group. Target deliverable: ideal, per above.

**Tier 2 — Broader simplnx catalog.** Longer-term. Recommended biting-off strategy:

- **Classify first, verify second.** Do oracle-class classification for every remaining filter before any verification work begins. The output is a spreadsheet: filter name, likely oracle class, rough effort estimate, dependencies on other filters. Classification is cheap; verification is not. This single artifact makes the rest of the planning tractable.
- **Batch by oracle class.** Within Tier 2, work Class 1 and Class 4 filters first — they're cheap, they build momentum, and they expand the set of filters with proper V&V quickly. Class 2 and Class 3 filters come second. Class 5 filters are either deferred or get a dedicated expert-review campaign; don't mix them into the general flow.
- **Batch by algorithm family.** Filters that share algorithm structure (all segmentation filters, all morphological filters, all Feature-level statistics filters, all orientation-math filters) batch well. One set of toy data and one oracle script often serves a whole family, which amortizes the most expensive parts of oracle design. Consider assigning each family to a single engineer rather than spreading a family across the team.
- **Target deliverable:** minimum per above (user-facing Deviation writeups). Promote to "ideal" only where the filter is in heavy user pipelines.

### Class-5-only filters

Some filters on either tier will not have any feasible class-1-to-4 oracle (Monte-Carlo simulations, filters whose output is only meaningful as an aggregate visual). These are identified during the shared classification step, not during verification. For each such filter, record in the archive ReadMe: the class-5-only justification (why no stronger oracle is feasible), the named expert, and the signed-off screenshots. A class-5-only filter does not block the rest of the pass.

### Engineer alignment

All engineers performing retroactive V&V must read this policy document before starting their first filter. This is the single biggest rework-prevention lever — inconsistent interpretations of "oracle," "deviation," or "Algorithm Relationship" across engineers will produce a set of reports that cannot be stitched together into a coherent MTR deliverable.

Concretely:

- Oracle classification for Tier 1 filters is a shared review, not solo.
- A running list of assigned Deviation IDs (`<FilterName>-D<N>`) is maintained by the team to prevent collisions when engineers work in parallel on the same filter.
- A single worked example is produced first (one Tier 1 filter taken end-to-end) and used as a reference for the rest of the team before additional filters are started.

 ## MTR SBIR Table of Filters Retroactive to Verify and Validate

Columns: SIMPLNX UUID is the primary key. Both ClassName columns will be populated as part of the PR walk (they come from the filter's source filename). Human Name columns carry the user-facing label — the SIMPLNX name is the canonical one used in Deviation IDs.

| SIMPLNX UUID | SIMPLNX ClassName | SIMPLNX Human Name | SIMPL UUID | SIMPL ClassName | SIMPL Human Name | PR # (since 2025-10-01) |
|---|---|---|---|---|---|---|
| 5b062816-79ac-47ce-93cb-e7966896bcbd | ReadAngDataFilter | Import EDAX EBSD Data (.ang) | b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40 |  |  | #1438, #1586 |
| 64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6 | ComputeIPFColorsFilter | Generate IPF Colors | a50e6532-8075-5de5-ab63-945feb0de7f7 |  |  | #1438 |
| 4246245e-1011-4add-8436-0af6bed19228 | MultiThresholdObjectsFilter | Threshold Objects (Advanced) | 686d5393-2b02-5c86-b887-dd81a8ae80f2 |  |  |    #1502, #1582 |
| 00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed | WritePoleFigureFilter | Export Pole Figure Images | a10bb78e-fcff-553d-97d6-830a43c85385 |  |  | #1438, #1491, #1566, #1587 |
| 501e54e6-a66f-4eeb-ae37-00e649c00d4b | ConvertOrientationsFilter | Convert Orientation Representation | e5629880-98c4-5656-82b8-c9fe2b9744de |  |  |  #1438, #1468 |
| a51c257a-ddc1-499a-9b21-f2d25a19d098 | ComputeCAxisLocationsFilter | Find C-Axis Locations | 68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e |  |  | #1438, #1582 |
| 3f342977-aea1-49e1-a9c2-f73760eba0d3 | BadDataNeighborOrientationCheckFilter | Neighbor Orientation Comparison (Bad Data) | f4a7c2df-e9b0-5da9-b745-a862666d6c99 |  |  | #1438, #1499 |
| 4625c192-7e46-4333-a294-67a2eb64cb37 | NeighborOrientationCorrelationFilter | Neighbor Orientation Correlation | 6427cd5e-0ad2-5a24-8847-29f8e0720f4f |  |  | #1438, #1513 |
| 94d47495-5a89-4c7f-a0ee-5ff20e6bd273 | IdentifySampleFilter | Isolate Largest Feature (Identify Sample) | 0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a |  |  | #1473, #1530, #1562 |
| 9fe07e17-aef1-4bf1-834c-d3a73dafc27d | CAxisSegmentFeaturesFilter | Segment Features (C-Axis Misalignment) | bff6be19-1219-5876-8838-1574ad29d965 |  |  | #1373, #1438, #1490 |
| da5bb20e-4a8e-49d9-9434-fbab7bc434fc | ComputeFeaturePhasesFilter | Find Feature Phases | 6334ce16-cea5-5643-83b5-9573805873fa |  |  |  #1455 |
| 086ddb9a-928f-46ab-bad6-b1498270d71e | ComputeAvgOrientationsFilter | Find Feature Average Orientations | bf7036d8-25bd-540e-b6de-3a5ab0e42c5f |  |  | #1438, #1458, #1577 |
| c6875ac7-8bdd-4f69-b6ce-82ac09bd3421 | ComputeFeatureCentroidsFilter | Find Feature Centroids | 6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca |  |  |  #1457 |
| 7177e88c-c3ab-4169-abe9-1fdaff20e598 | ComputeFeatureNeighborsFilter | Find Feature Neighbors | 97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac |  |  |  #1569 |
| a59eb864-9e6b-40bb-9292-e5281b0b4f3e | FillBadDataFilter | Fill Bad Data | 30ae0a1e-3d94-5dab-b279-c5727ab5d7ff |  |  |  #1515,#1534 |
| c666ee17-ca58-4969-80d0-819986c72485 | ComputeFeatureSizesFilter | Find Feature Sizes | 656f144c-a120-5c3b-bee5-06deab438588 |  |  |   #1540 |
| 453cdb58-7bbb-4576-ad5e-f75a1c54d348 | ComputeAvgCAxesFilter | Find Average C-Axis Orientations | c5a9a96c-7570-5279-b383-cc25ebae0046 |  |  | #1438, #1582 |
| 16c487d2-8f99-4fb5-a4df-d3f70a8e6b25 | ComputeFeatureReferenceCAxisMisorientationsFilter | Find Feature Reference C-Axis Misalignments | 16c487d2-8f99-4fb5-a4df-d3f70a8e6b25 |  |  | #1438, #1582 |
| 0b68fe25-b5ef-4805-ae32-20acb8d4e823 | ComputeFeatureNeighborMisorientationsFilter | Find Feature Neighbor Misorientations | 286dd493-4fea-54f4-b59e-459dd13bbe57 |  |  | #1438, #1576, #1582 |
| 636ee030-9f07-4f16-a4f3-592eff8ef1ee | ComputeFeatureNeighborCAxisMisalignmentsFilter | Find Feature Neighbor C-Axis Misalignments | cdd50b83-ea09-5499-b008-4b253cf4c246 |  |  | #1438, #1467, #1474, #1582 |
| a181ee3e-1678-4133-b9c5-a9dd7bfec62f | ITKImageWriterFilter | ITK::Image Writer | 11473711-f94d-5d96-b749-ec36a81ad338 |  |  |   #1489, #1490, #1555, #1576 |
| ff46afcf-de32-4f37-98bc-8f0fd4b3c122 | ComputeGroupingDensityFilter | Find Grouping Densities | 708be082-8b08-4db2-94be-52781ed4d53d |  |  | #1548 |

## MTR SBIR Table of Filters to complete

| SIMPLNX UUID | SIMPLNX ClassName | SIMPLNX Human Name | SIMPL UUID | SIMPL ClassName | SIMPL Human Name | PR # (since 2025-10-01) |
|---|---|---|---|---|---|---|
| 4ab5153f-6014-4e6d-bbd6-194068620389 | RequireMinNumNeighborsFilter | Minimum Number of Neighbors | dab5de3c-5f81-5bb5-8490-73521e1183ea |  |  |   |
| 7f2f7378-580e-4337-8c04-a29e7883db0b | ErodeDilateBadDataFilter | Erode/Dilate Bad Data | 3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254 |  |  |  |
| b3a95784-2ced-41ec-8d3d-0242ac130003 | WriteDREAM3DFilter | Write DREAM.3D Data File | 3fcd4c43-9d75-5b86-aad4-4441bc914f37 |  |  |   |
| d2451dc1-a5a1-4ac2-a64d-7991669dcffc | RotateSampleRefFrameFilter | Rotate Sample Reference Frame | e25d9b4c-2b37-578c-b1de-cf7032b5ef19 |  |  |      |
| 0458edcd-3655-4465-adc8-b036d76138b5 | RotateEulerRefFrameFilter | Rotate Euler Reference Frame | ef9420b2-8c46-55f3-8ae4-f53790639de4 |  |  |    |
| 4c8c976a-993d-438b-bd8e-99f71114b9a1 | CopyFeatureArrayToElementArrayFilter | Create Element Array from Feature Array | 99836b75-144b-5126-b261-b411133b5e8a |  |  |   |
| 65128c53-d3be-4a69-a559-32a48d603884 | ReplaceElementAttributesWithNeighborValuesFilter | Replace Element Attributes with Neighbor (Threshold) | 17410178-4e5f-58b9-900e-8194c69200ab |  |  |   |
| 3f695987-81b1-47c3-8cff-b49cfa219be0 | GroupMicroTextureRegionsFilter (SimplnxReview plugin) | Group Microtexture | 5e18a9e2-e342-56ac-a54e-3bd0ca8b9c53 |  |  | *(PRs in SimplnxReview repo — not scanned here)* |
| 7e3dbc15-51a3-482c-97c2-f82f7af685bf | MergeColoniesFilter (SimplnxReview plugin) | Merge Colonies | 2c4a6d83-6a1b-56d8-9f65-9453b28845b9 |  |  | *(PRs in SimplnxReview repo — not scanned here)* |


### PR legend — broad refactors vs. filter-specific work

Most of the PRs above are repo-wide refactors/cleanups that touched many filters but did not perform V&V. When assessing which PRs represent actual verification work for a given filter, filter out the broad ones first:

| PR | Date | Subject | Nature |
|---|---|---|---|
| #1438 | 2025-10-25 | ENH: Microtexture related filter cleanup | Cleanup, broad reach |

PRs not in the legend above are more likely to be filter-specific and worth inspecting for V&V-style content (new tests, exemplar data, algorithm corrections). A rough first pass: **treat the PRs in the legend as noise; the remaining PRs per filter are the candidates for V&V history**.