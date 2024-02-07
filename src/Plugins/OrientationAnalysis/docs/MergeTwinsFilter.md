# Merge Twins

## Group (Subgroup)

Reconstruction (Grouping)

## Description

*THIS FILTER ONLY WORKS ON CUBIC-HIGH (m3m) Laue Classes.*

This **Filter** groups neighboring **Features** that are in a twin relationship with each other (currently only FCC &sigma; = 3 twins).  The algorithm for grouping the **Features** is analogous to the algorithm for segmenting the **Features** - only the average orientation of the **Features** are used instead of the orientations of the individual **Elements**.  The user can specify a tolerance on both the *axis* and the *angle* that defines the twin relationship (i.e., a tolerance of 1 degree for both tolerances would allow the neighboring **Features** to be grouped if their misorientation was between 59-61 degrees about an axis within 1 degree of <111>, since the Sigma 3 twin relationship is 60 degrees about <111>).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (10) SmallIN100 Full Reconstruction
+ (06) SmallIN100 Postsegmentation Processing

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
