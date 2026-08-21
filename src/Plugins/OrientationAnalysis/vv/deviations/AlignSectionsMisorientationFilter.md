# Deviations from DREAM3D 6.5.171: AlignSectionsMisorientationFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its
DREAM3D 6.5.171 equivalent (`AlignSectionsMisorientation`, legacy UUID
`{4fb2b9de-3124-534b-b914-dbbbdbc14604}`).

Entries are referenced by stable ID (`AlignSectionsMisorientationFilter-D<N>`) from the V&V
report. The Filter UUID field is the permanent cross-reference anchor.

The A/B comparison behind these entries is described in
`vv/AlignSectionsMisorientationFilter.md` (§Deviations). Headline: across four fixtures and
eight runs, **every cell array was element-for-element identical and every shift value was
identical** — 95 checks, 0 failures, with no tolerance of any kind in the comparison predicate.
Log: `ww_work/AlignSectionsMisorientation/ab/ab_comparison_results.txt`; predicate:
`ww_work/AlignSectionsMisorientation/ab/compare_ab.py`. Only D1 below was observed as an actual
output difference; the rest are interface, precision-bound, or guard differences established by
source reading and, where noted, by targeted fixtures.

---

## AlignSectionsMisorientationFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D1` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active |

**Symptom:** The alignment shifts are delivered as three DataArrays in a new Attribute Matrix
instead of as a tab-separated text file, and the arrays carry one extra leading tuple that has
no counterpart in the legacy file.

**Root cause:** *algorithmic choice.* Introduced deliberately by simplnx PR #1237 ("Alignment
Filters Modernization"), which also bumped the filter's parameters version 1 -> 2. Legacy
writes one row per section pair, `slice \t slice+1 \t relX \t relY \t cumX \t cumY`, with no
header and no row for the anchor section. SIMPLNX writes `Slice Indices` (uint32 x2),
`Relative Shifts` (int64 x2) and `Cumulative Shifts` (int64 x2), each with one tuple per
section. The values map one-to-one: **legacy text row `r` (0-based) corresponds to SIMPLNX
tuple `r + 1`**. SIMPLNX tuple 0 describes no section pair and is all zeros.

**Affected users:** Anyone with a script that parses the legacy shift text file. The data is
all still available, but it must be read from the DataStructure (or exported to CSV) rather
than from a side-channel file. Users reading `Slice Indices` tuple 0 could misread `{0, 0}` as
"section 0 aligned to section 0"; the filter documentation now calls this out.

**Recommendation:** Trust SIMPLNX. Keeping the shifts in the DataStructure makes them
inspectable and usable by downstream filters instead of stranding them in a file. The one-tuple
offset is documented in the filter's markdown.

---

## AlignSectionsMisorientationFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D2` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active |

**Symptom:** The misorientation angle that the shift search thresholds is computed in double
precision by a different formula than legacy used, so the two angles differ in their low bits.
No output difference was observed.

**Root cause:** *precision / library.* Legacy holds orientations as `QuatF` (float) and, for
Cubic_High, finishes with `2*acos(w)` in float. SIMPLNX promotes to `QuatD` and calls
EbsdLib 3.1.0, whose `CubicOps::calculateMisorientationInternal` finishes with
`2*atan2(|v|, w)` in double; the returned double is then narrowed back to float before the
comparison. Non-cubic Laue classes still use `2*acos(w)` on both sides. The angle feeds
**only** the boolean `angle > misorientationTolerance`, so the shifts change only if a sampled
pair's disorientation sits within the two formulas' disagreement of the threshold — bounded by
roughly 9e-4 rad near 0 degrees (where `acos` loses precision that `atan2` keeps) and roughly
1e-7 rad elsewhere.

**Affected users:** Nobody at realistic tolerances. Reachable only with a tolerance below
about 0.05 degrees, or on a dataset where a sampled pair's disorientation lands within about
1e-3 rad of the chosen tolerance. Because the search is a discrete argmin, the consequence
even then is a shift differing by one voxel on one section, not a gradual error.

**Recommendation:** Trust SIMPLNX. The double-precision `atan2` form is strictly better
conditioned; legacy's float `acos` reported spurious non-zero angles for orientation pairs
that are exactly symmetry-equivalent.

---

## AlignSectionsMisorientationFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D3` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active |

**Symptom:** The degrees-to-radians conversion of the tolerance parameter can differ from
legacy's in the last representable bit.

