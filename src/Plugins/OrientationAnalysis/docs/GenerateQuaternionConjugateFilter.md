# Generate Quaternion Conjugate

## Group (Subgroup) ##

Processing (OrientationAnalysis)

## Description ##

This filter will generate the transpose of a [1x4] _Quaternion_ laid out in memory such that < x, y, z >, w. This can be
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

| Kind                | Default Name     | Type  | Component Dimensions | Description |
|---------------------|------------------|-------|----------------------|-------------|
| **Attribute Array** | Quaternion Input | float | (4)                  |             |

## Created Objects ##

| Kind                | Default Name     | Type  | Component Dimensions | Description |
|---------------------|------------------|-------|----------------------|-------------|
| **Attribute Array** | Quaternion Ouput | float | (4)                  |             |

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: GenerateQuaternionConjugateFilter
+ Displayed Name: Generate Quaternion Conjugate

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| delete_original_data | Delete Original Data | Should the original Data be deleted from the DataStructure | complex.BoolParameter |
| output_data_array_path | Output Data Array Path | The name of the generated output DataArray | complex.DataObjectNameParameter |
| quaternion_data_array_path | Quaternions | Specifies the quaternions to convert | complex.ArraySelectionParameter |

