# V&V Commit Message Template

This is the standard commit message format for landing a completed V&V cycle on a SIMPLNX filter. It is used by the engineer at Phase 13 (status flip DRAFT → READY FOR REVIEW / COMPLETE), and is the commit that opens the PR.

## Format

```
VV: <Filter Human Name> fully V&V'ed

Summary:
- Found and fixed <N> bug(s) (<one-line per bug, or "Confirmed no bugs">);
- documented <M> deviation(s) from DREAM3D 6.5.171 (<D1 …, D2 …>);
- retired <P> tests (<reason: circular oracle / UNIMPLEMENTED stub / etc., or "none">);
- unit tests replaced with <Q> inlined *<Class X (Name) + Class Y (Name)>* test fixtures;
- added 3 V&V source-tree deliverables (report, deviations, provenance);
- <optional: doc/pipeline-name fixes, when applicable>.
```

## Title rules

- Must start with `VV:` (matches existing precedent — e.g., commit `99f3a9865` for `Compute Feature Reference Misorientations`).
- Use the filter's `humanName()` from the `.cpp` (with spaces, not the class name).
- End with `fully V&V'ed`. If the cycle is partial (rare — e.g., second-engineer oracle review still pending), end with `— READY FOR REVIEW` instead.

## Body rules

- One bullet per category, even when the count is zero. **Don't drop bullets** — write "Confirmed no bugs" or "no tests retired" so the reader knows the question was asked.
- Past-tense verbs at the start of each bullet ("Found and fixed", "documented", "retired", "replaced", "added", "fixed").
- Semicolons at the end of each bullet except the last (period on the last).
- Inline the deviation IDs (`D1`, `D2`, …) with a 2-3 word reminder of what each one was. The full text lives in `vv/deviations/<FilterName>.md`; the commit message just needs to be greppable.
- Oracle classes get the parenthetical name (`Class 1 (Analytical)`, `Class 4 (Invariant)`). See `oracle_classes.md` for the canonical class list.
- The "3 V&V source-tree deliverables" line is invariant across all V&V commits (every cycle produces exactly those 3). Don't rewrite it per-filter.

## Worked example

This is what the F#2 (`ComputeFeatureNeighborMisorientations`) V&V cycle's commit looks like:

```
VV: Compute Feature Neighbor Misorientations fully V&V'ed

Summary:
- Found and fixed 1 bug (divisor clobbered inside inner j-loop of algorithm.cpp);
- documented 2 deviations from DREAM3D 6.5.171 (D1 divisor bug, D2 EbsdLib 2.4.1 precision);
- retired 2 tests (circular-oracle exemplar consumer + UNIMPLEMENTED stub);
- unit tests replaced with 4 inlined *Class 1 (Analytical) + Class 4 (Invariant)* test fixtures;
- added 3 V&V source-tree deliverables (report, deviations, provenance);
- fixed pipeline-name typo in user-facing doc.
```

## When a category is zero or N/A

| Situation                                          | Bullet phrasing                                                       |
|----------------------------------------------------|-----------------------------------------------------------------------|
| Clean Port, no bugs found                          | `Confirmed no bugs (clean Port);`                                     |
| No deviations from legacy                          | `Confirmed no deviations from DREAM3D 6.5.171;`                       |
| Fresh V&V, no prior tests to retire                | `No prior tests retired (fresh V&V cycle);`                           |
| Existing tests kept, only new ones added           | `Augmented existing tests with <Q> inlined …;`                        |
| Doc was already correct                            | *omit the last bullet entirely (it's the only optional one)*          |
| New filter, no legacy equivalent                   | `documented 0 deviations (no legacy equivalent — new filter);`        |

## Why this format

- **Scannable.** A reviewer skimming `git log` should be able to decide whether a V&V commit is worth opening from the title and one bullet pass.
- **Greppable.** `git log --grep='^VV:'` returns every V&V commit. `git log --grep='D2 EbsdLib'` returns every commit that mentions the EbsdLib precision deviation. The deviation IDs are stable across commits (per the `<FilterName>-D<N>` rule in `deviation_template.md`).
- **Constrained.** The five fixed categories (bugs / deviations / retired tests / new fixtures / V&V deliverables) match what every V&V cycle produces under the v2 policy. The engineer fills slots rather than designing prose.
- **Cross-references the source-tree deliverables.** The commit is the "shortest legible record" of the V&V; the long-form analysis lives at `src/Plugins/<Plugin>/vv/{,deviations/,provenance/}<FilterName>.md`. A reader who wants more clicks through.