**Root cause:** *precision.* Legacy computes `m_MisorientationTolerance * k_Pif / 180.0f`
entirely in float. SIMPLNX computes `float(tolerance * (pi_double / 180.0))` — a double
product narrowed once. The two results can differ by one ULP. Since the comparison is a strict
`>`, this can only change an outcome for a pair whose disorientation equals the tolerance to
within that ULP.

**Affected users:** Nobody in practice. Requires a sampled pair's disorientation to coincide
with the tolerance to about 1e-7 rad.

**Recommendation:** Either acceptable within tolerance, where the tolerance is **one ULP of the
float tolerance value** (about 1e-7 rad at the tolerances users actually set) — the two conversions
cannot differ by more than that, and the value only feeds a strict `>` comparison. The
single-narrowing form is marginally more accurate.
Deliberately **not** asserted by the test suite: pinning behaviour exactly at the strict-`>`
boundary would encode float noise as a contract. The oracle fixtures are all kept at least
5 degrees away from the tolerance in use.

---

## AlignSectionsMisorientationFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D4` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active |

**Symptom:** SIMPLNX accepts a `uint8` mask array; legacy accepts only `bool`.

**Root cause:** *algorithmic choice (superset).* Legacy hard-types the mask as
`DataArray<bool>` both in the parameter requirement and in `dataCheck`. SIMPLNX routes the
mask through `MaskCompareUtilities::InstantiateMaskCompare`, which accepts `bool` or
`uint8` (non-zero is true). Every input legacy accepts is still accepted with identical
semantics.

**Affected users:** Nobody negatively. Users whose mask came from a filter that emits `uint8`
no longer need a conversion step.

**Recommendation:** Trust SIMPLNX. Strict superset of legacy behaviour. Not exercised in the
A/B because the legacy side cannot read a `uint8` mask at all; the widening is covered by the
SIMPLNX oracle suite through the `bool` path and by the mask utility's own tests.

---

## AlignSectionsMisorientationFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D5` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active |

**Symptom:** SIMPLNX rejects, or warns about, **five** classes of input that legacy either
accepted silently or reported differently — four errors and one warning, enumerated in the table
below.

**Root cause:** *bug (in both, fixed in SIMPLNX)* for `-68008`, which closes an out-of-bounds read
present in both implementations, plus *algorithmic choice* for the other four, which are added
defensive guards with no legacy counterpart in that exact form. Added during this V&V pass:

| Code | Condition | Legacy behaviour |
|---|---|---|
| -68005 | any geometry dimension <= 1 | Legacy had the same guard as `-3010`, but SIMPLNX had lost it in the port. **Restored**, with a different code, and — unlike legacy — with an early return. Legacy set the error condition without returning, so a later `dataCheck` failure could overwrite the reported code. |
| -68006 | a selected cell array's tuple count differs from the selected geometry's cell count | Not possible to express in legacy, which fetched the arrays from the geometry's own attribute matrix. Reachable in SIMPLNX because array selection is free-form, and it led to out-of-bounds reads during execute. |
| -68007 | negative misorientation tolerance | Legacy accepted it and produced meaningless shifts (every pair, including identical orientations, counts as a mismatch). |
| -68008 | a phase value at or above the crystal-structure ensemble tuple count | Legacy read out of bounds (`m_CrystalStructures[m_CellPhases[pos]]` behind only a `> 0` test). Shared latent defect; SIMPLNX now detects it. |
| -68009 (warning) | an indexed phase whose crystal structure is not a known Laue class | Both silently treat such cells as misoriented against everything. SIMPLNX now says so. |

**Known residual in `-68006`.** The guard checks only that each selected cell array's tuple count
equals the selected geometry's cell count; it does **not** check that the arrays are children of
that geometry's cell attribute matrix. A same-sized array belonging to a *different* geometry
therefore still passes preflight and is then indexed as though it belonged to the selected
geometry. That is silently wrong output rather than an out-of-bounds read, so the undefined
behaviour the guard was added for is closed; the parentage half is not. Logged rather than
implemented because adding a parentage requirement is a user-visible tightening of what the
selection parameters accept, which is beyond the ratified scope of this pass.

**Affected users:** Users who were previously getting silent garbage or an out-of-bounds read
from malformed input now get a specific, actionable error naming the offending values. Users
with well-formed input see no change — confirmed by the A/B, where no guard fired on either
side, and by the retained legacy-parity exemplar test.

