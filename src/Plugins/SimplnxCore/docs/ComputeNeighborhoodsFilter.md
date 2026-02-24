# Compute Feature Neighborhoods

## Group (Subgroup)

Statistics (Morphological)

## Description

For each feature, determine how many other features are within a user defined multiple of the average *Equivalent Sphere Diameter* of the centroid of the feature.

The algorithm for determining the number of **Features** is given below:

1. Define a sphere centered at the **Feature**'s *centroid* and with radius equal to the average equivalent sphere diameter multiplied by the user defined multiple
2. Check every other **Feature**'s *centroid* to see if it lies within the sphere and keep count and list of those that satisfy
3. Repeat 1. & 2. for all **Features**

## Output Notes

There are 2 outputs from this filter:
- The "Number of Neighbors" for each feature
- The list of "neighbor" features for each neighbor

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ InsertTransformationPhase
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
