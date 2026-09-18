# Rotate Euler Reference Frame

## Group (Subgroup)

Processing (Conversion)

## Description

This **Filter** performs a passive rotation (Right hand rule) of the Euler Angles about a user defined axis. The *reference frame* is being rotated and thus the *Euler Angles* necessary to represent the same orientation must change to account for the new *reference frame*. The user can set an *angle* and an *axis* to define the rotation of the *reference frame*.

For each Euler angle triplet the filter computes `g' = g · R(n, ω)`, where `g` is the orientation matrix of the input Euler angles (Bunge ZXZ convention), `R(n, ω)` is the rotation matrix for the user-supplied axis `n` and angle `ω`, and `g'` is converted back to Euler angles. For a rotation about the sample Z axis this reduces to `φ1' = φ1 − ω (mod 2π)` with `Φ` and `φ2` unchanged.

### Units & Conventions

- The rotation **angle parameter is in degrees**; the **Euler angle data is in radians** (Bunge ZXZ convention).
- The rotation axis does not need to be normalized — the filter normalizes it internally. A zero-length axis (0,0,0) is rejected during preflight.
- The selected Euler angles array is modified **in place** (no new array is created).
- Output Euler angles are canonicalized to `φ1, φ2 ∈ [0, 2π)` and `Φ ∈ [0, π]` by the double-precision conversion, then stored as `float32`. Because a `double` just below `2π` rounds up to the `float32` value `6.2831855`, a stored `φ1`/`φ2` can be numerically equal to (or a hair above) `2π`; treat the upper bound as inclusive at float32 precision.

## Algorithm

The filter constructs a rotation matrix from the user-specified axis and angle. For each element, the Euler angles are converted to an orientation matrix, multiplied by the rotation matrix (applying the passive reference frame rotation), re-normalized column-wise, and converted back to Euler angles. This is an in-place operation that modifies the input Euler angle array.

### In-Core Path

The Euler angle array is accessed through the AbstractDataStore API for both reading and writing.

### Out-of-Core Path

The Euler angle array is processed in sequential 64K-tuple chunks. Each chunk is bulk-read via `copyIntoBuffer`, the rotation is applied to all elements in the chunk, and the modified values are bulk-written back to the same location via `copyFromBuffer`. The rotation matrix is computed once at the start from the user-specified axis-angle.

### Performance

The per-element rotation (Euler to matrix, matrix multiply, matrix to Euler) is moderately compute-intensive, but I/O dominates for OOC data. The chunked read-modify-write pattern ensures sequential access through the array with minimal OOC overhead.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ EBSD_File_Processing/Read_EDAX_Ang_File
+ EBSD_File_Processing/Edax_IPF_Colors
+ EBSD_File_Processing/CI_Histogram
+ EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis
+ EBSD_File_Processing/ImportEdaxOIMData

## References

- D. Rowenhorst, A. D. Rollett, G. S. Rohrer, M. Groeber, M. Jackson, P. J. Konijnenberg, and M. De Graef, "Consistent representations of and conversions between 3D rotations," *Modelling and Simulation in Materials Science and Engineering*, vol. 23, no. 8, p. 083501, 2015. DOI: [10.1088/0965-0393/23/8/083501](https://doi.org/10.1088/0965-0393/23/8/083501)


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
