# Merge Twins

## Group (Subgroup)

Reconstruction (Grouping)

## Description

This **Filter** groups neighboring **Features** (grains) that share a *twin relationship* into a single *parent* grain. Twins are sub-grains of a parent grain that share a highly specific crystallographic relationship, and for many downstream analyses -- morphology, size distributions, neighbor statistics -- it is useful to treat a grain and its twin variants as one entity rather than as separate grains.

Only the FCC *&Sigma;3* twin relationship is detected by this filter.

### What is a Twin?

A **twin** is a region within a grain whose crystal lattice is a mirror image of the surrounding parent grain across a specific plane, called the *twin plane*. Mechanically, the twinned region is still part of the same grain structure; crystallographically, it has a different -- but highly specific -- orientation.

The most common twin in FCC metals (copper, nickel, aluminum, austenitic stainless steels, superalloys such as IN100 and Inconel) is the **&Sigma;3 twin**. Two grains are in a &Sigma;3 twin relationship when they are related by a 60&deg; rotation about the `<111>` crystal direction. This is often called an *annealing twin* because it frequently forms during recrystallization.

The "&Sigma;3" notation comes from *Coincidence Site Lattice* (CSL) theory: &Sigma; is the reciprocal density of lattice sites shared between the two crystals, and &Sigma;3 means 1 out of every 3 atomic sites line up exactly across the boundary.

### Why Merge Twins?

After segmentation (for example with the *Segment Features (Misorientation)* filter), each twin variant is identified as its own separate **Feature** because it has a distinct average orientation from its parent. This is correct from a pure orientation standpoint, but can mislead morphological and statistical analyses:

- A grain with three twin bands will be counted as four separate grains, inflating the apparent grain count.
- Grain-size distributions will be skewed smaller because twin variants are smaller than the parent.
- Neighbor statistics will count twin-to-parent boundaries as regular grain boundaries.

Merging twin variants back into a single parent grain produces a *post-twin* grain structure that more closely reflects what a metallurgist would call a "grain" under an optical microscope, and is usually the correct input for morphological statistics.

### How This Filter Works

The filter uses a burn-style grouping algorithm operating on the **Feature** level (not the **Cell** level):

1. A **Feature** that has not yet been assigned a parent is selected as the starting point and given a new *Parent Id*.
2. For each of its contiguous neighbors (provided by the input *Contiguous Neighbor List*), the filter computes the misorientation between the two features using their average quaternions.
3. If the misorientation axis is within the *Axis Tolerance* of `<111>` **and** the misorientation angle is within the *Angle Tolerance* of 60&deg;, the neighbor is in a &Sigma;3 twin relationship with the current feature and is assigned the same *Parent Id*.
4. The grouping then extends recursively through the newly added neighbor's own neighbors, so that a chain of twins-of-twins is absorbed into one parent.
5. When no more features can be added to the current parent group, a new unassigned feature is picked and the process repeats until every eligible feature has a *Parent Id*.

### Parameter Guidance

- **Axis Tolerance (degrees)**: How far the misorientation rotation axis can deviate from the ideal `<111>` direction and still be accepted as a twin. Default is 3&deg;. Typical values range from 1&deg; to 5&deg;. Tighter tolerance = fewer false positives but may miss slightly distorted real twins.
- **Angle Tolerance (degrees)**: How far the misorientation rotation angle can deviate from the ideal 60&deg; and still be accepted as a twin. Default is 2&deg;. Typical values range from 1&deg; to 3&deg;.

Both tolerances are in **degrees**. A tolerance of 3&deg;/2&deg; accepts a neighbor as a twin if its misorientation falls within roughly 58&deg;-62&deg; about an axis within 3&deg; of `<111>`.

### Required Input Sources

This filter requires that several prior operations have already been run:

- **Cell Feature Ids** -- produced by a segmentation filter (e.g., [Segment Features (Misorientation)](EBSDSegmentFeaturesFilter.md)).
- **Contiguous Neighbor List** -- produced by [Compute Feature Neighbors](../SimplnxCore/ComputeFeatureNeighborsFilter.md).
- **Feature Phases** -- produced by [Compute Feature Phases](../SimplnxCore/ComputeFeaturePhasesFilter.md).
- **Average Quaternions** -- produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md).
- **Crystal Structures** -- the ensemble-level array describing each phase's Laue class.

### Limitations

- **Cubic-High (m3m) only.** The filter only evaluates twin relationships for phases with the m3m Laue class (FCC and BCC cubic materials). Phases with other symmetries are skipped with a warning; their features remain ungrouped. Only FCC materials actually exhibit &Sigma;3 annealing twins in practice.
- **&Sigma;3 only.** Other CSL boundaries (&Sigma;5, &Sigma;7, &Sigma;9, etc.) are not detected.
- **Requires well-computed average orientations.** The twin detection is only as accurate as the input *Average Quaternions*. If twin variants were segmented together with the parent (too-loose segmentation tolerance), the average orientation will be wrong and this filter will not reliably detect twins.

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
