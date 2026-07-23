# Deviations from DREAM3D 6.5.171: MultiThresholdObjectsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`MultiThresholdObjectsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## Status: comparison not yet run

No local build of legacy DREAM3D 6.5.171 is available in this environment. No runtime A/B has been performed against **either** legacy predecessor:

- **Threshold Objects** (`MultiThresholdObjects`, SIMPL UUID `014b7300-cf36-5ede-a751-5faf9b119dae`) — flat, AND-only comparison list
- **Threshold Objects (Advanced)** (`MultiThresholdObjects2`, SIMPL UUID `686d5393-2b02-5c86-b887-dd81a8ae80f2`) — nested AND/OR comparison sets

**0 deviation entries exist as of this DRAFT.**

Per `vv_policy.md`'s one ordering rule ("pick the oracle before running any DREAM3D 6.5.171 comparison"), the V&V report's Class 1 (Analytical) oracle already establishes SIMPLNX correctness independently of legacy — see `vv/MultiThresholdObjectsFilter.md`. The legacy A/B remains outstanding and is required before this filter's status can move past DRAFT; it is not a precondition for the oracle work already done.

**Because the Algorithm Relationship is classified as Rewrite** (this filter consolidates two independently-shipped legacy filters into one — see the V&V report), the burden here is higher than a straight port: the eventual comparison must independently confirm output equivalence against **both** legacy filters, run separately —

1. Configurations expressible in the legacy flat (AND-only) model, compared against **Threshold Objects**.
2. Configurations using nested AND/OR sets or per-set invert, compared against **Threshold Objects (Advanced)**.

A clean result on only one of the two legacy filters is not sufficient to close this Rewrite's defense — both source filters must be reconciled before the merged UUID's functional-equivalence claim can be considered verified.

## Filter UUID

`4246245e-1011-4add-8436-0af6bed19228`

## Other configurations to prioritize once a legacy build is available

Beyond running the two legacy comparisons described above, these individual parameter additions are the other likely sources of drift — not observed deviations, just a prioritized test plan for the eventual A/B:

1. **Multi-component index selection** (`#1184` addition) — not present in legacy `Threshold Objects`; unconfirmed whether `Threshold Objects (Advanced)` had an equivalent. Comparison is only meaningful against whichever legacy filter (if either) has this feature.
2. **Custom TRUE/FALSE mask output values** (`#669` addition) — compare with it left at legacy defaults first, then with custom values set.
3. **Default mask output `DataType`** — SIMPLNX defaults to `uint8` (`#1502`); confirm what each legacy filter's default was and whether any migration guidance is needed for pipelines that relied on the default rather than explicitly setting it.

## Entries

No entries yet. When a comparison surfaces an actual behavioral difference, add it here following the stable-ID convention (`MultiThresholdObjectsFilter-D1`, `-D2`, …) with fields: Deviation ID, Filter UUID, Status, Symptom, Root cause (`bug` | `precision` | `order of operations` | `library` | `algorithmic choice`), Affected users, Recommendation.
