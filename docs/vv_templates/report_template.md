# V&V Report: <FilterName>

<!-- Write the report in ASD-STE100 Simplified Technical English. Use short sentences, active voice, one meaning per sentence, and consistent technical terms. -->

<!-- The report states the current verified state of the filter. Do NOT put these in a report:
  - Sections, header rows, or At-a-glance rows that this template does not have. Do not rename them.
  - `###` sub-sections inside a template section. Put long evidence in the deviations or provenance sidecar.
  - Workflow journals: "Phase N" sections, step-by-step notes, or "this pass" / "this recertification" narrative.
  - History of earlier V&V passes, earlier statuses, or what changed since the last report. Git history keeps that.
  - Run logs, CTest pass counts with dates, build directories, local or working-folder paths, and virtual-environment details.
  - Text after the Status value (for example, a date). The Sign-off row holds the dates.
  - A Verified commit before SBIR deliverable assembly.
  - Test inventory status values other than `kept`, `new-for-V&V`, and `retired`. Put the reason for a change in Notes.
  - Emoji, check marks, or strike-through text.
  - Any legacy version other than DREAM.3D v6.5.171. A comparison on a non-stock legacy build is "a local proof version".
  - The name "DREAM3D" for the legacy application. Write "DREAM.3D v6.5.171", or "DREAM.3D" for short. "DREAM3D-NX" is the new application.
-->

|        |              |
|--------|--------------|
| Plugin | <PluginName> |
| SIMPLNX UUID | <uuid> |
| SIMPLNX Human Name | <human name shown in DREAM3D-NX> |
| DREAM.3D v6.5.171 equivalent | <LegacyName> *or* None (new filter) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | DRAFT *or* READY FOR REVIEW *or* COMPLETE |
| Sign-off | *<engineer(s), date>* |

## At a glance

A scannable dashboard for reviewers. Each row is one sentence to one short paragraph — enough that a reader can decide whether they need to read the long-form sections below.

| Aspect   | Current state|
|----------|--------------|
| Algorithm Relationship | *Port \| Minor changes \| Rewrite \| New filter* — *one sentence naming the legacy equivalent (or "no legacy") + any material changes.* |
| Oracle (confirmed)     | *Class N — one-sentence applied + one-sentence "encoded as" pointer (e.g., "5 fixtures in `test/<Name>Test.cpp`, all pass").* |
| Code paths enumerated  | *N of M exercised; one phrase about any uncovered paths.*            |
| Tests today            | *N test cases — one phrase about coverage (e.g., "parameter sweep over (Tolerance, NumberOfNeighbors)", "1 positive + 1 negative + 1 SIMPL backward-compat").* |
| Exemplar archive       | *`<archive.tar.gz>` — one phrase on what it provides (inputs only, inputs + outputs, retired/replaced).*       |
| Legacy comparison      | *Run / Not run — one-sentence headline (bit-identical, N deviations). Always framed as SIMPLNX vs DREAM.3D v6.5.171; a run on a non-stock legacy build is called "a local proof version" and names no other legacy version or commit.* |
| Defect hunt            | *N confirmed defects; M of M modes value-asserted; K of N mutants killed.* |
| Bug flags| *None / list of deviation IDs flagged as suspected bugs.*            |
| V&V phase| *Which phases of the V&V workflow are complete; what is outstanding before status promotion.*    |

For a worked instance see `src/Plugins/SimplnxCore/vv/DBSCANFilter.md`.

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

<!-- Include every confirmed defect that the V&V of this filter found and fixed, also when an earlier PR fixed it. Use the stable `<FilterName>-D<N>` deviation IDs. Each ID has an entry in Part 2 of `vv/deviations/<FilterName>.md`, or in Part 1 when the defect is also a difference from DREAM.3D v6.5.171. Do not use `SC-*` labels. State all affected released versions. Use `docs/dream3d_nx_release_dates.md` to identify the DREAM3D-NX releases. If the next release number is not known, state that the fix will be in the release after the latest affected release. Put a confirmed defect that is not fixed in the `Bug flags` row, not in this table. If the V&V found no defects, replace this table with `None.` -->

*This branch fixes all defects in this table. The fixes will be in the DREAM3D-NX release after v<latest affected release>.*

