# Deviations from DREAM3D 6.5.171: MultiThresholdObjectsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`MultiThresholdObjectsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Filter UUID

`4246245e-1011-4add-8436-0af6bed19228`

## Headline

**3 deviations documented: 2 bugs (both in SIMPLNX, both fixed pre-branch), 1 confirmed non-bug capability difference.** Legacy comparison has been **run**: an independent three-way A/B — DREAM3D 6.5.171 `PipelineRunner`, this branch's `nxrunner`, and an independent numpy oracle — on a shared 100-tuple fixture, covering representative flat (`Threshold Objects`), nested, and inverted-nested (`Threshold Objects (Advanced)`) configurations, re-run again at 50M tuples. Post-fix, all three sources MATCH in every case at both scales. All 17 in-repo ctest entries also pass locally.

The same three pipelines run against `develop` (pre-fix) reproduce two real bugs quantitatively: `MultiThresholdObjectsFilter-D1` (38/100 tuples wrong) and `MultiThresholdObjectsFilter-D2` (51/100 tuples wrong). Both are fixed by commit `25f1986f1` ("Fixed MultiThresholdObjects ThresholdSets algorithm", 2026-04-23), which predates this V&V pass. Neither has a dedicated regression test in the in-repo `TEST_CASE` suite yet (see the V&V report's Code path coverage row 25 and Test inventory "Missing" note) — status should not promote past DRAFT until at least D1's trigger shape has one.

`MultiThresholdObjectsFilter-D3` documents a confirmed, deliberate capability difference (not a bug): multi-component index selection only exists in SIMPLNX.

---

## Comparison method

| | |
|---|---|
| **Comparison type** | Runtime three-way A/B: legacy DREAM3D 6.5.171 (`PipelineRunner`) vs. this branch (`nxrunner`) vs. an independent numpy oracle |
| **Shared input** | Legacy-format fixture, 100 tuples: `Int32 = 0..99`, `Float32 = 0.01*(i+1)` |
| **Scale re-run** | Same three configurations (AB1–AB3) re-run at 50M random tuples — this branch matches the numpy oracle exactly at scale |
| **In-repo regression suite** | All 17 ctest entries in `test/MultiThresholdObjectsTest.cpp` pass locally at the verified commit |

### Per-configuration result (100-tuple fixture, this branch = post-fix)

| Case | Config | Legacy filter | This branch vs. legacy vs. oracle |
|---|---|---|---|
| `AB1` | `Int32 > 42 AND Float32 < 0.70` (flat) | `MultiThresholdObjects` ("Threshold Objects") | **MATCH** (legacy = NX = oracle) |
| `AB2` | `Int32 > 20 AND (Float32 < 0.60 OR Int32 == 55)` (nested set) | `MultiThresholdObjects2` ("Threshold Objects (Advanced)") | **MATCH** |
| `AB3` | `Int32 < 80 OR NOT(Int32 > 30 AND Float32 < 0.95)` (inverted nested set) | `MultiThresholdObjects2` | **MATCH** |

### Pre-fix (`develop`) result, same three pipelines

| Case | Result on `develop` (pre-`25f1986f1`) | Deviation |
|---|---|---|
| `AB1` | Matches legacy/oracle (flat configs were never affected — see D1/D2 root causes) | none |
| `AB2` | **All-false mask — 38/100 tuples wrong** | `MultiThresholdObjectsFilter-D1` |
| `AB3` | **Differs in 51/100 values** | `MultiThresholdObjectsFilter-D2` |

Both bug-fix claims in the PR are real, and the fix restores legacy semantics: legacy's `invertThreshold()` flips values element-wise; the old SIMPLNX `std::reverse` was a misport of that, and the old per-item functor dispatch broke nested-set combination entirely. (Engineer's account, corroborated by the `25f1986f1` diff — see D1/D2 Root cause below.)

---

## MultiThresholdObjectsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D1` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | retired 2026-04-23 — fixed by commit `25f1986f1`, prior to this V&V pass |

**Symptom:** An `ArrayThresholdSet` whose children mix at least one leaf `ArrayThreshold` with at least one nested `ArrayThresholdSet` (e.g. `AB2`: `{leaf: Int32 > 20, nestedSet: (Float32 < 0.60 OR Int32 == 55)}`) produced an all-false mask, regardless of input data. Quantified on the `AB2` fixture: **38 of 100 tuples wrong** (all forced false) vs. legacy `Threshold Objects (Advanced)` and the numpy oracle.

