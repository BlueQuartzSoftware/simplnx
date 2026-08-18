# Deviations from DREAM3D 6.5.171: MultiThresholdObjectsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`MultiThresholdObjectsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Filter UUID

`4246245e-1011-4add-8436-0af6bed19228`

## Headline

**4 deviations documented: 3 bugs (all in SIMPLNX, all fixed by this PR), 1 confirmed non-bug capability difference.** Legacy comparison has been **run**: an independent three-way A/B — DREAM3D 6.5.171 `PipelineRunner`, this branch's `nxrunner`, and an independent numpy oracle — on a shared 100-tuple fixture, covering representative flat (`Threshold Objects`), nested, and inverted-nested (`Threshold Objects (Advanced)`) configurations, re-run again at 50M tuples. Post-fix, all three sources MATCH in every case at both scales. The in-repo `MultiThresholdObjectsTest.cpp` suite (13 `TEST_CASE`/`TEMPLATE_TEST_CASE` declarations / 30 ctest entries — see the V&V report's Test inventory for the count basis) also passes locally.

The same three pipelines run against `develop` (pre-fix) reproduce two real bugs quantitatively: `MultiThresholdObjectsFilter-D1` (38/100 tuples wrong) and `MultiThresholdObjectsFilter-D2` (51/100 tuples wrong). A third bug, `MultiThresholdObjectsFilter-D4`, is a floating-point comparison-precision defect (raw `std::less`/`std::greater`/`std::equal_to`/`std::not_equal_to` applied directly to `float`/`double` operands, unsafe near or at threshold boundaries) — not directly exercised by the `AB1`–`AB3` fixtures, so it isn't independently quantified against legacy the way D1/D2 are. All three are **fixed by this PR** — not by a commit that predates this V&V pass. None of D1, D2, or D4 has a dedicated regression test in the in-repo `TEST_CASE` suite yet (see the V&V report's Code path coverage row 25 and Test inventory "Missing" note) — status should not promote past DRAFT until at least D1's trigger shape has one.

`MultiThresholdObjectsFilter-D3` documents a confirmed, deliberate capability difference (not a bug): multi-component index selection only exists in SIMPLNX.

---

## Comparison method

| | |
|---|---|
| **Comparison type** | Runtime three-way A/B: legacy DREAM3D 6.5.171 (`PipelineRunner`) vs. this branch (`nxrunner`) vs. an independent numpy oracle |
| **Shared input** | Legacy-format fixture, 100 tuples: `Int32 = 0..99`, `Float32 = 0.01*(i+1)` |
| **Scale re-run** | Same three configurations (AB1–AB3) re-run at 50M random tuples — this branch matches the numpy oracle exactly at scale |
| **In-repo regression suite** | `test/MultiThresholdObjectsTest.cpp` passes locally at the verified commit (13 `TEST_CASE`/`TEMPLATE_TEST_CASE` declarations / 30 ctest entries; see the V&V report's Test inventory for the count basis) |

### Per-configuration result (100-tuple fixture, this branch = post-fix)

| Case | Config | Legacy filter | This branch vs. legacy vs. oracle |
|---|---|---|---|
| `AB1` | `Int32 > 42 AND Float32 < 0.70` (flat) | `MultiThresholdObjects` ("Threshold Objects") | **MATCH** (legacy = NX = oracle) |
| `AB2` | `Int32 > 20 AND (Float32 < 0.60 OR Int32 == 55)` (nested set) | `MultiThresholdObjects2` ("Threshold Objects (Advanced)") | **MATCH** |
| `AB3` | `Int32 < 80 OR NOT(Int32 > 30 AND Float32 < 0.95)` (inverted nested set) | `MultiThresholdObjects2` | **MATCH** |

### Pre-fix (`develop`) result, same three pipelines

| Case | Result on `develop` (pre-fix) | Deviation |
|---|---|---|
| `AB1` | Matches legacy/oracle (the flat config was never affected — see D1/D2 root causes) | none |
| `AB2` | **All-false mask — 38/100 tuples wrong** | `MultiThresholdObjectsFilter-D1` |
| `AB3` | **Differs in 51/100 values** | `MultiThresholdObjectsFilter-D2` |

Both bug-fix claims in the PR are real, and the fix restores legacy semantics: legacy's `invertThreshold()` flips values element-wise; the old SIMPLNX `std::reverse` was a misport of that, and the old per-item functor dispatch broke nested-set combination entirely. (Engineer's account.)

