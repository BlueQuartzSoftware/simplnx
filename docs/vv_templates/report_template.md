# V&V Report: <FilterName>

<!-- Write the report in ASD-STE100 Simplified Technical English. Use short sentences, active voice, one meaning per sentence, and consistent technical terms. -->

|        |              |
|--------|--------------|
| Plugin | <PluginName> |
| SIMPLNX UUID | <uuid> |
| DREAM3D 6.5.171 equivalent | <LegacyName> *or* None (new filter) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | DRAFT |
| Sign-off | *<engineer(s), date>* |

## At a glance

A scannable dashboard for reviewers. Each row is one sentence to one short paragraph — enough that a reader can decide whether they need to read the long-form sections below.

| Aspect                 | Current state                                                                                                                |
|------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | *Port \| Minor changes \| Rewrite \| New filter* — *one sentence naming the legacy equivalent (or "no legacy") + any material changes.* |
| Oracle (confirmed)     | *Class N — one-sentence applied + one-sentence "encoded as" pointer (e.g., "5 fixtures in `test/<Name>Test.cpp`, all pass").* |
| Code paths enumerated  | *N of M exercised; one phrase about any uncovered paths.*                                                                    |
| Tests today            | *N test cases — one phrase about coverage (e.g., "parameter sweep over (Tolerance, NumberOfNeighbors)", "1 positive + 1 negative + 1 SIMPL backward-compat").* |
| Exemplar archive       | *`<archive.tar.gz>` — one phrase on what it provides (inputs only, inputs + outputs, retired/replaced).*                     |
| Legacy comparison      | *Run / Not run — one-sentence headline (bit-identical, N deviations). Always framed as SIMPLNX vs DREAM3D 6.5.171; root-cause proof via a patched local build of the legacy source is described without naming a legacy version or commit.* |
| Bug flags              | *None / list of deviation IDs flagged as suspected bugs.*                                                                    |
| V&V phase              | *Which phases of the V&V workflow are complete; what is outstanding before status promotion.*                                |

For worked instances see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md` and `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md` (on `topic/vv/compute_avg_caxis`).

## Summary

*2–3 sentences: what the filter does, how it was verified, headline result.*

## Algorithm Relationship

*One of:* Port | Minor changes | Rewrite | New filter

*Evidence:* *one line — UUID inheritance, PR history, complexity comparison.*

## Oracle

*Class:* *N (1=Analytical, 2=Reference, 3=Paper, 4=Invariant, 5=Expert-visual)*

*Applied:* *one line describing how the oracle generates expected output.*

*Encoded:* *`<test file>::<TEST_CASE>` — N fixtures, all pass.*

*Second-engineer review:* *<name, date>* OR *skipped — reason.*

## Bugs found and fixed

<!-- Include only confirmed defects that are fixed in the verified branch. Use the stable `<FilterName>-D<N>` deviation IDs. Do not use `SC-*` labels. State all affected released versions. Use `docs/dream3d_nx_release_dates.md` to identify the DREAM3D-NX releases. If the next release number is not known, state that the fix will be in the release after the latest affected release. If the V&V found no defects, replace this table with `None.` -->

*This branch fixes all defects in this table. The fixes will be in the DREAM3D-NX release after v<latest affected release>.*

| Deviation | Defect | Affected released versions | Resolution in this branch |
|---|---|---|---|
| `<FilterName>-D1` | *State the defect and its effect.* | *DREAM.3D 6.5.171; DREAM3D-NX v<first> through v<last>.* | *State how the verified branch fixes the defect.* |
| `<FilterName>-D2` | *State the defect and its effect.* | *DREAM.3D 6.5.171 only. DREAM3D-NX was not affected.* | *State how the verified branch fixes or prevents the defect.* |

## Code path coverage

*N of M paths exercised. If N < M: which paths are NOT covered, and why each gap is acceptable (or what would close it).*

Source: *`src/Plugins/<P>/src/<P>/Filters/Algorithms/<AlgoName>.cpp` (<N> lines).*

*Optional 1–2 sentences naming the algorithm's logical phases (e.g., "(a) preflight scan, (b) per-cell accumulation, (c) per-feature finalize") so the `Phase` column reads in context.*

| #  | Phase           | Path                                              | Test case                                  |
|----|-----------------|---------------------------------------------------|--------------------------------------------|
| 1  | *(a) Preflight* | *e.g. all phases non-Hex → return error `-76402`* | *`No_Hex_Phase`*                           |
| 2  | *(a) Preflight* | *e.g. mixed phases → push warning, proceed*       | *`Class 1 Oracle`*                         |
| 3  | *(b) Per-cell*  | *e.g. `featureId == 0` → skip*                    | *Not directly tested. <one-line reason.>*  |
| 4  | *(b) Per-cell*  | *e.g. normal accumulation branch*                 | *`Class 1 Oracle` — F1, F2, F3 checks*     |
| 5  | *(c) Finalize*  | *e.g. `cellCount == 0` → write NaN*               | *`Class 1 Oracle` — F0, F5, F6 NaN checks* |

When a path is intentionally not covered, write `*Not directly tested. <one-line reason — e.g., "low-value loop guard", "exercised implicitly by shipping pipelines", "requires cancel-signal injection">*` in the Test case cell rather than omitting the row. Uncovered paths must still appear in the table so the reader can audit the gap.

*The `Phase` column can be dropped (or renamed `Pass` / `Stage`) when the algorithm has no obvious staged structure. For a worked two-pass instance see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md`; for a worked three-phase instance see `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md` (on `topic/vv/compute_avg_caxis`).*

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| *TestName* | kept / new-for-V&V / retired | *one line if needed* |

## Exemplar archive

- **Archive:** *`<name_vN.tar.gz>`*
- **SHA512:** *`<copy from test/CMakeLists.txt>`*
- **Provenance:** *`src/Plugins/<P>/vv/provenance/<name>.md`*

## Deviations from DREAM.3D 6.5.171

*Either:*

- No deviations observed. Comparison run on *<fixture>*.

*Or:*

See `vv/deviations/<FilterName>.md` for the root cause, affected users, and recommendation for each deviation.

| Deviation | Observed difference |
|---|---|
| `<FilterName>-D1` | *State the user-visible difference.* |
| `<FilterName>-D2` | *State the user-visible difference.* |
