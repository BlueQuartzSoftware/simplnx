# Rotate Euler Reference Frame

## Group (Subgroup)

Processing (Conversion)

## Description

This **Filter** performs a passive rotation (Right hand rule) of the Euler Angles about a user defined axis. The *reference frame* is being rotated and thus the *Euler Angles* necessary to represent the same orientation must change to account for the new *reference frame*.  The user can set an *angle* and an *axis* to define the rotation of the *reference frame*.

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

+ INL Export
+ Export Small IN100 ODF Data (StatsGenerator)
+ Edax IPF Colors
+ Confidence Index Histogram

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
