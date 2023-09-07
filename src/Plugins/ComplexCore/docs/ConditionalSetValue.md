# Replace Value in Array (Conditional)

## Group (Subgroup) ##

Core (Misc)

## Description ##

This **Filter** replaces values in a user specified **Attribute Array** with a user specified value a second boolean **Attribute Array** specifies, but only when **Use Conditional Mask** is *true*. For example, if the user entered a *Replace Value* of *5.5*, then for every occurence of *true* in the conditional boolean array, the selected **Attribute Array** would be changed to 5.5. If **Use Conditional Mask** is *false*, then **Value to Replace** will be searched for in the provided **Attribute Array** and all instances will be replaced. Below are the ranges for the values that can be entered for the different primitive types of arrays (for user reference). The selected **Attribute Array** must be a scalar array.
    
### Primitive Data Types ##

| Type             | Size |        Range       |
|------------------|------|--------------------|
| Signed Integer | 8 bit |0 to 255|
| Unsigned Integer | 8 bit |-128 to 127|
| Signed Integer | 16 bit |-32,768 to 32,767|
| Unsigned Integer | 16 bit |0 to 65,535|
| Signed Integer | 32 bit |-2,147,483,648 to 2,147,483,647|
| Unsigned Integer | 32 bit |0 to 4,294,967,295|
| Signed Integer | 64 bit |   9,223,372,036,854,775,808 to 9,223,372,036,854,775,807|
| Unsigned Integer | 64 bit |0 to 18,446,744,073,709,551,615|
| Float | 32 bit | -3.4e+38 to -1.1e-38, 0.0, 1.1e-38 to 3.4e+38 (7 digits)|
| Double | 64 bit | -1.7e+308 to -2.2e-308, 0.0, 2.2e-308 to 1.7e+308 (15 digits)|
| Boolean | 8 bit |0 = false and any other value will be forced to 1 = true|

## Parameters ##

| Name             | Type | Description |
|------------------|------|-------------|
| New Value | std::string | Value to replace the removed values in the array [will be typecasted to appropriate value later] |

## Optional Data Mask ##
| Name             | Type | Description |
|------------------|------|-------------|
| Use Conditional Mask | bool | Whether to use a boolean mask array to replace values marked true |
| Invert Mask | bool | If checked values marked FALSE will be replaced instead |
| Any **Attribute Array** | None | Bool | (1) | The complete path to the conditional array that will determine which values/entries will be replaced if index is true|
| Value to Replace | std::string | The numerical value that will be replaced in the array [will be typecasted to appropriate value later] |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|----------------|
| Any **Attribute Array** | None | Any | (1) | Path to **Attribute Array** that will have values replaced |

## Created Objects ##

None

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


