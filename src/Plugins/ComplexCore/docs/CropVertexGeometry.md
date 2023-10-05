# Crop Geometry (Vertex)

## Group (Subgroup)

DREAM3D Review (Cropping/Cutting)

## Description

This **Filter** crops a **Vertex Geometry** by the given bounding box.  Unlike the cropping of an Image, it is unknown until run time how the **Geometry** will be changed by the cropping operation.  Therefore, this **Filter** requires that a new **Data Container** be created to contain the cropped **Vertex Geometry**.  This new **Data Container** will contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Data Container**.  Additionally, all **Vertex** data will be copied, with tuples *removed* for any **Vertices** outside the bounding box.  The user must supply a name for the cropped **Data Container**, but all other copied objects (**Attribute Matrices** and **Data Arrays**) will retain the same names as the original source.

*Note:* Since it cannot be known before run time how many **Vertices** will be removed during cropping, the new **Vertex Geometry** and all associated **Vertex** data to be copied will be initialized to have size 0.  Any **Feature** or **Ensemble** information will retain the same dimensions and size.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.
