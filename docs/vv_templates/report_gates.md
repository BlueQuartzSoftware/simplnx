# V&V Report — Section Gates

A section of `src/Plugins/<P>/vv/<FilterName>.md` is "done" when all its gates pass. Sections may be worked in any order.

**The only ordering rule:** the **Oracle** is chosen *before* any DREAM3D 6.5.171 comparison is run. 6.5.171 is never the source of truth — it is the *thing being compared against* the independently-established oracle.

---

## Header table

- [ ] Plugin, SIMPLNX UUID, legacy DREAM3D equivalent (or "None") filled in
- [ ] Status reflects current state: `DRAFT` | `READY FOR REVIEW` | `COMPLETE`
- [ ] Verified-commit field present (filled in at SBIR deliverable assembly)
- [ ] Sign-off line has named engineer(s) and date at sign-off

## At a glance

The dashboard a reviewer reads first. Lets a reviewer decide in 30 seconds whether they need to dig into the long-form sections below.

- [ ] All 8 rows present: Algorithm Relationship, Oracle (confirmed), Code paths enumerated, Tests today, Exemplar archive, Legacy comparison, Bug flags, V&V phase
- [ ] Each cell is one sentence to one short paragraph — not a single word, not a full subsection. If a row needs more than ~3 sentences, that detail belongs in the long-form section and the dashboard summarizes it
- [ ] **Algorithm Relationship** row names the legacy equivalent (or "no legacy equivalent") and the classification — must agree with the long-form `## Algorithm Relationship` section
- [ ] **Oracle (confirmed)** row names the Class number(s) and the encoded test fixture(s). Use "confirmed" only when the oracle has been applied and the test passes; otherwise write "tentative" or "in progress"
- [ ] **Code paths enumerated** row states `N of M exercised` — agrees with the long-form `## Code path coverage` count
- [ ] **Tests today** row gives the test-case count and a one-phrase shape of coverage (parameter sweep, positive/negative/conversion, etc.)
- [ ] **Exemplar archive** row names the archive and flags retired/replaced archives (cross-reference the long-form `## Exemplar archive` SHA512)
- [ ] **Legacy comparison** row is `Run` / `Not run` / `Three-way (SIMPLNX vs 6.5.171 vs 6.5.172)` plus a one-sentence headline. "Not run" must include a brief reason ("design-by-inspection — pure port", "legacy binary unavailable", "deferred to Phase 9")
- [ ] **Bug flags** row is `None` or a list of deviation IDs flagged as suspected bugs (not all deviations are bugs; only those classified as bug under the root-cause taxonomy)
- [ ] **V&V phase** row lists which phases of the workflow are complete and what is outstanding — drives the Status field in the header table

## Summary

- [ ] 2–3 sentences only
- [ ] States what the filter does
- [ ] States the verification approach (one phrase, e.g., "Class 3 paper-based vs Rowenhorst 2015")
- [ ] States the headline result (e.g., "1 deviation, all tests pass")

## Algorithm Relationship

- [ ] One classification: Port | Minor changes | Rewrite | New filter
- [ ] One-line evidence (UUID inheritance, PR history, line-count diff with legacy)
- [ ] **Rewrite + outputs diverge from legacy** → explicit defense required in the Deviations file (same UUID is a claim of functional equivalence)
- [ ] *Optional but encouraged for Port / Minor changes:* numbered list of **Port-time deltas** — API swaps, library version differences, progress-reporting changes, normalization steps added. Each delta gets one sentence justifying why it does or does not change output. Forces the engineer to *enumerate* the structural diff instead of asserting "it's a port" with no evidence.
- [ ] *Optional:* **Material PRs since baseline** line — `(none identified for this filter)` is a valid answer. The discipline of looking is what matters; it surfaces drift introduced after the last V&V pass.

## Oracle

For detailed explanations of each class — with examples, strengths and weaknesses, drift-risk analysis, and a decision tree for picking the right class — see [`oracle_classes.md`](./oracle_classes.md). The summary below is the gate checklist.