*If an earlier PR fixed some of the defects, add:* *PR #<N> fixed `<FilterName>-D<N>`, and DREAM3D-NX v<release> contains that fix.*

| Deviation | Defect | Affected released versions | Resolution in this branch |
|-----------|--------|----------------------------|---------------------------|
| `<FilterName>-D1` | *State the defect and its effect.* | *DREAM.3D v6.5.171; DREAM3D-NX v<first> through v<last>.* | *State how the verified branch fixes the defect.* |
| `<FilterName>-D2` | *State the defect and its effect.* | *DREAM.3D v6.5.171 only. DREAM3D-NX was not affected.* | *State how the verified branch fixes or prevents the defect.* |

## Defect hunt

*Reviewer:* *<name>* — patterns from [`defect_hunt.md`](../../../../docs/vv_templates/defect_hunt.md) Part 1, with sibling branches compared side by side.

| ID | Pattern | Location | Trigger | Disposition |
|----|---------|----------|---------|-------------|
| H1 | *e.g. Copy-paste drift* | *`<Algo>.cpp:<line>`* | *e.g. centroid offset (1, 2, 3)* | *fixed — `<FilterName>-D1`* / *not a defect — <reason>* / *deferred — <issue link>* |

*Mode coverage:* *M of M modes value-asserted. List any gaps.*

*Mutation check:* *K of N mutants killed. List each survivor and its disposition.*

*Metamorphic relations:* *e.g. translation by (1, 2, 3) moves the output by (1, 2, 3)* / *None.*

## Code path coverage

*N of M paths exercised. If N < M: which paths are NOT covered, and why each gap is acceptable (or what would close it).*

Source: *`src/Plugins/<P>/src/<P>/Filters/Algorithms/<AlgoName>.cpp` (<N> lines).*

*Optional 1–2 sentences naming the algorithm's logical phases (e.g., "(a) preflight scan, (b) per-cell accumulation, (c) per-feature finalize") so the `Phase` column reads in context.*

| #  | Phase           | Path    | Test case      |
|----|-----------------|---------|----------------|
| 1  | *(a) Preflight* | *e.g. all phases non-Hex → return error `-76402`* | *`No_Hex_Phase`*             |
| 2  | *(a) Preflight* | *e.g. mixed phases → push warning, proceed*       | *`Class 1 Oracle`*           |
| 3  | *(b) Per-cell*  | *e.g. `featureId == 0` → skip*      | *Not directly tested. <one-line reason.>*  |
| 4  | *(b) Per-cell*  | *e.g. normal accumulation branch*   | *`Class 1 Oracle` — F1, F2, F3 checks*     |
| 5  | *(c) Finalize*  | *e.g. `cellCount == 0` → write NaN* | *`Class 1 Oracle` — F0, F5, F6 NaN checks* |

When a path is intentionally not covered, write `*Not directly tested. <one-line reason — e.g., "low-value loop guard", "exercised implicitly by shipping pipelines", "requires cancel-signal injection">*` in the Test case cell rather than omitting the row. Uncovered paths must still appear in the table so the reader can audit the gap.

*The `Phase` column can be dropped (or renamed `Pass` / `Stage`) when the algorithm has no obvious staged structure. For a worked two-pass instance see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md`; for a worked three-phase instance see `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md`.*

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| *TestName* | kept / new-for-V&V / retired | *one line if needed* |

## Exemplar archive

- **Archive:** *`<name_vN.tar.gz>`*
- **SHA512:** *`<copy from test/CMakeLists.txt>`*
- **Provenance:** *`src/Plugins/<P>/vv/provenance/<name>.md`*

## Deviations from DREAM.3D v6.5.171

*Either:*

- No deviations observed. Comparison run on *<fixture>*.

*Or:*

See `vv/deviations/<FilterName>.md` for the root cause, affected users, and recommendation for each deviation. List only the Part 1 entries of that file here. Part 2 entries are SIMPLNX defects and SIMPLNX behavior changes; the `Bugs found and fixed` table and the `Bug flags` row refer to them.

| Deviation | Observed difference |
|-----------|---------------------|
| `<FilterName>-D1` | *State the user-visible difference.* |
| `<FilterName>-D2` | *State the user-visible difference.* |
