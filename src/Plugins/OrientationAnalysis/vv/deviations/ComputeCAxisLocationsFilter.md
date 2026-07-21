# Deviations from DREAM3D 6.5.171: ComputeCAxisLocationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ComputeCAxisLocationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## Headline: No deviations observed

The legacy comparison used the inline data **"OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle"**: These inline quaternion values were encoded into a csv file. This was then read into a 6.5.171 pipeline with the `FindCAxisLocations` filter and wrote out to a DREAM3D file. The output of NX test was also run and written to a file. These outputs were then verified to have no differences by using `h5py` to read both files and compare the values.
