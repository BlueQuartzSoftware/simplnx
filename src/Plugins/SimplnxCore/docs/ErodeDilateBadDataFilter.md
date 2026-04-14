# Erode/Dilate Bad Data

## Group (Subgroup)

Processing (Cleanup)

## Description

Bad data refers to a **Cell** that has a *Feature Id* of *0*, which means the **Cell** has failed some sort of test and
been marked as a *bad* **Cell**.

### Dilation

If the **bad** data is *dilated*, the Filter grows the *bad* data by one **Cell** in
an iterative sequence for a user defined number of iterations. During the *dilate* process the *Feature Id* of any
Cell neighboring a *bad* **Cell** will be changed to *0*.

| Before Dilation                      | After Dilation                       |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateBadData_1.png) | ![](Images/ErodeDilateBadData_2.png) |

### Erosion

If the *bad* data is *eroded*, the Filter shrinks the
bad data by one **Cell** in an iterative sequence for a user defined number of iterations. During the *erode* process
the *Feature Id* of the *bad* **Cell** is changed from *0* to the *Feature Id* of the majority of its neighbors. If
there is a tie between two *Feature Ids*, then one of the *Feature Ids*, chosen randomly, will be assigned to the *bad*
**Cell**.

| Before Erosion                       | After Erosion                        |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateBadData_1.png) | ![](Images/ErodeDilateBadData_3.png) |

`

Goals a user might be trying to accomplish with this Filter include:

- Remove small or thin regions of bad data by running a single (or two) iteration *erode* operation.
- Increase the size of a *bad* data region by running an *dilate* operation. This might be useful if the experimental
  technique tends to underestimates the size of certain objects. For example, when running EBSD, the pores (which show
  up as *bad* data) are generally smaller in the scans than in the specimen, because the beam, when it is just inside
  the pore, still can pick up signal from the material just beneath the pore.

Running the *erode-dilate* operations in pairs can often change the size of some objects without affecting others. For
example, if there were a number of big pores and a number of single *bad* **Cells**, running a single *erode* operation
would remove the single **Cells** and reduce the pores by one **Cell**. If this is followed immediately by a *dilate*
operation, then the pores would grow by one **Cell** and return to near their original size, while the single **Cells**
would remain removed and not "grow back".

### Operation

The *Operation* parameter selects which morphological operation to apply:

- **Dilate [0]**: Grows bad data regions by one **Cell** per iteration. Any **Cell** neighboring a bad **Cell** has its *Feature Id* changed to 0.
- **Erode [1]**: Shrinks bad data regions by one **Cell** per iteration. Each bad **Cell** is assigned the *Feature Id* of the majority of its neighbors.

## Algorithm

This filter performs iterative morphological erosion or dilation on "bad" voxels (cells with FeatureId == 0) within an ImageGeom grid.

### Erosion

For each bad voxel, the algorithm examines its 6 face-connected neighbors and tallies the FeatureIds of any good (non-zero) neighbors. The bad voxel is then assigned the FeatureId that appears most frequently among its good neighbors (a "majority vote"). If there is a tie, one of the tied FeatureIds is chosen. This process shrinks bad-data regions by one cell per iteration.

### Dilation

For each bad voxel, the algorithm examines its 6 face-connected neighbors. Any good neighbor adjacent to the bad voxel has its FeatureId set to 0, effectively growing the bad-data region outward by one cell per iteration.

In both cases, all sibling data arrays in the same Attribute Matrix (except those in the user's ignored list) are updated to match the FeatureId changes, so the data remains consistent.

### Iteration

The operation is repeated for the user-specified number of iterations. Each iteration makes a full pass over the volume. Because each pass modifies the data, subsequent iterations see the cumulative effect of all prior passes.

### Performance

This algorithm is optimized for both in-memory and out-of-core (OOC) data stores. When data resides on disk in chunked format, random voxel access can cause expensive chunk load/evict cycles. The implementation avoids this by:

- **Sequential Z-slice processing**: The volume is scanned one Z-slice at a time, which aligns with typical chunk boundaries and avoids random access patterns.
- **3-slice rolling window**: Three adjacent Z-slices of FeatureIds are held in memory simultaneously, allowing face-neighbor lookups without hitting the data store for each voxel.
- **Deferred bulk writes**: Data modifications are batched per Z-slice and written back in bulk, minimizing the number of I/O operations.
- **O(sliceSize) memory**: Per-slice mark arrays replace a full-volume neighbor array, keeping peak memory proportional to a single Z-slice rather than the entire volume.

## WARNING: Feature Data Will Become Invalid

By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature level data should be rerun to ensure accurate final results from your pipeline.

% Auto generated parameter table will be inserted here

## Example Pipelines

- (08) SmallIN100 Full Reconstruction
- (07) SmallIN100 Final Processing
- 04_Steiner Compact

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
