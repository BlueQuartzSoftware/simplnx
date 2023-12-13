# Crop Geometry (Image)

## Description

This **Filter** allows the user to crop a region of interest (ROI) from an **Image Geometry**.  The input parameters are in units of voxels.  For example, if a **Image Geometry** has dimensions of 100x100x100 voxels and each voxel was 0.25 x 0.25 x 0.25 units per voxel, then if the user wanted to crop the last 5 microns in the X direction, then the user would enter the following:

Bounds: 100 voxels * 0.25 micron/voxel = 25 microns

    Xmin = 80 (20 micron),
    Xmax = 99 (25 micro),
    Ymin = 0 (0 micron),
    Ymax = 99 (25 micron),
    Zmin = 0 (0 micron),
    Zmax = 99 (25 micron)

*Note:* The input parameters are *inclusive* and begin at *0*, so in the above example *0-99* covers the entire range of **Cells** in a given dimension.

## Global Position of Cropped Region

Figure 1 shows the position of the cropped region relative to the original image.

![Figure 1](Images/CropImageGeometry_1.png)

## Renumber Features

It is possible with this **Filter** to fully remove **Features** from the volume, possibly resulting in consistency errors if more **Filters** process the data in the pipeline. If the user selects to *Renumber Features* then the *Feature Ids* array will be adjusted so that all **Features** are continuously numbered starting from 1. The user should decide if they would like their **Features** renumbered or left alone (in the case where the cropped output is being compared to some larger volume).

The user has the option to save the cropped volume as a new **Data Container** or overwrite the current volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
