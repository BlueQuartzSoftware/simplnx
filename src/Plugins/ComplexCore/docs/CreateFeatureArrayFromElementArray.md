# Create Feature Array From Element Array 


## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** copies all the associated **Element** data of a selected **Element Array** to the **Feature** to which the **Elements** belong. The value stored for each **Feature** will be the value of the _last element copied_. 

## Parameters ##

None

## Required Geometry ##

Not Applicable


## Required Objects ##

| Kind  | Type | Component Dimensions | Description |
|------|------|----------------------|-------------|
| Element Array DataPath | Any | Any | **Element** data to copy to **Feature** data |
| Element Array DataPath | int32_t | (1) | Specifies to which **Feature** each **Element** belongs |


## Created Objects ##

| Kind | Type | Component Dimensions | Description |
|------|------|----------------------|-------------|
| Feature Array DataPath | Any  | Any | The copied array containing **Feature** data |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