**Root cause:** Bug (SIMPLNX-side, pre-fix). Per the reporting engineer: the old per-item functor dispatch broke nested-set combination entirely. Corroborating evidence from the `25f1986f1` diff: the pre-fix `MultiThresholdObjects.cpp` threaded a redundant `bool inverse` parameter down through recursive `ThresholdSet`/`ThresholdValue` calls (separate from each node's own `arrayThreshold.isInverted()`), and applied a dual apply strategy — the first item at any nesting level (`replaceInput == true`) took a direct-copy path while later items took an `InsertThreshold`-based AND/OR combine path against an accumulator pre-filled with `falseValue`. Commit `25f1986f1` ("Standardized apply threshold values between thresholds and sets. Removed unnecessary inversion parameter...") replaced both call sites with a single `ApplyThresholdValues` → `InsertThreshold` path. The exact instruction-level trace of why this specifically zeroed the mask for the mixed-leaf/nested-set shape was not independently re-derived line-by-line for this report; the symptom and fix are established by the `AB2` runtime comparison against real legacy output, which is stronger evidence than a re-derived trace would be.

**Affected users:** Any pipeline (SIMPLNX-native, or converted from legacy `Threshold Objects (Advanced)`) using a threshold set that combines a plain leaf comparison with a sibling nested group — a common shape, not an exotic edge case. Silent: the filter reported success and wrote a fully-false mask with no warning.

**Recommendation:** Trust SIMPLNX (current/post-fix — confirmed bit-for-bit against legacy on `AB2` at both 100 tuples and 50M tuples). The pre-fix output was unconditionally wrong; anyone on a pre-`25f1986f1` build should upgrade and re-verify any pipeline outputs generated before the fix.

---

## MultiThresholdObjectsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D2` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | retired 2026-04-23 — fixed by commit `25f1986f1`, prior to this V&V pass |

**Symptom:** A leaf combined with an inverted nested set (`AB3`: `{leaf: Int32 < 80, invertedNestedSet: NOT(Int32 > 30 AND Float32 < 0.95)}`) produced incorrect mask output. Quantified on the `AB3` fixture: **51 of 100 values differ** vs. legacy `Threshold Objects (Advanced)` and the numpy oracle. `AB3`'s shape overlaps with `D1`'s mixed-leaf/nested-set trigger, so this result is not a clean isolation of the inversion defect alone — both mechanisms plausibly contribute to the discrepancy.

**Root cause:** Bug (SIMPLNX-side, pre-fix). Per the reporting engineer: legacy's `invertThreshold()` flips mask values element-wise; the pre-fix SIMPLNX code instead called `std::reverse(tempResultVector.begin(), tempResultVector.end())` on the intermediate result buffer under certain replace/invert conditions — reversing the *order* of elements rather than flipping each element's own TRUE/FALSE value. This is not a valid implementation of per-element boolean inversion. The correct operation (applied correctly elsewhere in the same file, e.g. inside `InsertThreshold`: `newVector[i] = (newVector[i] == trueValue) ? falseValue : trueValue`) flips each element's own value in place, changing nothing about element order. Fixed by the same `25f1986f1` commit that removed the redundant `inverse` parameter and both `std::reverse` call sites, consolidating all inversion through `ApplyThresholdValues` → `InsertThreshold`'s per-element flip. The precise nesting depth at which the pre-fix `std::reverse` branch was reachable (top-level only, or also for a nested child, as in `AB3`) was not independently re-derived line-by-line for this report; documented here on the `AB3` runtime evidence plus the `25f1986f1` diff.

**Affected users:** Any pipeline using an inverted `ArrayThresholdSet` — top-level or nested — combined with sibling thresholds/sets. Not a narrow edge case: "Invert Mask" is a standard, documented option. On an image geometry, wrong tuple correspondence scrambles which voxels are masked; there is no legitimate downstream use of the pre-fix output.

**Recommendation:** Trust SIMPLNX (current/post-fix — confirmed bit-for-bit against legacy on `AB3` at both 100 tuples and 50M tuples). Anyone on a pre-`25f1986f1` build using an inverted threshold set should upgrade and re-verify any pipeline outputs generated before the fix.

---

## MultiThresholdObjectsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D3` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |

**Symptom:** SIMPLNX accepts a component index on a multi-component threshold array (`ArrayThreshold::setComponentIndex()`); neither legacy filter has an equivalent parameter.

**Root cause:** Algorithmic choice (deliberate SIMPLNX capability addition, not a port artifact — appropriate under this filter's Rewrite classification). Confirmed directly from legacy source: `MultiThresholdObjects2::dataCheck()` rejects non-scalar (multi-component) arrays outright with error `-11003`. Legacy `Threshold Objects` (the non-Advanced filter) has no per-component comparison concept either. SIMPLNX's `#1184` (`32837a30f`) added multi-component index selection with no legacy equivalent in either predecessor.

**Affected users:** Nobody migrating *from* legacy is affected (the capability didn't exist to lose). Anyone relying on this SIMPLNX-only feature should be aware there is no DREAM3D 6.5.171 equivalent pipeline to fall back to if downgrading.

**Recommendation:** Trust SIMPLNX. This is an intentional superset capability, not a correctness issue. Worth a one-line callout in public migration guidance: legacy `Threshold Objects (Advanced)` never supported per-component thresholding on multi-component arrays; SIMPLNX does.

---

## Outstanding comparison work

Both legacy filters have now been run separately on representative configurations (`AB1` vs. `Threshold Objects`; `AB2`/`AB3` vs. `Threshold Objects (Advanced)`), satisfying this filter's Rewrite-classification requirement that functional equivalence be independently confirmed against both predecessors, not just one. Remaining lower-priority gaps:

1. **Custom TRUE/FALSE mask output values** (`#669` addition) — not exercised by `AB1`–`AB3`. Compare with it left at legacy defaults first, then with custom values set.
2. **Default mask output `DataType`** — SIMPLNX defaults to `uint8` (`#1502`); confirm what each legacy filter's default was and whether migration guidance is needed for pipelines that relied on the default rather than explicitly setting it.
3. **A broader configuration sweep** beyond the three representative `AB1`–`AB3` shapes (e.g., deeper nesting, mixed AND/OR at multiple levels) is optional given the strong quantitative match already obtained at both 100-tuple and 50M-tuple scale, but would further reduce residual risk before COMPLETE status.
