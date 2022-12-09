# Copy Data Object


## Group (Subgroup) ##

Core (Generation)

## Description ##

This **Filter** copies one or more DataObjects

## Parameters ##

None

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|----------------|
| DataObject | N/A | N/A | N/A | The list of DataObjects to copy. |
| bool | false | N/A | N/A | Whether to copy the DataObjects to a new parent or not. |
| DataGroup | N/A | N/A | N/A | The group to be used as the parent for all the DataObject copies if the Copy to New Parent option is selected |
| string | _COPY | N/A | N/A | The suffix string to be appended to each copy's name |

## Created Objects ##

A deep copy of the DataObjects selected in the input.

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)
