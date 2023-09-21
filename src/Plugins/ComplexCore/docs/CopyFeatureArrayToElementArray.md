# Create Element Array from Feature Array 


## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** copies the values associated with a **Feature** to all the **Elements** that belong to that **Feature**.  Xmdf visualization files write only the **Element** attributes, so if the user wants to display a spatial map of a **Feature** level attribute, this **Filter** will transfer that information down to the **Element** level.

## Parameters ##

None

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Feature **Attribute Array**  | None | Any | Any | Feature** data to copy to **Element** data  |
| Element **Attribute Array | FeatureIds  | int32_t | (1) | Specifies to which **Feature** each **Element** belongs  |


## Created Objects ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Element **Attribute Array | None | Any | Any | Copied **Attribute Array**  |


## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


