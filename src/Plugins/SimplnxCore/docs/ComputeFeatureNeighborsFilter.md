# Compute Feature Neighbors

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines, for each **Feature**, the number of other **Features** that are in contact with it.  The algorithm for determining the number of "contiguous" neighbors of each **Feature** is as follows:

1. Identify the **Feature** to which a **Cell** belongs
2. Identify the **Features** to which each of the current **Cell**'s six (6) face-face neighboring **Cells** (front, back, left, right, up, down) belong
3. If a neighboring **Cell** belongs to a different **Feature** than the current **Cell**, then that **Feature** (owner of the neighboring **Cell**) is added to the list of contiguous neighbors of the **Feature** that owns the current **Cell**
4. Repeat 1-3 for all **Cells**

While performing the above steps, the number of neighboring **Cells** with a different **Feature** owner than a given **Cell** is stored, which identifies whether a **Cell** lies on the surface/edge/corner of a **Feature** (i.e. the **Feature** boundary). Additionally, the surface area shared between each set of contiguous **Features** is calculated by tracking the number of times two neighboring **Cells** correspond to a contiguous **Feature** pair. The **Filter** also notes which **Features** touch the outer surface of the sample (this is obtained for "free" while performing the above algorithm). The **Filter** gives the user the option whether or not they want to store this additional information.

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
