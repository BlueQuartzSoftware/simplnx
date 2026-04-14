# Compute Surface Area to Volume & Sphericity

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the ratio of surface area to volume for each **Feature** in an **Image Geometry**.

This filter also optionally calculate the [Sphericity](https://en.wikipedia.org/wiki/Sphericity) of each feature.

![Equation for Sphericity used in the filter](Images/Sphericity_Equation.png)

This **Filter** determines whether a **Feature** touches an outer *Surface* of the sample volume. A **Feature** is considered touching the *Surface* of the sample if either of the following conditions are met:

+ Any cell location is x<sub>min</sub>, x<sub>max</sub>, y<sub>min</sub>, y<sub>max</sub>, z<sub>min</sub> or z<sub>max</sub>
+ Any cell has **Feature ID = 0** as a neighbor.

## Algorithm

The filter computes the surface-area-to-volume ratio for each feature in two phases:

**Phase 1 -- Surface area accumulation**: For each voxel in the image geometry, examine its 6 face-connected neighbors. When a neighbor belongs to a different feature (and the neighbor's Feature ID > 0), the area of the shared face is added to the current feature's surface area total. The face area depends on which axis the face is normal to:

+ Z-normal faces (shared by +/-Z neighbors): spacing.x * spacing.y
+ Y-normal faces (shared by +/-Y neighbors): spacing.y * spacing.z
+ X-normal faces (shared by +/-X neighbors): spacing.z * spacing.x

**Phase 2 -- Ratio and sphericity**: For each feature, divide the accumulated surface area by the feature's volume (number of cells * voxel volume). If sphericity is requested, it is computed as: sphericity = (pi^(1/3) * (6V)^(2/3)) / SA, where a perfect sphere has sphericity = 1.0.

### In-Core Algorithm (Direct)

The in-core variant iterates all voxels in Z-Y-X order and uses pre-computed flat-index offsets to look up the 6 face neighbors directly via operator[] on the FeatureIds DataStore. Surface area is accumulated into a local vector (since multiple voxels contribute to each feature), and the final ratio is written to the output array.

### Out-of-Core Algorithm (Scanline)

When the FeatureIds array is stored out-of-core in chunked format, the in-core algorithm's scattered neighbor lookups would trigger chunk thrashing. The Scanline variant reads one complete Z-slice at a time using sequential bulk I/O, maintaining a 3-slice rolling window:

+ **prevSlice**: Z-slice at z-1, needed for -Z neighbor lookups
+ **curSlice**: Z-slice at z (being processed)
+ **nextSlice**: Z-slice at z+1, needed for +Z neighbor lookups

Within a Z-slice, X and Y neighbors are simple index offsets within curSlice. After the voxel scan, the feature-level NumCells array is also bulk-read into a local cache, the ratio and optional sphericity are computed locally, and the results are bulk-written back. This ensures zero random-access operator[] calls on any OOC DataStore.

### Performance

The in-core and out-of-core variants produce identical results. The algorithm dispatch is automatic based on the storage type of the FeatureIds array.

### WARNING - Aliasing

The surface area will be the surface area of the **Cells** in contact with the neighboring **Feature** and will be influenced by the aliasing of the structure.  As a result, the surface area to volume will likely be over-estimated with respect to the *real* **Feature**.

### WARNING - Skewed Results for features touching the surface

Because the filter does not include any surface that is touching/connected to a "FeatureId = 0", those features that are in contact with the edge of the virtual volume or in contact with internal features that are labeled as "FeatureId = 0" will have their values skewed.

### Warning - 2D Image Geometry Results

Because even a single slice has *volume* according to DREAM3D-NX, results will still be computed. These results should **NOT** be interpreted as "Boundary Length to Area" values.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
