# Map Point Cloud to Regular Grid

## Group (Subgroup)

Sampling (Mapping)

## Description

This **Filter** determines, for a user-defined grid, in which voxel each point in a **Vertex Geometry** lies.  The user can either construct a sampling grid by specifying the dimensions, or select a pre-existing **Image Geometry** to use as the sampling grid.  The voxel indices that each point lies in are stored on the vertices.  

Additionally, the user may opt to use a mask; points for which the mask are false are ignored when computing voxel indices (instead, they are initialized to voxel 0).

One of the features provided to the user is control over Out-of-Bounds handling and values. The former comes with three options `Silent`, `Warning with Count`, and `Error at First Instance`, these give the user control over how the algorithm handles values from the vertex geometry falling outside the image geometry. The default selection is `Silent`, but it is mostly provided as a way to preserve existing functionality. For most use cases it is recommended to use the `Warning with Count` because: if no values fall out of bounds, no warning will be returned. Therefore, it acts as a simple validation that will not clutter warnings if not needed. What follows are a few use cases we had in mind when adding this functionality, organized by handling type:

- `Silent` option:
  - User may want to preserve identical functionality between **SIMPL** and **simplnx**
  - User may expect values to fall outside the target image geometry or intend to crop all that fall outside it anyway
- `Warning with Count` option:
  - User may be intending to create a general use pipeline for various different tasks, for which monitoring and validation may be important
  - User may intend to create a workflow that will be distributed in which the end user may not have control over the parameter, but should be monitoring for anomalies in output
  - User may want to watch for unexpected behavior
- `Error at First Instance` option
  - User may to trace down where a anomaly first occured
  - User may be creating a pipeline in a known problem space with a well defined outcome where any data anomalies must be caught early to prevent downstream problems

Continuing along the Out-of-Bounds discussion, the Out-of-Bounds value allows the user to specify a specific `uint64` value to use for every value from the vertex geometry that falls outside the image geometry. The default value is just the max `unsigned long long int` in an effort to make sure that it doesn't intersect with exisiting indexed values. This is identical to previous functionality. However, consider the situation where a user has a geometry that contains 1000 voxels, in this case the actual index values are 0-999, so a user could select 1000 and it wouldn't overlap any existing voxel index. Doing this may reduce skew of coloring or other statistic-based analysis. Advanced users may intentionally select a value that overlaps an existing voxel index they wish to remove in a later filter or to later downcast the datasize without overflow, but this is considered an edge case that is functional, but not recommended.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
