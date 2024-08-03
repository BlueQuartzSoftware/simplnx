# Generate and Write Pole Figure Images

## Group (Subgroup)

IO (Output)

## Description

This **Filter** creates a standard crystallographic pole figure image for each **Ensemble** (phase) in a selected **Data Container**. The **Filter** uses Euler angles in radians and requires the crystal structures and material names for each **Ensemble** array and the corresponding **Ensemble** Ids on the **Cells**. The **Filter** also optionally can use a *mask* array to determine which angles are valid for the pole figure computation.

In a practical sense, this means that the following information is available to the filter:

- Cell Level

  - Euler Angles (Float 32) ordered as sets of (phi1, Phi, phi2).
  - Phases (Int32) This is the phase that each Euler angle belongs to
  - Optional Mask(boolean or uint8) True/1 if the Euler angle should be included in the pole figure.

- Ensemble Level (Phase Information)

  - Laue Class (UInt32)
  - Material Names (String)

### Algorithm Choice

1: The pole figure algorithm uses a *modified Lambert square* to perform the interpolations onto the unit circle. This is an alternate type
of interpolation that the EBSD OEMs do not perform which may make the output from DREAM.3D look slightly different than output
obtained from the OEM programs.

**Only an advanced user with intimate knowledge of the modified Lambert projection should attempt to change the value for
the "Lambert Image Size (Pixels)" input parameter.**

2: Discrete Pole figure. The algorithm will simply mark each pixel that had at least 1 count as a black pixel.

## Output Options

### Write Image to Disk

The user can select to have the combined set of pole figures written to disk as a tiff image file

### Save Pole Figure as Image Geometry

The combined pole figure image will be saved into the DataStructure as an Image Geometry

### Save Raw Intensity Data

The normalized count data is saved for each pole figure into a Data Array that is stored inside an Image Geometry. This allows
the user to select their own color plots. The Image Geometry will also have a string DataArray that lists the pertinent
data that went into the creation: Number of points, which hemisphere, Phase Name, etc.

### Image Layout

The 3 pole figures can be laid out in a Square, Horizontal row or vertical column. Supporting information (including the color bar legend for color pole figures) will also be printed on the image.

| Colorized Intensity | Discrete |
|--------------------|----------|
| ![Example Pole Figure Using Square Layout](Images/PoleFigure_Example.png) | ![Example Pole Figure Using Square Layout](Images/Pole_Figure_Discrete_Example.png) |

% Auto generated parameter table will be inserted here

## Example Pipelines

- TxCopper_Exposed
- TxCopper_Unexposed

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
