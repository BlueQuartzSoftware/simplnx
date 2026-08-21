# Read Volume Graphics File (.vgi/.vol)

## Group (Subgroup)

IO (Input)

## Description

This filter imports **Volume Graphics** data, the format produced by Volume Graphics VGStudio and similar industrial computed-tomography (CT) software. A Volume Graphics dataset is a pair of files that must live in the same directory:

- A `.vgi` file: a small human-readable metadata header that lists the volume dimensions, voxel spacing (resolution), length units, and the name of the companion `.vol` file.
- A `.vol` file: the raw block of voxel values (the reconstructed density volume from the CT scan).

The filter reads the `.vgi` header to learn the geometry, then reads the raw voxel data from the `.vol` file. Both files must exist for the filter to run.

### What Is Created

The filter creates an **Image Geometry** (a regular grid of equally-sized voxels) whose dimensions and spacing come from the `.vgi` header. Under that geometry it creates a **Cell Attribute Matrix** containing a single `float32` density array (one value per voxel), the per-voxel value read from the `.vol` file. The geometry origin is set to (0, 0, 0).

This filter reads the full volume described by the `.vgi` file. It does not currently extract an arbitrary sub-volume, even though the `.vgi` header may describe a region of interest.

### Dimensions, Spacing, and Units

The volume dimensions (number of voxels along X, Y, and Z), the voxel spacing (the physical size of one voxel), and the length unit are all read from the `.vgi` header. When the header's `unit` field is `mm`, the geometry length unit is set to millimeters; otherwise the spacing values are stored as plain numbers in whatever unit the header implies.

### Required Input Sources

None — this filter reads directly from a `.vgi`/`.vol` file pair on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
