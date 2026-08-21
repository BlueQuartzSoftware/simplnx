# Compute Euclidean Distance Map

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates, for every **Cell**, how far that **Cell** is from the nearest **Feature** boundary, **triple line**, and/or **quadruple point**. A **Feature** is a contiguous region of like-segmented cells (for example a grain). These three terms describe the interfaces between **Features**:

- A **Feature boundary** (abbreviated **GB**, grain boundary) is the interface where two different **Features** meet.
- A **triple line** (abbreviated **TJ**, triple junction) is where three different **Features** meet.
- A **quadruple point** (abbreviated **QP**) is where four different **Features** meet.

The following algorithm explains the process:

1. Find the **Feature** that owns each **Cell** and the **Features** that own each of its six face-sharing neighbors.
2. For all **Cells** that have *at least 2* different neighboring **Features**, set their *GB* distance to *0*. For all **Cells** that have *at least 3*, set their *TJ* distance to *0*. For all **Cells** that have *at least 4*, set their *QP* distance to *0*.
3. For each of the three distance maps, iteratively "grow" outward from the **Cells** identified as distance *0* using the following sub-steps:

   - Determine the **Cells** that neighbor a **Cell** of distance *0* in the current map.
   - Assign a distance of *1* to those **Cells** and internally track the distance-*0* **Cell** as their *nearest neighbor*.
   - Repeat the previous two sub-steps, increasing the distance by *1* each iteration, until every **Cell** has a distance assigned.

   *Note:* the distances calculated at this point are integer "city-block" (Manhattan) distances, expressed as a count of **Cells** (voxels), not shortest straight-line distances. The nearest-neighbor information is used internally and is not saved as an output array.

4. If *Output arrays are Manhattan distance* is *false*, then the integer city-block distances are overwritten with the true straight-line (Euclidean) distance from each **Cell** to its internally tracked nearest-neighbor **Cell**. This Euclidean result is a physical distance expressed in the **Image Geometry** length units (for example microns) and is stored in a *float32* array instead of an *int32* array.

### Required Input Sources

- **Cell Feature Ids** -- the Feature that owns each Cell, produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

- (01) SmallIN100 Morphological Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