---

## MultiThresholdObjectsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D1` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | fixed by this PR |

**Symptom:** On `develop`, an `ArrayThresholdSet` whose children mix at least one leaf `ArrayThreshold` with at least one nested `ArrayThresholdSet` (e.g. `AB2`: `{leaf: Int32 > 20, nestedSet: (Float32 < 0.60 OR Int32 == 55)}`) produced an all-false mask, regardless of input data. Quantified on the `AB2` fixture: **38 of 100 tuples wrong** (all forced false) vs. legacy `Threshold Objects (Advanced)` and the numpy oracle.

**Root cause:** Bug (SIMPLNX-side, `develop`). Per the reporting engineer: the old per-item functor dispatch broke nested-set combination entirely — a redundant inversion/apply mechanism was threaded separately from each node's own `isInverted()`, combined with a dual apply strategy (direct-copy for the first item in a set vs. an AND/OR combine for later items against an accumulator pre-filled false) that this PR replaced with a single, consistent combination path. The exact instruction-level trace of why this specifically zeroed the mask for the mixed-leaf/nested-set shape on `develop` was not independently re-derived line-by-line for this report; the symptom and fix are established by the `AB2` runtime comparison against real legacy output, which is stronger evidence than a re-derived trace would be.

**Affected users:** Any pipeline (SIMPLNX-native, or converted from legacy `Threshold Objects (Advanced)`) using a threshold set that combines a plain leaf comparison with a sibling nested group — a common shape, not an exotic edge case. Silent on `develop`: the filter reported success and wrote a fully-false mask with no warning.

**Recommendation:** Trust SIMPLNX (this PR — confirmed bit-for-bit against legacy on `AB2` at both 100 tuples and 50M tuples). The `develop` output was unconditionally wrong; anyone on a `develop` build predating this PR should upgrade and re-verify any pipeline outputs generated before the fix.

---

## MultiThresholdObjectsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D2` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | fixed by this PR |

**Symptom:** On `develop`, a leaf combined with an inverted nested set (`AB3`: `{leaf: Int32 < 80, invertedNestedSet: NOT(Int32 > 30 AND Float32 < 0.95)}`) produced incorrect mask output. Quantified on the `AB3` fixture: **51 of 100 values differ** vs. legacy `Threshold Objects (Advanced)` and the numpy oracle. `AB3`'s shape overlaps with `D1`'s mixed-leaf/nested-set trigger, so this result is not a clean isolation of the inversion defect alone — both mechanisms plausibly contribute to the discrepancy.

