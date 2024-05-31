# Slice Triangle Geometry

## Group (Subgroup)

Sampling (Geometry)

## Description

This **Filter** slices an input **Triangle Geometry**, producing an **Edge Geometry**.  The user can control the range over which to slice (either the entire range of the geoemtry or a specified subregion), and the spacing bewteen slices. Currently this filter only supports slicing along the direction of the z axis. The total area and perimieter of each slice is also computed and stored as an attribute on each created slice.

Additionally, if the input **Triangle Geometry** is labeled with an identifier array (such as different regions or features), the user may select this array and the resulting edges will inherit these identifiers.

% Auto generated parameter table will be inserted here

## Example Pipelines

CreateScanVectors

## License & Copyright 

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users