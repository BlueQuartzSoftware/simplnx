# Merge Twins

## Group (Subgroup)

Reconstruction (Grouping)

## Description

*THIS FILTER ONLY WORKS ON CUBIC-HIGH (m3m) Laue Classes.*

This **Filter** groups neighboring **Features** that are in a twin relationship with each other (currently only FCC &sigma; = 3 twins).  The algorithm for grouping the **Features** is analogous to the algorithm for segmenting the **Features** - only the average orientation of the **Features** are used instead of the orientations of the individual **Elements**.  The user can specify a tolerance on both the *axis* and the *angle* that defines the twin relationship (i.e., a tolerance of 1 degree for both tolerances would allow the neighboring **Features** to be grouped if their misorientation was between 59-61 degrees about an axis within 1 degree of <111>, since the Sigma 3 twin relationship is 60 degrees about <111>).

## Algorithm

The algorithm uses a seed-and-grow approach analogous to feature segmentation. A random unassigned feature is selected as a seed and assigned a new parent ID. The seed's contiguous neighbors are examined: if the misorientation between the seed and a neighbor is within tolerance of the Sigma 3 twin relationship (60 degrees about <111>), the neighbor is added to the same parent group. This process repeats for newly grouped features until no more twins are found, then a new seed is selected. After grouping, cell-level parent IDs are assigned by looking up each cell's feature ID in the feature-to-parent map.

### In-Core Path

Feature-level arrays (phases, parent IDs, average quaternions, crystal structures) are accessed through the AbstractDataStore API. The contiguous neighbor list provides adjacency information. Cell-level arrays are written directly.

### Out-of-Core Path

The cell-level parent ID array is initialized in 64K-element chunks via `copyFromBuffer` to avoid a single large fill operation on an OOC store. After the feature grouping phase completes, the feature-to-parent map is cached locally. Cell-level feature IDs are then read in 64K-tuple chunks via `copyIntoBuffer`, translated to parent IDs using the local cache, and the results are bulk-written via `copyFromBuffer`.

### Performance

The feature-level grouping algorithm involves random access to feature arrays (phases, quaternions, parent IDs), but feature counts are small enough (thousands) that this does not cause OOC bottlenecks. The cell-level pass, which touches millions of voxels, uses sequential chunked I/O to avoid per-element OOC overhead.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