- [ ] Class named (1–5)
  - 1 = Analytical (closed-form expected output on hand-built input)
  - 2 = Reference implementation (NumPy / SciPy / MTEX / EbsdLib upstream)
  - 3 = Paper-based (published figure / table / equation)
  - 4 = Invariant-based (derivable property the output must satisfy)
  - 5 = Expert-visual (last resort, requires justification)
- [ ] If Class 5: justification block stating why no Class 1–4 oracle was feasible
- [ ] One-line description of how oracle was applied
- [ ] Encoded test reference: `<file>::<TEST_CASE>` exists and is greppable
- [ ] N fixtures stated; all pass at the verified commit
- [ ] Second-engineer review of oracle design, OR documented skip reason

## Code path coverage

- [ ] `Source:` line cites the algorithm `.cpp` with line count (e.g., `Source: src/Plugins/<P>/.../Algorithms/<Name>.cpp (181 lines).`) — anchors the reader to the file being audited
- [ ] *Optional 1–2 sentences* naming the algorithm's logical phases when it has staged structure (e.g., "(a) preflight scan, (b) per-cell accumulation, (c) per-feature finalize") so the `Phase` column reads in context
- [ ] All algorithm code paths enumerated — kernel choices, mask on/off, edge cases, error paths, cancel paths, background/sentinel branches
- [ ] Paths numbered via the `#` column so they have stable IDs for referencing from Test inventory, Deviations, and review comments
- [ ] *Recommended:* `Phase` (or `Pass` / `Stage`) column groups paths when the algorithm has distinct stages; drop the column when the algorithm is flat
- [ ] `N of M paths exercised.` count stated at the top of the section
- [ ] If `N < M`: each uncovered path appears as its own table row with `*Not directly tested. <one-line reason>*` in the Test case cell — paths are **never silently omitted**. Acceptable reasons include: low-value loop-guard, exercised implicitly by shipping pipelines (name one), requires cancel-signal injection, deferred to integration test
- [ ] Each covered path maps to ≥1 named test case (`TEST_CASE` name or `DYNAMIC_SECTION` label as it appears in the test source)
- [ ] Parameter-dependent paths: every combination of interest represented (don't trust a single test case to cover the parameter cube)

## Test inventory

- [ ] Every `TEST_CASE` in the filter's test file listed (including `DYNAMIC_SECTION` variants that show up as separate ctest entries — list each one)
- [ ] Each marked: `kept` | `new-for-V&V` | `retired` (with one-line reason for retired)
- [ ] **Notes** column states what each test actually verifies (number of arrays compared, number of assertions, exemplar archive consumed, any expected-failure status). Don't leave Notes blank — "Validates 80 element-wise assertions against bundled exemplar" beats no entry
- [ ] If a test was modified for this V&V cycle (e.g., inline expected-array updates, exemplar bump), the Notes column records what changed and why (one line; cite the deviation ID if the change is traceable to one)
- [ ] All non-retired tests pass at the verified commit in **both** in-core and OOC builds

## Exemplar archive

- [ ] Archive name matches `download_test_data()` entry in `test/CMakeLists.txt`
- [ ] SHA512 in report matches SHA512 in `test/CMakeLists.txt`
- [ ] Provenance sidecar exists at `src/Plugins/<P>/vv/provenance/<archive>.md` and documents:
  - who generated the archive
  - when
  - with what pipeline / script
  - what oracle output was canonical
- [ ] If the archive was regenerated during V&V to fix a circular-oracle situation → documented in the sidecar

## Deviations from DREAM3D 6.5.171

- [ ] Comparison was run on at least one fixture (named in the report)
- [ ] If no deviations: a one-line confirmation that 6.5.171 and SIMPLNX outputs matched within tolerance
- [ ] If deviations: each ID referenced in the report points to a fleshed-out entry in `src/Plugins/<P>/vv/deviations/<FilterName>.md`
- [ ] Each deviation entry has:
  - stable ID (`<FilterName>-D<N>`)
  - filter UUID (permanent cross-reference anchor)
  - symptom (user-visible)
  - root cause: `bug` | `precision` | `order of operations` | `library` | `algorithmic choice`
  - affected users
  - recommendation: `trust SIMPLNX` | `trust 6.5.171` | `either acceptable within tolerance` | `see quick-patch link`
