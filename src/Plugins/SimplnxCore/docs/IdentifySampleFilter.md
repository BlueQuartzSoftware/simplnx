# Isolate Largest Feature (Identify Sample)

## Group (Subgroup)

Processing (Cleanup)

## Description

Serial-sectioning experiments -- especially FIB-SEM -- typically *over-scan* the sample area, producing a border of *bad* data around the actual sample. This **Filter** identifies the sample within that over-scanned volume by finding the single largest contiguous region of *good* cells.

The filter assumes that the sample is one connected set of cells, and it requires that the user has already produced a boolean mask marking which cells are *good* and which are *bad* -- typically via [Multi-Threshold Objects](MultiThresholdObjectsFilter.md) applied to a confidence or quality array.

The algorithm is:

1. Search for the largest contiguous set of *good* cells. This is assumed to be the sample.
2. Change all other *good* cells to *bad*. (This removes the "speckling" of *good* cells in the outer border region.)

If *Fill Holes* is enabled, two additional steps are run:

1. Search for the largest contiguous set of *bad* cells. This is assumed to be the outer border region.
2. Change all other *bad* cells to *good*. (This removes the "speckling" of *bad* cells inside the sample.)

*Note:* If the sample contains real holes, enabling *Fill Holes* will close them by calling all cells "inside" the sample *good*. To reidentify those holes afterward, re-run the threshold filter with the criteria *GoodVoxels = 1* AND whatever original criterion identified the holes. This limits the original hole-finding criteria to within the sample and not the outer border region.

## Slice-By-Slice Option

Only completely water-tight, internal holes within the sample are addressed when *Fill Holes* 
is enabled.  To fill in a contiguous group of good cells that includes holes located along 
the outer edge of the sample, try enabling *Process Data Slice-By-Slice*.  For each slice 
of the chosen plane, this will search for the largest contiguous set of *good* **Cells**, 
set all other *good* **Cells** to be *bad* **Cells**, and (if *Fill Holes* is enabled) 
fill all water-tight holes PER SLICE instead of the whole 3D volume at once.  This option 
can be used to allow non water-tight holes to be filled without also accidentally 
filling the surrounding overscan area.

| Name                                           | Description                                                                  |
|------------------------------------------------|------------------------------------------------------------------------------|
| ![Small IN100 IPF Map](Images/Small_IN100.png) | Good dataset to use this filter                                              |
| ![APTR IPF Colors](Images/aptr12_001_0.png)    | NOT** a good data set to use because there is **no** overscan of the sample. |

### Slice-By-Slice Plane

When *Process Data Slice-By-Slice* is enabled, the *Slice-By-Slice Plane* parameter selects the plane along which the volume is scanned one slice at a time:

- **XY [0]**: Processes the volume slice by slice along the Z axis, scanning each XY plane independently.
- **XZ [1]**: Processes the volume slice by slice along the Y axis, scanning each XZ plane independently.
- **YZ [2]**: Processes the volume slice by slice along the X axis, scanning each YZ plane independently.

### Required Input Sources

- **Good Voxels Mask** -- a boolean array marking cells as *good* or *bad*, typically produced by [Multi-Threshold Objects](MultiThresholdObjectsFilter.md) applied to EBSD confidence index, image quality, or a similar scalar.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction
+ INL Export

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
