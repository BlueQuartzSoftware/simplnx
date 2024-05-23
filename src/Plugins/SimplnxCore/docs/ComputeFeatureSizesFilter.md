# Compute Feature Sizes

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the sizes of all **Features**.  The **Filter** simply iterates through all **Elements** querying for the **Feature** that owns them and keeping a tally for each **Feature**.  The tally is then stored as *NumElements* and the *Volume* and *EquivalentDiameter* are also calculated (and stored) by knowing the volume of each **Element**.

During the computation of the **Feature** sizes, the size of each individual **Element** is computed and stored in the corresponding **Geometry**. By default, these sizes are deleted after executing the **Filter** to save memory. If you wish to store the **Element** sizes, select the *Generate Missing Element Sizes* option. The sizes will be stored within the **Geometry** definition itself, not as a separate **Attribute Array**.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (01) SmallIN100 Morphological Statistics
+ (10) SmallIN100 Full Reconstruction
+ InsertTransformationPhase
+ (06) SmallIN100 Postsegmentation Processing
+ (06) SmallIN100 Synthetic
+ (09) Image Segmentation

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
