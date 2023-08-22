# Create Data Array

## Group (Subgroup) ##

Core (Generation)

## Description ##

This **Filter** creates an **Data Array** of any primitive type with any number of components along a _single component dimension_. For example, a scalar as (1) or a 3-vector as (3), but _not_ a matrix as (3, 3). The array is initialized to a user define value or with random values within a specified range.

When initializing a multicomponent array square bracket notation can be used to specify different initialization values for each component. For example say that I want to intialize a 2 component array where the first component is 0 and the second component is 1 we would use the following input string for the *Initialization Value*

    0;1

We are using semicolons instead of commas or decimal points due to different international standards (European versus United States?).

Another example is if you want to create a floating point array where each tuple has 10 components but you just want the value of 2.5 to be used for each, then simply use:

    2.5

When creating a Data Array within an Attribute matrix, the tuple dimensions will **always** be taken direct from the Attribute Matrix. This means that the *Set Tuple Dimensions* parameter can be unchecked to hide the tuple dimensions entry table. 

If the parent is **NOT an Attribute Matrix**, then the user ***MUST*** set the tuple dimensions themselves.

### Scalar Type Values ###

    static const int Int8 = 0;
    static const int UInt8 = 1;
    static const int Int16 = 2;
    static const int UInt16 = 3;
    static const int Int32 = 4;
    static const int UInt32 = 5;
    static const int Int64 = 6;
    static const int UInt64 = 7;
    static const int Float = 8;
    static const int Double = 9;
    static const int Bool = 10;

### Primitive Data Type Valid Ranges ##

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

The number of components should be at least 1. Examples of _Number of Components_ would be 3 for an RGB Image, 1 for a gray scale image, 1 for a scalar array, 4 for a quaternions array, etc. All values of the array will be initialized to the user set value. The initialization value text box
must have a user entry or the default value _0_ will be used.

## Parameters ##

| Name             | Type | Description |
|------------------|------|-------------|
| Set Tuple Dimensions | bool | This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix |
| Numeric Type | Enumeration | Primitive data type for created array |
| Number of Components | int32_t | The number of components that each tuple contains. Matrix are row major form within SIMPL|
| Initialization Value | String | Initialization value for array |
| Data Format | String | This value will specify which data format is used by the array's data store. An empty string results in in-memory data store. |

## Required Geometry ##

Not Applicable

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|----------------|
| Any **Attribute Array** | None | Any | Any | Created **Attribute Array** location and name |

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: CreateDataArray
+ Displayed Name: Create Data Array

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| advanced_options | Set Tuple Dimensions [not required if creating inside an Attribute Matrix] | This allows the user to set the tuple dimensions directly rather than just inheriting them 

This option is NOT required if you are creating the Data Array in an Attribute Matrix | complex.BoolParameter |
| component_count | Number of Components | Number of components | complex.UInt64Parameter |
| data_format | Data Format | This value will specify which data format is used by the array's data store. An empty string results in in-memory data store. | complex.DataStoreFormatParameter |
| initialization_value | Initialization Value | This value will be used to fill the new array | complex.StringParameter |
| numeric_type | Numeric Type | Numeric Type of data to create | complex.NumericTypeParameter |
| output_data_array | Created Array | Array storing the data | complex.ArrayCreationParameter |
| tuple_dimensions | Data Array Dimensions (Slowest to Fastest Dimensions) | Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry. | complex.DynamicTableParameter |

