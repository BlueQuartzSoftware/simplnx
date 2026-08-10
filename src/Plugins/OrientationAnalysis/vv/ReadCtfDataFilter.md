# V&V Report: ReadCtfDataFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `7751923c-afb9-4032-8372-8078325c69a4` |
| SIMPLNX Human Name | Read Oxford Instr. EBSD Data (.ctf) |
| DREAM3D 6.5.171 equivalent | `ReadCtfData` (SIMPL UUID `d1df969c-0428-53c3-b61d-99ea2bb6da28`) — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ReadCtfData.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE — all V&V phases complete; verified-correct against independent oracle; second-engineer sign-off recorded at PR review. |
| Sign-off | Michael A. Jackson <mike.jackson@bluequartz.net> — 2026-07-24. Second engineer: Jared Duffey, 2026-07-28 (PR #1692 review). |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Minor changes.** Faithful port of legacy `ReadCtfData` control flow with deliberate deltas: unindexed-point (Phase 0) remap removed (D1/D2, PR #937 — predates this pass), 3D multi-slice support restored (D4, fixed this pass), five malformed-input guards added (D3), Euler math on double-precision intermediates (restores legacy bit-parity), and the legacy PIMPL file-cache dropped (no output effect). |
| Oracle (confirmed)     | **Confirmed.** **Class 1 (analytical) + Class 4 (invariant)**, scoped to the filter's value-add per the "don't re-test upstream" rule — EbsdLib (vcpkg 3.1.0) owns `.ctf` parsing and is trusted (Class 2 boundary). Hand-authored inline toy `.ctf` fixtures (3×2 two-phase, 2×2×2 multi-slice; all values float32-exact) with every expected value hand-derived from the fixture text; the two angle transforms (+30° hex alignment, degrees→radians) are correctly-rounded IEEE-754 double-intermediate results derived independently with NumPy. Encoded as 12 TEST_CASEs in `test/ReadCtfDataTest.cpp`; all pass. SIMPLNX matched the oracle with zero discrepancies. |
| Code paths enumerated  | 19 of 22 paths exercised (see Code path coverage); the gaps are the unreadable-header passthrough (needs permission manipulation), the file-changed phase-count guard `-19605` (race window inside a single execute; needs injection), and the cancel-signal paths (need injection; untested per scope). |
| Tests today            | 12 test cases: Class 1+4 analytical oracle, 4-combo Euler-conversion sweep (DYNAMIC_SECTION) with double-precision-pinning angle values, 2-case 3D multi-slice (DYNAMIC_SECTION), 5 value-add error-guard tests (−19600…−19604 incl. an exact-boundary phase value; −19605 is injection-only), 3 EbsdLib error passthroughs (−102/−105/−107), and SIMPL 6.4/6.5 backwards-compat (new — the filter previously had no conversion test). All inline hand-built fixtures — no exemplar archive. |
| Exemplar archive       | **None — retired `6_6_read_ctf_data_2.tar.gz`** (legacy-generated exemplar = forbidden oracle). `download_test_data()` entry removed from `test/CMakeLists.txt`; retirement documented in `vv/provenance/6_6_read_ctf_data_2.md`. Its production Cugrid scan lives on as an A/B fixture in the comparison working folder only. |
| Legacy comparison      | **Run (2026-07-24) vs the official DREAM3D 6.5.171 release.** Four runs over three byte-identical input files: toy (2 conversion combos), Cugrid 550×400 production scan, 2×2×2 multi-slice toy. **All numeric outputs bit-identical** — 543,950 of the production scan's 660,000 Euler values match exactly (the 116,050 differing values are all the unindexed points' φ2), and the toy's double-precision-pinning angles match legacy bit-for-bit — except the unindexed-point family: Phases 0→1 remap (D1) and the consequent +30° on unindexed φ2 (D2). Three malformed-input fixtures demonstrated legacy segfaults/silent corruption vs SIMPLNX errors (D3). |
| Bug flags              | Legacy: crash/UB on malformed files, **empirically confirmed** (two segfaults, one silent heap-dependent output) — D3. SIMPLNX (pre-pass): multi-slice `.ctf` silently truncated to slice 0 — D4, **fixed this pass** and pinned by the 3D test; latent OOB/null-deref twins of the legacy crashes existed in the NX copy path and were guarded this pass (−19600/−19601/−19602/−19603). |
| V&V phase              | Discovery, relationship, oracle, reconciliation, algorithm review (fixes applied), tests, legacy comparison, deviations, provenance, docs — **complete**. Tests pass 12/12 in both `simplnx-Rel` and `simplnx-ooc-Rel` (OOC caveat: that build's out-of-core backend registration is under separate investigation; its pass is reported as-run). Second-engineer sign-off completed at PR review (Jared Duffey, 2026-07-28, PR #1692). |

## Summary

`ReadCtfDataFilter` ("Read Oxford Instr. EBSD Data (.ctf)") imports a single Oxford/HKL Channel 5 `.ctf` file into a new Image Geometry: it builds the geometry from the header (XCells/YCells/ZCells, XStep/YStep/ZStep, origin 0, Micrometer), creates one cell array per data column plus the condensed 3-component `EulerAngles` and verbatim `Phases` arrays (optionally applying the EDAX hexagonal +30° φ2 alignment and degrees→radians conversion), and populates the ensemble arrays (`CrystalStructures` via EbsdLib's Laue-group mapping, `MaterialName`, `LatticeConstants`) with slot 0 reserved for the "Invalid Phase". Verification is Class 1 analytical + Class 4 invariant on hand-authored inline toy `.ctf` fixtures whose expected outputs were derived independently of both codebases (EbsdLib parsing is trusted upstream and not re-tested). Headline result: SIMPLNX matches the oracle exactly; against DREAM3D 6.5.171 every numeric output is bit-identical on all four fixtures except the deliberate unindexed-point deviations (D1/D2), with one SIMPLNX functional gap (3D multi-slice import, D4) found and fixed during this pass and legacy's malformed-input crashes (D3) empirically demonstrated. All 12 unit tests pass; the legacy-oracle exemplar archive `6_6_read_ctf_data_2.tar.gz` is retired.

## Algorithm Relationship

*Classification:* **Minor changes.**

*Evidence:* Same SIMPL UUID inherited (`d1df969c-…` → SIMPLNX `7751923c-…`) with `FromSIMPLJson` conversion and 6.4/6.5 fixtures at `test/simpl_conversion/{6_4,6_5}/ReadCtfDataFilter.json`. The legacy `ReadCtfData::copyRawEbsdData()` structure (phase copy, Euler interleave with hex-alignment/radians options, verbatim column copies) and `loadMaterialInfo()` (slot-0 defaults + per-phase fill) are preserved block-for-block in `Algorithms/ReadCtfData.cpp`.

### Port-time deltas (each mapped to a Deviation entry where user-visible)

1. **Unindexed-point remap removed** (D1, and consequence D2). Legacy remaps phase `< 1 → 1` in the reader buffer before storing `Phases`, which also routes unindexed points into the hex +30° branch; SIMPLNX copies the phase column verbatim (deliberate change, PR #937, May 2024). Changes `Phases` and unindexed-point `EulerAngles` output.
2. **3D multi-slice support restored** (D4, fixed this pass). Legacy sizes the geometry `XCells × YCells × ZCells` with `ZStep` slice thickness; the NX port had hard-coded z=1/1.0 and silently imported only slice 0. Preflight now mirrors legacy (`ZCells` clamped to ≥1; `ZStep` of 0 → 1.0). Bit-identical to legacy on the 3D fixture.
3. **Malformed-input guards added** (D3): empty phase list (−19600), missing data column (−19601), out-of-range phase value (−19602), reader/geometry cell-count mismatch (−19603), zero XCells/YCells at preflight (−19604), and a file-changed phase-count guard in the ensemble fill (−19605, the ReadAngData −19504 analog). Legacy segfaults or silently produces heap-dependent output on these inputs (demonstrated where statically reachable). Rejection paths only; no effect on well-formed files.
4. **Double-precision Euler intermediates.** Legacy computes `+30.0` and `× M_PI/180.0` with double intermediates (float operands promoted); the NX port used float32 arithmetic throughout. Restored double intermediates this pass — more accurate (correctly-rounded float32 results) and bit-identical to legacy on all indexed points of every fixture. No deviation entry needed.
5. **`determineLaueGroup()` → `determineOrientationOpsIndex()`** — the EbsdLib Laue-group→structure mapping was diffed function-body-for-function-body against the 6.5.171 bundled EbsdLib: **identical bodies and identical enum values** (pure rename). No output effect.
6. **Error-code source fix.** `CtfReader::readFile()` leaves its error-code member at 0 on its zero-step/zero-cells rejections (message only); the filter now falls back to the returned code (−102/−103) instead of reporting error 0. Rejection paths only.
7. **Legacy PIMPL file-cache dropped** (`ReadCtfDataPrivate`/`Ctf_Private_Data` — ported as dead declarations, removed during this pass). No output effect; NX re-reads the file on each execute.

*Material PRs since baseline:* PR #937 (phase-0 remap removal, May 2024 — deviation D1/D2); EbsdLib API migrations (#1122, #1472 — `ebsdlib` namespace, no algorithm change).

## Oracle

*Class:* **1 (Analytical) + 4 (Invariant)**; EbsdLib parsing = **Class 2 boundary (trusted, not re-tested)**.

### The EbsdLib boundary (what we do NOT re-test)

EbsdLib's `CtfReader` (vcpkg 3.1.0) owns: header-key parsing, phase-section parsing (lattice constants/angles, names, Laue group), data-column tokenizing and typed buffer allocation, European-decimal fix-up, multi-slice line ordering, and its own error codes (−100 unopenable, −102/−103 zero step/cells, −105 premature EOF, −107 unknown column, etc.). Those behaviors are upstream's to verify. The filter's value-add — everything this oracle covers — is the deterministic plumbing on top: geometry construction (including 3D), array creation/typing, verbatim phase copy, Euler interleave + the two optional angle transforms, ensemble sizing + slot-0 defaults, Laue-group index placement, lattice-constant copy, and the six value-add error paths (−19600…−19605).

### Applied

Hand-authored toy `.ctf` fixtures live as string literals in the test source: a 3×2 grid (XStep 0.25 / YStep 0.5) with hexagonal "Hex Phase A" (Laue 9) + cubic "Copper" (Laue 11) and two unindexed Phase-0 points, and a 2×2×2 multi-slice variant (ZStep 0.75). Every fixture value is float32-exact (multiples of 1/8), so verbatim copies are asserted with exact equality. Expected outputs — geometry (3,2,1)/(0.25,0.5,1.0)/(0,0,0)/Micrometer and (2,2,2)/(0.25,0.5,0.75), `Phases {1,2,0,1,2,0}` (verbatim, Phase 0 preserved), the 18-value Euler interleave, 7 verbatim pass-through columns, `CrystalStructures {999,0,1}` (Unknown / Hexagonal_High / Cubic_High from documented HKL Laue groups 9 and 11), material names, lattice constants — were derived by hand from the fixture text. The +30° and degrees→radians expectations are the correctly-rounded float32 results of double-precision arithmetic (`value × M_PI/180.0`), computed independently with NumPy IEEE-754 float32/float64 semantics and embedded as exact literals with derivation comments. Class 4 invariants: ensemble tuple count = phases+1; slot-0 Invalid-Phase defaults; `Phases` unaffected by either conversion option; unindexed/cubic points never receive the hex shift.

*Encoded:*
- `test/ReadCtfDataTest.cpp::"…Class 1 Analytical Oracle"` — full Class 1+4 assertion set (geometry, 9 cell arrays element-wise, 3 ensemble arrays element-wise).
- `…::"Euler Conversion Combinations"` — DYNAMIC_SECTION over the 2×2 DegreesToRadians × EdaxHexagonalAlignment grid, 18 Euler values each, hex shift scoped to Hexagonal_High points only.
- `…::"3D Multi-Slice CTF"` — DYNAMIC_SECTION over ZStep present/absent; dims/spacing/arrays for the 2-slice fixture (regression pin for D4).
- `…::"Empty Phases rejected (-19600)"`, `…::"Missing Data Column rejected (-19601)"`, `…::"Out-of-Range Phase Value rejected (-19602)"`, `…::"Reader/Geometry Cell Count Mismatch rejected (-19603)"`, `…::"Invalid Cells Preflight Error (-19604)"` — value-add guards.
- `…::"EbsdLib Error Passthrough - Zero Step (-102)"`, `…- Truncated Data (-105)"`, `…- Unknown Column (-107)"` — error propagation from the trusted boundary (three distinct return points in `CtfReader`).
- `…::"SIMPL Backwards Compatibility"` — UUID + argument conversion (6.4 and 6.5 fixtures; DYNAMIC_SECTION).

All 12 pass in `simplnx-Rel` and `simplnx-ooc-Rel`. Reconciliation found zero SIMPLNX-vs-oracle discrepancies (the pre-identified fixes — 3D support, guards, double intermediates — were implemented before the first oracle run). Every fixed delta has a regression pin that fails against the pre-pass code: the guard error codes did not exist, the 3D dims assertion cannot pass against the z=1 hard-code, and — added after the adversarial review found the original angles blind to it — four Euler fixture values (7.125°, 3.375°, 6.75°, and 37.125° via the hex shift) whose correctly-rounded results differ between float32 arithmetic and the double-precision intermediates.

*Second-engineer review:* **Signed off by Jared Duffey, 2026-07-28** (PR #1692 review). A dedicated oracle re-derivation was scoped as unnecessary — documented reason: the filter's value-add is pure data plumbing plus two elementwise transforms whose expected values are mechanically derivable IEEE-754 roundings, leaving no design freedom for the author-bias failure mode the review guards against. See `vv/provenance/6_6_read_ctf_data_2.md`. The V&V work was authored by Michael A. Jackson, so the PR review is independent of the author.

## Algorithm review

Line-by-line review performed via the `review-algorithm` skill after oracle reconciliation. All findings applied (all 12 tests pass after rebuild):

- **Robustness (Critical):** added the five malformed-input guards (−19600…−19604) replacing two demonstrated-segfault paths, one silent OOB-read path, and two zero-dimension paths (details under D3).
- **Correctness:** Euler transforms moved to double-precision intermediates (correctly-rounded results; restores legacy bit-parity). Error-code fallback for `CtfReader` failure paths that set only the message.
- **Dead code:** removed the vestigial legacy PIMPL (`ReadCtfDataPrivate`, `Ctf_Private_Data`), the unused `FloatVec3Type` alias, unused `tDims`/`cDims` locals, a dead `PreflightResult` local, an unused `LaueOps.h` include, and scaffold-generator comments.
- **API consistency:** `loadMaterialInfo`/`copyRawEbsdData` now return `Result<>` (matches post-V&V `ReadAngData`).
- **Progress messaging:** status messages before the EbsdLib read and the cell-data copy (loops are memcpy-speed; no throttled messenger warranted).
- **Cancel checks:** 3 early-return sites added.
- **Naming:** `angPhases` → `ctfPhases`; doc `@brief`s filled in.

### Five-perspective hardening review (post-deliverable)

Five independent reviews were run after the deliverables were drafted; all findings applied and all 12 tests re-pass:

- **Adversarial:** found the conversion suite blind to the double-precision Euler fix (all original angles round identically under float32 and double arithmetic) — fixed by re-deriving the fixture with four diverging angles, re-verified bit-identical against 6.5.171; found a stale −19600 rationale comment and a wrong "mirrors readData" claim for negative ZCells (negative ZCells made `CtfReader::readData()` read zero lines and surface a garbage-value −19602 — now rejected at preflight with −19604 and tested); moved the −19602 test to the exact boundary value; corrected two report wording errors. All hand-derived oracle values, guard-reachability claims, and A/B consistency survived independent recomputation.
- **Senior engineer:** added the −19605 file-changed phase-count guard (the ReadAngData −19504 analog), the preflight error-code fallback, removed dead `getCancel()`, include-what-you-use and sibling-parity cleanups, error-message substring pins and `CheckArraysInheritTupleDims` in every error test, docs grammar/markup fixes.
- **CPU performance:** no action needed — copies are ~5–10% of wall time (EbsdLib text parsing dominates); idioms match the post-V&V ReadAngData baseline; the only lever (bulk store I/O) does not exist on this branch.
- **Memory:** no bugs; peak ≈ 88 bytes/scan-point (reader + destination resident simultaneously) now documented in the filter docs; reader lifetime/cleanup verified correct on all return paths.
- **Out-of-core:** all destination writes are forward-sequential and chunk-cache-benign; the Euler loop's re-read of the just-written Phases array was replaced with the reader's in-core buffer (bit-identical, removes an OOC read-back stream); `copyFromBuffer`-style conversions deferred until that API exists outside the OOC rewrite branch.

## Code path coverage

*19 of 22 enumerated paths exercised; the gaps are one passthrough needing permission manipulation (row 1), the file-changed phase-count guard (row 9b), and the cancel checks (row 21). Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadCtfData.cpp` (272 lines) + preflight in `Filters/ReadCtfDataFilter.cpp` (268 lines).* Logical phases: **(a)** preflight (header-only read → output actions), **(b)** execute read + ensemble population (`loadMaterialInfo`), **(c)** cell-data copy (`copyRawEbsdData`).

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) Preflight | `readHeaderOnly` error → passthrough | *Not directly tested — `FileSystemPathParameter` rejects missing files first; an unreadable-but-present file needs permission manipulation. Same gap accepted for ReadAngDataFilter row 1.* |
| 2  | (a) Preflight | `XCells/YCells < 1` or `ZCells < 0` → `-19604` | `Invalid Cells Preflight Error (-19604)` (DYNAMIC_SECTION: zero XCells; negative ZCells) |
| 3  | (a) Preflight | Geometry action: dims (X,Y,Z≥1), spacing (XStep,YStep,ZStep or 1.0), origin (0,0,0), Micrometer | `Class 1 Analytical Oracle` (2D defaults) + `3D Multi-Slice CTF` (both ZStep variants) |
| 4  | (a) Preflight | Per-column array creation with int32/float32 type dispatch from `CtfFields` | `Class 1 Analytical Oracle` (all 9 cell arrays exist with right types) |
| 5  | (a) Preflight | Ensemble AM sized `phases + 1`; CrystalStructures/LatticeConstants/MaterialName actions | `Class 1 Analytical Oracle` (3 tuples) + `3D Multi-Slice CTF` (2 tuples) |
| 6  | (a) Preflight | Scan/phase preflight-info values (incl. Z line when ZCells > 1) | *Exercised implicitly by every preflight; display-only values, not asserted.* |
| 7  | (b) Execute | `readFile()` error → passthrough (with error-code fallback) | `Zero Step (-102)` (fallback path), `Truncated Data (-105)`, `Unknown Column (-107)` *(other reader codes are the same return statement)* |
| 8  | (b) Execute | `phases.empty()` → `-19600` | `Empty Phases rejected (-19600)` |
| 9  | (b) Execute | All-slot Invalid-Phase default initialization | `Class 1 Analytical Oracle` (slot 0). *Uncovered-slot case is unreachable for CTF — CtfReader assigns indices 1..N contiguously.* |
| 9b | (b) Execute | phase index `>= numTuples` → `-19605` | *Not directly tested. Only reachable if the file gains a phase between the execute-time preflight and the algorithm's re-read — requires file-mutation injection inside a single execute call. Mirrors ReadAngDataFilter's untested `-19504`.* |
| 10 | (b) Execute | Per-phase fill: Laue→structure index, name, lattice constants | `Class 1 Analytical Oracle` (2 phases, 2 Laue classes) |
| 11 | (c) Copy | reader element count `< totalCells` → `-19603` | `Reader/Geometry Cell Count Mismatch rejected (-19603)` — static `ZCells 0` fixture trips it deterministically |
| 12 | (c) Copy | missing data column → `-19601` | `Missing Data Column rejected (-19601)` |
| 13 | (c) Copy | phase value out of `[0, phases]` → `-19602` | `Out-of-Range Phase Value rejected (-19602)` |
| 14 | (c) Copy | Phase verbatim copy (Phase 0 preserved) | `Class 1 Analytical Oracle` (points 2, 5) |
| 15 | (c) Copy | Euler interleave `3i/3i+1/3i+2` | `Class 1 Analytical Oracle` (18 values) |
| 16 | (c) Copy | Hex-alignment +30° branch (Hexagonal_High points only, option on) | `Euler Conversion Combinations` (points 0/3 shifted; cubic and Phase-0 points pinned unshifted) |
| 17 | (c) Copy | Degrees→radians branch | `Euler Conversion Combinations` (both radians combos) |
| 18 | (c) Copy | 7 verbatim pass-through columns (Bands, Error, MAD, BC, BS, X, Y) | `Class 1 Analytical Oracle` (element-wise) |
| 19 | (c) Copy | Multi-slice data volume (x·y·z cells) | `3D Multi-Slice CTF` (8 cells across 2 slices, element-wise) |
| 20 | —  | SIMPL 6.4/6.5 parameter conversion | `SIMPL Backwards Compatibility` |
| 21 | (b)/(c) | Cancel checks (3 sites) | *Not directly tested. Requires cancel-signal injection; standard early-return pattern. Excluded from scope by direction.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ReadCtfDataFilter: Class 1 Analytical Oracle` | new-for-V&V | Class 1 + 4. Geometry (dims/spacing/origin/units), 9 cell arrays and 3 ensemble arrays asserted element-wise (~80 assertions), all expected values hand-derived, float32-exact fixture. Inline data. |
| `…: Euler Conversion Combinations` | new-for-V&V | Class 1. DYNAMIC_SECTION over the 2×2 conversion-option grid; NumPy-derived correctly-rounded literals; pins hex shift scoping (never Phase-0/cubic points). |
| `…: 3D Multi-Slice CTF` | new-for-V&V | Class 1. DYNAMIC_SECTION (ZStep 0.75 / absent). Regression pin for D4 (3D support restored this pass). |
| `…: Empty Phases rejected (-19600)` | new-for-V&V | Value-add guard. Legacy segfaults on this fixture (demonstrated, D3). |
| `…: Missing Data Column rejected (-19601)` | new-for-V&V | Value-add guard. Legacy segfaults on this fixture (demonstrated, D3). |
| `…: Out-of-Range Phase Value rejected (-19602)` | new-for-V&V | Value-add guard. Legacy silently succeeds with heap-dependent output (demonstrated, D3). |
| `…: Reader/Geometry Cell Count Mismatch rejected (-19603)` | new-for-V&V | Value-add guard. Static `ZCells 0` fixture — no file-mutation injection needed. |
| `…: Invalid Cells Preflight Error (-19604)` | new-for-V&V | Value-add preflight rejection (`XCells 0`). |
| `…: EbsdLib Error Passthrough - Zero Step (-102)` | new-for-V&V | Exercises the error-code fallback (reader sets message only). |
| `…: EbsdLib Error Passthrough - Truncated Data (-105)` | new-for-V&V | Premature-EOF return point. |
| `…: EbsdLib Error Passthrough - Unknown Column (-107)` | new-for-V&V | Column-allocation return point. |
| `…: SIMPL Backwards Compatibility` | new-for-V&V | DYNAMIC_SECTION over the existing 6.4/6.5 conversion fixtures — the filter previously had **no** conversion test despite shipping the fixtures. |
| *(retired)* `OrientationAnalysis::ReadCtfData: Valid Execution` | retired | Exemplar comparison against `6_6_read_ctf_data.dream3d` — **legacy-generated oracle** (forbidden). Replaced by the Class 1 oracle above. |

## Exemplar archive

- **Archive:** None. **`6_6_read_ctf_data_2.tar.gz` retired** this pass (SHA512 was `f397fa3b…ad1707`); its `download_test_data()` entry is removed from `test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/6_6_read_ctf_data_2.md` — documents the legacy-oracle status and the inline replacement.
- All current oracle data is inline in `test/ReadCtfDataTest.cpp`; there is nothing to archive.

## Deviations from DREAM3D 6.5.171

*Comparison run 2026-07-24 against the official DREAM3D 6.5.171 release on four byte-identical-input fixtures (toy ×2 conversion combos, Cugrid 550×400 production scan, 2×2×2 multi-slice toy) plus three malformed-input fixtures. All numeric outputs bit-identical outside the entries below. Full write-ups: `vv/deviations/ReadCtfDataFilter.md`; working artifacts archived to OneDrive (comparison working folder).*

- `ReadCtfDataFilter-D1` — Unindexed (Phase 0) points: legacy remaps to phase 1, SIMPLNX preserves 0 (deliberate, PR #937). 116,050 points on the production fixture. Trust SIMPLNX.
- `ReadCtfDataFilter-D2` — Consequence of D1: legacy applies the hexagonal +30° φ2 shift to unindexed points; SIMPLNX never does. Exactly 0.5235988 rad on affected values. Trust SIMPLNX.
- `ReadCtfDataFilter-D3` — Malformed-input behavior: legacy segfaults (missing column; empty phases — both demonstrated, exit 139) or silently emits heap-dependent output (out-of-range phase value — demonstrated, exit 0); SIMPLNX rejects with −19600…−19604. Trust SIMPLNX.
- `ReadCtfDataFilter-D4` — *(retired)* Multi-slice `.ctf` silently truncated to slice 0 by earlier DREAM3D-NX releases; fixed this pass, now bit-identical to legacy 3D output. Trust SIMPLNX (current).
