# Deviations from DREAM3D 6.5.171: ErodeDilateBadDataFilter

Entries use stable IDs (`ErodeDilateBadDataFilter-D<N>` for legacy deviations, `ErodeDilateBadDataFilter-B<N>` for SIMPLNX-side bugs).

---

## Headline: No legacy comparison has been performed

The [V&V report](../ErodeDilateBadDataFilter.md) for this filter uses a **Class 1 (Analytical) oracle only** — expected outputs are hand-traced against a small synthetic dataset, independent of any DREAM3D 6.5.171 run. No pipeline was executed in legacy DREAM3D 6.5.171 to produce a reference `.dream3d` file, and the legacy `ErodeDilateBadData` C++ source (`Source/Plugins/Processing/ProcessingFilters/ErodeDilateBadData.{h,cpp}`) is not present in this repository, so no source-level diff was possible either.

Consequently, this file records **no confirmed deviations** — not because none exist, but because the comparison that would surface them has not been done. This is a gap, not a clean bill of health.

## What would need to happen to fill this in

1. Obtain or build a DREAM3D 6.5.171 binary (available locally at `C:\Users\holym\BlueQuartz\Builds\DREAM3D\DREAM3D-6.5.171-Win64` on this machine) and run an `ErodeDilateBadData` pipeline against a shared input dataset, in both Erode and Dilate modes, covering at least one case where direction restriction actually changes the result (see the V&V report's note that the current Class 1 fixture is direction-invariant for all 7 combinations it exercises).
2. Compare the legacy output against SIMPLNX output on the same input, using the same comparison discipline as other filters in this plugin (`UnitTest::CompareExemplarToGeneratedData` or equivalent element-wise check).
3. If legacy source becomes available for reference, diff the neighbor-selection, vote/tie-break, and direction-masking logic (`adjustValidNeighbors` in `Algorithms/ErodeDilateBadData.cpp`) against it directly — this is the one piece of the current implementation flagged for second-engineer scrutiny in the V&V report, precisely because the tie-break/direction-masking behavior could not be corroborated against a reference.

## Non-deviations (documented for awareness)

### Legacy tie-break language says "chosen randomly"; SIMPLNX is deterministic

The SIMPLNX filter markdown (`docs/ErodeDilateBadDataFilter.md`), which reads as carried over from legacy documentation, states that erode ties are broken "randomly." The current SIMPLNX implementation is deterministic: the first-processed neighbor (by `faceNeighborInternalIdx` order, `[-Z,-Y,-X,+X,+Y,+Z]`) wins ties, since a later neighbor's vote must strictly exceed the current maximum to replace it. Whether legacy DREAM3D 6.5.171 was actually nondeterministic (e.g., using an RNG) or merely used "random" loosely to mean "implementation-defined scan-order" has not been verified against legacy source. Recorded here as a documentation-language discrepancy worth resolving once legacy source or a legacy binary comparison is available, not asserted as a behavioral deviation.