**Root cause:** Bug (SIMPLNX-side, `develop`). Per the reporting engineer: legacy's `invertThreshold()` flips mask values element-wise; the `develop` code instead called `std::reverse()` on the intermediate result buffer under certain replace/invert conditions — reversing the *order* of elements rather than flipping each element's own TRUE/FALSE value. This is not a valid implementation of per-element boolean inversion. This PR fixed it by consolidating all inversion through a single, consistent per-element-flip combination path (visible today as `InsertThreshold`'s `if(inverse) { newValue = !newValue; }`). The precise nesting depth at which the `develop` `std::reverse` branch was reachable (top-level only, or also for a nested child, as in `AB3`) was not independently re-derived line-by-line for this report; documented here on the `AB3` runtime evidence.

**Affected users:** Any pipeline using an inverted `ArrayThresholdSet` — top-level or nested — combined with sibling thresholds/sets. Not a narrow edge case: "Invert Mask" is a standard, documented option. On an image geometry, wrong tuple correspondence scrambles which voxels are masked; there is no legitimate downstream use of the `develop` output.

**Recommendation:** Trust SIMPLNX (this PR — confirmed bit-for-bit against legacy on `AB3` at both 100 tuples and 50M tuples). Anyone on a `develop` build predating this PR using an inverted threshold set should upgrade and re-verify any pipeline outputs generated before the fix.

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

## MultiThresholdObjectsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D4` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | fixed by this PR |

**Symptom:** On `develop`, threshold comparisons (`>`, `<`, `==`, `!=`) against a floating-point input array could give an incorrect result when an input value was very close to, or logically should have been exactly equal to, the threshold value. A value that should compare as "equal" could instead evaluate as `>` or `<` (or vice versa), and `==`/`!=` in particular were unreliable near boundary values — an ordinary consequence of comparing floating-point numbers without tolerance.

**Root cause:** Bug (SIMPLNX-side, `develop`). Per the reporting engineer: `develop`'s comparison logic applied `std::less<>`, `std::greater<>`, `std::equal_to<>`, and `std::not_equal_to<>` directly to floating-point operands, which perform exact bit-for-bit comparison — not safe for floating-point precision, since two values that are mathematically equal (or intended to be) routinely differ in their low-order bits due to representation and accumulated rounding error. This PR fixes it by adding `OperatorLess`/`OperatorGreater`/`OperatorEqual`/`OperatorNotEqual` wrapper structs (`Algorithms/MultiThresholdObjects.cpp`) built on a shared tolerance check, `CheckEquality(a, b) = std::fabs(a - b) < std::numeric_limits<float32>::epsilon()`: `OperatorEqual`/`OperatorNotEqual` now use `CheckEquality` instead of exact equality, and `OperatorGreater`/`OperatorLess` explicitly exclude the near-equal band (`(value1 > value2) && !CheckEquality(value1, value2)`), so a value within epsilon of the threshold is never simultaneously reported as both "not equal" and "wrongly ordered."

**Affected users:** Any pipeline thresholding a floating-point array where the input data or threshold is close to, or intended to exactly match, a boundary value — most commonly when thresholding on a computed/derived float array (e.g. output of an upstream arithmetic filter) where an intended-exact match doesn't land on the identical bit pattern. Most likely to matter for `==`/`!=` comparisons and near-boundary `<`/`>` comparisons; low impact for thresholds far from any input value.

**Affected legacy comparison:** Unlike D1/D2, this fix is **not confirmed** to restore or preserve legacy semantics — whether DREAM3D 6.5.171's own comparison implementation uses exact or tolerant floating-point comparison has not been checked, and none of `AB1`–`AB3`'s threshold values are close enough to an input value to exercise the epsilon-tolerance branch either way. It is documented here as a genuine SIMPLNX-side correctness fix on its own terms, independent of the legacy comparison.

**Recommendation:** Trust SIMPLNX (this PR). Epsilon-tolerant floating-point comparison is the technically correct approach regardless of what legacy does. A dedicated near-boundary A/B fixture (input value within float32 epsilon of the threshold, deliberately not bit-identical) is recommended to confirm whether this also closes or opens a gap with legacy — see "Outstanding comparison work" below.

---

## Outstanding comparison work

Both legacy filters have now been run separately on representative configurations (`AB1` vs. `Threshold Objects`; `AB2`/`AB3` vs. `Threshold Objects (Advanced)`), satisfying this filter's Rewrite-classification requirement that functional equivalence be independently confirmed against both predecessors, not just one. Remaining lower-priority gaps:

1. **Custom TRUE/FALSE mask output values** (`#669` addition) — not exercised by `AB1`–`AB3`. Compare with it left at legacy defaults first, then with custom values set.
2. **Default mask output `DataType`** — SIMPLNX defaults to `uint8` (`#1502`); confirm what each legacy filter's default was and whether migration guidance is needed for pipelines that relied on the default rather than explicitly setting it.
3. **D4's near-boundary floating-point comparison** — a dedicated A/B fixture with an input value within `float32` epsilon of the threshold (but not bit-identical) is needed to determine whether legacy's own comparison is exact or tolerant, and therefore whether D4 opens or closes a legacy gap. `AB1`–`AB3` don't exercise this.
4. **A broader configuration sweep** beyond the three representative `AB1`–`AB3` shapes (e.g., deeper nesting, mixed AND/OR at multiple levels) is optional given the strong quantitative match already obtained at both 100-tuple and 50M-tuple scale, but would further reduce residual risk before COMPLETE status.
