# Crop Geometry (Vertex)

## Group (Subgroup)

DREAM3D Review (Cropping/Cutting)

## Description

This **Filter** crops a **Vertex Geometry** by the given bounding box.  Unlike the cropping of an Image, it is unknown until run time how the **Geometry** will be changed by the cropping operation.  Therefore, this **Filter** requires that a new **Geometry** be created to contain the cropped **Vertex Geometry**.  The user-selected **Vertex** data arrays will be copied, with tuples *removed* for any **Vertices** outside the bounding box.  The user must supply a name for the cropped **Geometry**, but all other copied objects will retain the same names as the original source.

*Note:* Since it cannot be known before run time how many **Vertices** will be removed during cropping, the new **Vertex Geometry** and all associated **Vertex** data to be copied will be initialized to have size 0.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.
