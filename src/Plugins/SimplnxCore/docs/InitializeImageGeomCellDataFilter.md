# Initialize Image Geometry Cell Data

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** overwrites every cell within a user-defined *subvolume* of an **Image Geometry** with either a constant value or random values. Use this to reset a region of interest, mask out an area for testing, or stub in synthetic data for development.

### Subvolume Specification

The subvolume is specified by minimum and maximum **cell indices** along each axis (X, Y, Z). Indices are **0-based, inclusive** -- a min of 5 and a max of 9 includes cells 5, 6, 7, 8, and 9 (5 cells). The min must be less than or equal to the max along each axis. The min must be at least 0 and the max must be less than the geometry's dimension along that axis.

The subvolume operates **only on cells inside this index range**; cells outside are unaffected.

### Initialization Type

The *Initialization Type* parameter provides three modes:

- **Manual [0]**: every cell in the subvolume is set to a single user-specified constant value. The value is reinterpreted into each array's data type (e.g., 1.5 → 1 for an int array, 1.5 → 1.5 for a float array, anything non-zero → true for a bool array).
- **Random [1]**: every cell is set to an independent random value drawn from the **full range of the array's data type**. For uint8, this means 0-255. For float32, this means the full IEEE-754 range (which produces extreme values rarely useful for visualization).
- **Random With Range [2]**: every cell is set to an independent random value drawn from the user-specified [min, max] interval. The min/max are interpreted in the array's data type.

Multiple cell arrays can be initialized in one filter pass; each gets independently-drawn random values when in Random mode.

### Random Seed

The random modes use a configurable seed for reproducibility. Enable *Use Seed for Random Generation* and supply an integer to make results deterministic across pipeline runs. With the option disabled, a time-based seed is used (results vary per run).

### Behavior on Boolean Arrays

For boolean arrays in random modes, each cell is set to a uniformly-chosen true/false. Range parameters are ignored for boolean arrays.

### Required Input Sources

- **Input Image Geometry** -- the geometry whose cell data will be modified in-place. Typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [ITK Import Image Stack](../ITKImageProcessing/ITKImportImageStackFilter.md), or an EBSD reader.
- **Cell Attribute Arrays to Initialize** -- one or more cell-level arrays already attached to the geometry's Cell Attribute Matrix.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
