# Regularize Z Spacing

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** resamples an **Image Geometry** that has *irregular* spacing along the Z axis onto a *regular* (uniform) Z spacing. This situation commonly arises from serial-sectioning data collection where the physical distance removed between successive sections is not constant.

The current physical Z position of every original slice boundary is read from a whitespace-delimited text file. The file must contain **(ZPoints + 1)** floating point values, where `ZPoints` is the number of cells (slices) along the Z axis of the selected **Image Geometry**. The first value is the position of the bottom boundary of the first slice, and the last value is the total Z extent of the volume.

The **Filter** computes a new number of Z slices as:

    newZPoints = floor(totalZExtent / newZSpacing)

(with a minimum of 1). For each new, evenly spaced Z plane the **Filter** selects the original slice whose boundary interval contains that plane's position and copies the entire XY slab of cell data from the original slice into the new slice. The X and Y dimensions, spacing, and the geometry origin are unchanged; only the Z dimension count and Z spacing are updated.

### Input File Format

The input file is a simple text file with one floating point value per line (or whitespace-delimited). For an **Image Geometry** with 5 Z slices the file must contain 6 values, for example:

    0.0
    1.0
    3.0
    6.0
    9.5
    12.0

The values must be monotonically non-decreasing, and the final value (the total Z extent) must be greater than zero. The `.txt` extension is a file-dialog hint only; any readable text file is accepted.

### Output Geometry

When *Perform In Place* is enabled the original **Image Geometry** is replaced by the resampled result. When it is disabled the resampled result is written to the *Created Image Geometry* path and the original geometry is left unchanged.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
