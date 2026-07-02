# Compute Feature Neighborhoods

## Group (Subgroup)

Statistics (Morphological)

## Description

For each feature, determine how many other features have their *centroid* within a user defined search radius of the feature's centroid.

The **Search Radius Type** parameter selects how that search radius is defined:

- **Multiples of Average Diameter** *(default)*: the search radius is computed as the average *Equivalent Sphere Diameter* of all features multiplied by the user supplied *Multiples of Average Diameter* value, divided by two. This is the original behavior of the filter. This mode requires the *Equivalent Diameters* feature array.
- **Search Radius (microns)**: the user supplies an absolute search radius (in microns) that is used directly. This mode does **not** require the *Equivalent Diameters* array.

The algorithm for determining the number of **Features** is given below:

1. Compute the average equivalent diameter for all features in a given phase
2. Determine the search radius from the selected **Search Radius Type**
3. Define a sphere centered at the **Feature**'s *centroid* with the search radius from step 2
4. Check every other **Feature**'s *centroid* to see if it lies within the sphere and keep count and list of those that satisfy
5. Repeat 3. & 4. for all **Features**

![](images/ComputeFeatureNeighborhoods_MultiplesOfAvgDiameter.png)

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
