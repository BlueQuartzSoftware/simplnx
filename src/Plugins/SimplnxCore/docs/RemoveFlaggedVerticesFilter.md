# Remove Flagged Vertices

## Group (Subgroup)

Data Cleanup (Geometry)

## Description

This **Filter** removes **Vertices** from the supplied **Vertex Geometry** that are flagged as **TRUE** by a boolean mask array. A new reduced **Vertex Geometry** is created that contains all the remaining **Vertices**. It is unknown until run time how many **Vertices** will be removed from the **Geometry**. Therefore, this **Filter** requires that a new **Data Container** be created to contain the reduced **Vertex Geometry**. This new **Vertex Geometry** will contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Data Container**. Additionally, _all **Vertex** data will be copied_, with tuples *removed* for any **Vertices** removed by the **Filter**. The user must supply a name for the reduced **Vertex Geometry**, but all other copied objects (**Attribute Matrices** and **Attribute Arrays**) will retain the same names as the original source.

*Note:* Since it cannot be known before run time how many **Vertices** will be removed, the new **Vertex Geometry** and all associated **Vertex** data to be copied will be initialized to have size 0. Any **Feature** or **Ensemble** information will retain the same dimensions and size.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
