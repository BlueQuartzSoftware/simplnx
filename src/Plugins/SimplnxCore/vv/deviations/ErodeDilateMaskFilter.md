# Deviations from DREAM3D 6.5.171: ErodeDilateMaskFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ErodeDilateMaskFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Output parity:** the binary A/B run for this V&V — 14 combinations ({dilate, erode} × the 7 non-empty
direction combinations, `NumIterations = 1`) on 2 fixtures, 28 pipeline pairs — produced **28/28
element-wise matches** on the `Mask` array. **No output-value deviation was observed.** The one entry
below is a validation-behaviour difference, not an output difference, and it has since been
**resolved** — as of 2026-08-19 there are **no active deviations** for this filter.

---

## ErodeDilateMaskFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateMaskFilter-D1` |
| **Filter UUID** | `cab66cd1-f64c-42b4-8f94-18f0835a967f` |
| **Status** | retired 2026-08-19 — difference eliminated in SIMPLNX by restoring the legacy guard (requester product decision, 2026-08-19). Retained for the historical record. |

**Symptom (as observed during this V&V, before the fix):** A pipeline that sets `Number of Iterations`
to zero or a negative value fails preflight in DREAM3D 6.5.171 with
`Error (-5555): The number of iterations (0) must be positive`, but ran to completion in SIMPLNX and
wrote the mask out unchanged. A user migrating such a pipeline got a silently unchanged mask where
they previously got a hard stop.

**Root cause:** Algorithmic choice — a validation guard that was not carried across in the port.
Legacy `ErodeDilateMask::dataCheck()` (`Source/Plugins/Processing/ProcessingFilters/ErodeDilateMask.cpp`,
in the `dataCheck` body) sets error condition `-5555` when `getNumIterations() <= 0`. SIMPLNX's
`ErodeDilateMaskFilter::preflightImpl` performs no equivalent check — it only calls
`MarkDataPathModified` — and `k_NumIterations_Key` is an unconstrained `Int32Parameter`. The
algorithm's `for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)` loop
then simply never executes, so the filter succeeds and the mask is untouched. Confirmed by running
both binaries on the same input with `NumIterations = 0`
(`ww_work/ErodeDilateMask/extra/`, "Extra probe" section of that folder's `ReadMe.md`).

**Affected users:** Anyone who reaches `NumIterations <= 0`, which in practice means pipelines built
by scripting or templating where the iteration count is computed rather than typed. Interactively the
value is rarely zero. The consequence is a silent no-op rather than wrong data: the mask that comes
out is bit-identical to the mask that went in, so downstream results are those of "filter not run",
not those of a corrupted run.

**Recommendation:** Trust SIMPLNX — **no remaining difference.** The V&V pass as first written left
this open, because adding a preflight error is a behavioural change to a shipping filter and therefore
a product decision rather than a verification finding. The requester made that decision on
**2026-08-19: match legacy, add the preflight error.** Rationale recorded with the decision — it
restores 6.5.171 parity, has zero migration impact (a `NumIterations <= 0` pipeline already errored in
legacy, so no working legacy pipeline can be relying on the SIMPLNX no-op), and needs no
parameter-version bump because no parameter key changes.

**Resolution (2026-08-19):** `ErodeDilateMaskFilter::preflightImpl` now returns
`MakeErrorResult<OutputActions>(k_InvalidNumIterationsError /* -14701 */, ...)` when
`NumIterations < 1`, with the offending value in the message. SIMPLNX's error code is **-14701**, not
legacy's `-5555`: `-5555` belongs to the legacy code space, and the SIMPLNX house convention is a
file-local `constexpr int32 k_...Error` in an anonymous namespace drawn from a per-filter block —
`-147xx` is the first free block adjacent to the sibling `ErodeDilateBadDataFilter`'s `-146xx`
(`k_NoDirectionsError = -14601`, `k_NoGeometryDimensionsError = -14602`). Covered by
`SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations`, which preflights `NumIterations`
of `0` and `-2` and asserts both the invalid result and the code.

---

## Confirmed non-deviations

Recorded so they are not relitigated.

### Direction parameters ignored — SIMPLNX port regression, fixed; not a deviation

Before this V&V pass, `Algorithms/ErodeDilateMask.cpp` never read `XDirOn`, `YDirOn`, or `ZDirOn`, so
the `X Direction` / `Y Direction` / `Z Direction` parameters had no effect on which face neighbours
participated. DREAM3D 6.5.171 honours all three (`ErodeDilateMask.cpp:227-247` gates each of the six
neighbour indices on the corresponding flag), so legacy was correct and SIMPLNX was wrong. Under the
bug-adjudication protocol that is a **port regression**, not a deviation from legacy behaviour: it was
fixed in the same change that found it, and once fixed no difference remains, so it gets no deviation
entry of its own. It is described in the V&V report's Summary and Bug Fixes sections. The
post-fix binary A/B (28/28 matches) is the confirmation of parity.

Same defect class as `ErodeDilateBadDataFilter-D1` (PR #1687); the fix mirrors that pattern and the
axis mapping was copied verbatim from `Algorithms/ErodeDilateBadData.cpp:64-80` rather than
re-derived, per that entry's branch-history note.

### `maskCopy` initial value

Legacy allocates `_INTERNAL_USE_ONLY_MaskCopy` and calls `initializeWithValue(false)`; SIMPLNX uses a
`std::vector<bool>(totalPoints, false)`. Neither initial value can be observed: the first statement
of every iteration copies `mask` over the whole of `maskCopy` before anything reads it. Not a
deviation.
