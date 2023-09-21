# Generate Quaternion Conjugate

## Group (Subgroup) ##

Processing (OrientationAnalysis)

## Description ##

This filter will generate the transpose of a [1x4] *Quaternion* laid out in memory such that < x, y, z >, w. This can be
handy when the user wants to convert the orientation transformation to an opposite effect. The algorihtm will calculate
the conjugate of each quaternion in the array of input quaternions

**NOTES**: Internally DREAM.3D assumes that the internal reference transformation is a **Sample to Crystal**
transformation. If the incoming data was collected in such a way that the orientation representation that is stored (
Quats, Eulers, Orientation Matrix, Rodrigues) was assumed to be a **Crystal to Sample** transformation then this filter
can be applied to a quaternion to convert into a reference frame that DREAM.3D assumes.

## Parameters ##

| Name                  | Type    | Description                                                                                       |
|-----------------------|---------|---------------------------------------------------------------------------------------------------|
| Delete Original Array | Boolean | Set this to TRUE/ON to have the original 3 component data array deleted at the end of the filter. |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind                | Default Name     | Type  | Comp Dims | Description |
|---------------------|------------------|-------|-------------|-------------|
| Attribute Array | Quaternion Input | float | (4)                  |             |

## Created Objects ##

| Kind                | Default Name     | Type  | Comp Dims | Description |
|---------------------|------------------|-------|-------------|-------------|
| Attribute Array | Quaternion Ouput | float | (4)                  |             |

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


