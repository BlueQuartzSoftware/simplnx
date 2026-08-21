# Compute Feature Neighbors

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines, for each **Feature**, which other **Features** it is in direct contact with and how much boundary they share. The result is a set of feature-level arrays that many downstream filters depend on -- neighbor misorientation statistics, GBCD, twin merging, boundary strength analysis, and more.

### How This Filter Works

The filter walks every cell in the input **Image Geometry** and compares each cell's *Feature Id* to the *Feature Ids* of its six face-sharing neighbors (front, back, left, right, up, down):

1. Identify the **Feature** the current cell belongs to.
2. For each of the cell's six face-neighbors, check whether the neighbor cell belongs to a different feature.
3. If so, record that neighbor feature in the current feature's *contiguous neighbor* list and increment the count of shared cell-faces between the two features.
4. While iterating, the filter also tracks which cells lie on a feature boundary (any cell with at least one differently-labeled neighbor) and which features touch the outer geometry boundary (any cell whose neighbor is outside the volume).

### What This Filter Produces

The main feature-level outputs are:

- **Neighbor List** -- for each feature, the list of *Feature Ids* of the features it touches.
- **Number of Neighbors** -- for each feature, the integer count of its contiguous neighbors. Equivalent to the length of each *Neighbor List* entry.
- **Shared Surface Area List** -- for each pair of neighboring features, the number of cell-faces they share. This is in **cell-face units** (a dimensionless count), not physical area. To convert to physical area, multiply by the area of one cell face (which depends on the cell spacing and the orientation of the shared face).

Two optional outputs can also be stored:

- **Boundary Cells** (enable *Store Boundary Cells Array*) -- a cell-level array marking which cells sit on any feature boundary. Useful for visualization and for flagging the cells that contribute to grain-boundary statistics.
- **Surface Features** (enable *Store Surface Features Array*) -- a feature-level boolean marking which features touch the outer volume bounds. Downstream statistical filters often exclude surface features from distributions because they are biased by sample truncation (see [Compute Biased Features](ComputeBiasedFeaturesFilter.md)).

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

This filter handles Image Geometries of all dimensions (0D/1D/2D/3D). Thus, it is up to the user to ensure spacing is set inline with intended behavior, specifically for Shared Surface Area List calculation. For more details see the Image Geometry section of the Geometry documentation (currently in the python docs).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ (02) Small IN100 Full Reconstruction
+ (02) Single Hexagonal Phase Equiaxed
+ (03) Single Cubic Phase Rolled
+ (05) Composite
+ (01) Single Cubic Phase Equiaxed
+ (04) Two Phase Cubic Hexagonal Particles Equiaxed
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
