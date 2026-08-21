# Read Fiji Montage (ITK)

Imports a set of overlapping tile images described by a Fiji/ImageJ TileConfiguration file, placing each tile in its own data group for later montage assembly.

## Group (Subgroup)

IO (Input)

## Description

This filter reads a Fiji/ImageJ *TileConfiguration* file and imports the overlapping tile images it references so they can later be assembled into a single montage. The configuration file lists each tile image along with the coordinate at which it belongs in the mosaic. The image files **must** be located in the same directory as the configuration file.

Each tile is imported into its own **Image Geometry**, named using the *Image Geometry Prefix* followed by the tile's file name. Inside each Image Geometry, the pixel data is stored in a **Cell Attribute Matrix** group, which holds the image **Data Array**. If *Parent Imported Images Under a DataGroup* is enabled, all of these per-tile Image Geometries are placed under a single parent **DataGroup** so they are grouped together in the DataStructure.

Use this filter as the first step in a montage-stitching workflow when your tiles are described by a Fiji TileConfiguration file.

### Parameter Guidance

- **Change Origin** / **Origin** — when enabled, overrides the origin of the assembled mosaic with the user-supplied value (in the chosen length unit).
- **Convert To GrayScale** / **Color Weighting** — when enabled, converts color tiles to grayscale using a weighted (luminosity) average of the red, green, and blue channels. The default weights are the BT.709 values *Red 0.2125, Green 0.7154, Blue 0.0721*, which approximate human brightness perception (the eye is most sensitive to green). The weights can be changed to any values. The incoming color array must be 8-bit unsigned (uint8); otherwise that image is skipped.
- **Set Image Data Type** / **Output Data Type** — when enabled, stores the imported pixel data using the chosen numeric type (uint8, uint16, or uint32).
- **Length Unit** — the physical length unit applied to the created Image Geometries.

### Required Input Sources

A Fiji/ImageJ TileConfiguration text file, with all referenced tile images in the same directory.

## Example Configuration File

    # Define the number of dimensions we are working on
    dim = 2                         <===== THIS LINE IS REQUIRED

    # Define the image coordinates  <===== THIS LINE IS REQUIRED
    SampleMosaic_p0.bmp; ; (0, 0)
    SampleMosaic_p1.bmp; ; (1227.55, 0)
    SampleMosaic_p3.bmp; ; (0.23675, 920.01)
    SampleMosaic_p2.bmp; ; (1227.55, 919.774)
    SampleMosaic_p4.bmp; ; (0.23675, 1839.55)
    SampleMosaic_p5.bmp; ; (1227.31, 1839.55)

% Auto generated parameter table will be inserted here

## Example Pipelines

Prebuilt Pipelines / Examples / ITKImageProcessing / Fiji Import

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
