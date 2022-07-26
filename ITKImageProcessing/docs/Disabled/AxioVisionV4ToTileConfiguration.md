# AxioVision V4 To Tile Configuration #


## Group (Subgroup) ##

Processing (Conversion)

## Description ##

This **Filter** will create a "Tile Configuration" file based off of a Zeiss AxioVision Version 4 *_meta.xml file. The Tile Configuration file is used by FIJI to stitch a montage of images into a single image.

An example file that is written by the filter is as follows:

    # File Written by DREAM.3D based on values from the _meta.xml
    # Define the number of dimensions we are working on.
    dim = 2

    # Define the image coordinates
    05MAR09_run2_64-Raw_p0.bmp; ; (0, 0)
    05MAR09_run2_64-Raw_p1.bmp; ; (1227.55, 0)
    05MAR09_run2_64-Raw_p3.bmp; ; (0.23675, 920.01)
    05MAR09_run2_64-Raw_p2.bmp; ; (1227.55, 919.774)
    05MAR09_run2_64-Raw_p4.bmp; ; (0.23675, 1839.55)
    05MAR09_run2_64-Raw_p5.bmp; ; (1227.31, 1839.55)

Even though the values written are in floating point notation, the values are in **pixel** units.

## Parameters ##

| Name | Type | Description |
|------|------|------|
| Input File | String | Path to the _meta.xml file |
| Output File | String | Path and filename to where the file will be saved |

## Required Geometry ##

Not Applicable

## Required Objects ##

Not Applicable

## Created Objects ##

None 

## Example Pipelines ##

This is used in the ZeissImport Unit Test

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users