**Recommendation:** Trust SIMPLNX. These convert undefined behaviour and silent nonsense into
diagnostics. Note that a workflow which previously "succeeded" on a degenerate 2D geometry will
now stop with -68005; that workflow was producing a no-op, and legacy would have rejected it
too.

---

## AlignSectionsMisorientationFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D6` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active |

**Symptom:** Legacy's base class had an `IgnoredDataArrayPaths` member for excluding arrays
from the shift transfer. SIMPLNX has no equivalent and always transforms every array in the
geometry's cell attribute matrix.

**Root cause:** *algorithmic choice (feature not ported).* The legacy member had no
`FilterParameter` entry, so it was unreachable from a JSON pipeline and was always empty in
any non-GUI run. **Both implementations therefore shift every cell array in practice**, which
is why the A/B's element-wise comparison of Quats, Phases and Mask matched.

**Affected users:** Nobody using pipelines. Legacy GUI users who had somehow populated the
member would see a difference; the parameter was never exposed in the 6.5.171 misorientation
filter's GUI either.

**Recommendation:** Either acceptable, and the applicable tolerance is **zero** — because the
legacy member was always empty in any pipeline run, both implementations shift exactly the same set
of arrays and the A/B compared them element for element with no tolerance at all. Nothing to
restore.

---

## AlignSectionsMisorientationFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D7` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active — documented, not fixed |

**Symptom:** The candidate-memoization array is read before the bounds test that would
establish the index is in range, so a shift approaching half the slice width causes an
out-of-bounds read.

**Root cause:** *bug, shared with 6.5.171.* The acceptance test is
`if(!misorients[idx] && llabs(k + oldxshift) < halfDim0 && llabs(j + oldyshift) < halfDim1)`,
which appears twice in `AlignSectionsMisorientation::findShifts` — once in each duplicated copy of
the candidate scan (recording copy `:203`, non-recording copy `:311` at the head of this branch;
legacy `:304`). `idx` is computed from the unvalidated candidate two lines earlier, so
`misorients[idx]` is evaluated first. Legacy reads past the end of a raw
`bool*`; SIMPLNX reads past the end of a `std::vector<bool>`. A truthy garbage read silently
skips an otherwise legal candidate, which makes both implementations non-reproducible in that
regime.

**Affected users:** Only workflows whose sections need shifting by nearly half the slice
width, which on realistic EBSD data would mean the sections barely overlap. Trivially
reachable on very small volumes.

**Recommendation:** Either acceptable — the two implementations agree exactly (zero tolerance
required) everywhere the behaviour is defined, and they differ only in the out-of-bounds regime
where neither has defined behaviour to preserve. The reordering is safe on in-bounds data but is a
behaviour change in that regime, so it was deliberately left unfixed in this V&V pass to keep the
pass free of unproven behaviour changes; logged as a follow-up. All V&V
fixtures are sized 32x32, so `misorients` holds 1024 entries and `halfDim0 == halfDim1 == 16`. The
largest shift any fixture reaches is **4** voxels (the multi-hop fixture's `d = (4, 0)`; every other
fixture peaks at 3), and the largest memoization index any fixture actually computes is **688** —
reached by the shift-accumulation fixture's second section pair, whose search re-centres on
`(-3, 2)` and so evaluates `idx = 32*(2 + 3 + 16) + (-3 + 3 + 16) = 688`. The next largest are 631
(multi-hop, re-centred on `(4, 0)`) and 629 (the `d = (2, 0)` fixtures). All are comfortably inside
the 1024-entry array and clear of this regime.

---

## AlignSectionsMisorientationFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsMisorientationFilter-D8` |
| **Filter UUID** | `8df2135c-7079-49f4-9756-4f3c028a5ced` |
| **Status** | active — legacy-only defect, nothing to fix in SIMPLNX |

**Symptom:** When a legacy run is cancelled mid-search, the shift text file is left open and
unflushed, so buffered rows are lost and the file is truncated.

**Root cause:** *bug in 6.5.171.* Legacy returns early on cancel (`:276-279`) without closing
the `std::ofstream` opened at `:226-230`. SIMPLNX has no such exposure: the shifts live in
DataArrays and cancellation simply leaves them at their initialized values.

**Affected users:** Legacy users who cancelled a long alignment run and then trusted the
partial shift file.

**Recommendation:** Trust SIMPLNX. Recorded because it is a real difference in cancellation
behaviour that a migrating user might otherwise attribute to SIMPLNX.
