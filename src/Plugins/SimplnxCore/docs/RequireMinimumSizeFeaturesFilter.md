# Require Minimum Size Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes **Features** that have a total number of **Cells** below the minimum threshold defined by the user. Entering a number larger than the largest **Feature** generates an *error* (since all **Features** would be removed). Hence, a choice of threshold should be carefully be chosen if it is not known how many **Cells** are in the largest **Features**. After removing all the small **Features**, the remaining **Features** are isotropically coarsened to fill the gaps left by the small **Features**.

The **Filter** can be run in a mode where the minimum number of neighbors is applied to a single **Ensemble**.  The user can select to apply the minimum to one specific **Ensemble**.

## WARNING: Feature Data Will Become Invalid

By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature level data should be rerun to ensure accurate final results from your pipeline.

## WARNING: NeighborList Removal

If the Cell Feature AttributeMatrix contains any *NeighborList* data arrays, those arrays will be **REMOVED** because those lists are now invalid. Re-run the *Find Neighbors* filter to re-create the lists.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (10) SmallIN100 Full Reconstruction
+ (06) SmallIN100 Postsegmentation Processing

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
