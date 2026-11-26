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

The `.vol` file must hold 32-bit float values. If the `.vgi` header gives a different `Datatype` or `BitsPerElement`, preflight fails with error -91505. A header that does not give a type is read as 32-bit float.

This filter reads the full volume described by the `.vgi` file. It does not currently extract an arbitrary sub-volume, even though the `.vgi` header may describe a region of interest.

### Dimensions, Spacing, and Units

A geometry block with `unit = mm` uses millimeters. The reader also assumes millimeters when the unit field is absent. Other unit strings produce **Unspecified**.

### Required Input Sources

None — this filter reads directly from a `.vgi`/`.vol` file pair on disk.

### Geometry Metadata Overrides

**Set Origin** replaces the default origin of (0, 0, 0) with the X, Y, and Z coordinates you provide. **Set Spacing** replaces the input spacing with the X, Y, and Z spacings you provide. Origins must be finite. Spacing must be finite and greater than zero.

If the reader cannot convert a resolution value to float32, the error gives the header path, field name, and value. It directs you to enable **Set Spacing**. Use this setting when the resolution is missing, invalid, or outside the float32 range.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
