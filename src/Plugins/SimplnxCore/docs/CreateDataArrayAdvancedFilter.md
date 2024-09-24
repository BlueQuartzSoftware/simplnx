# Create Data Array (Advanced)

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** creates a **Data Array** of any primitive type with any set of component dimensions.  The array is initialized to a user defined value or with random values within a specified range.

When initializing a multicomponent array square bracket notation can be used to specify different initialization values for each component. For example say that I want to intialize a 2 component array where the first component is 0 and the second component is 1 we would use the following input string for the *Initialization Value*

    0;1

We are using semicolons instead of commas or decimal points due to different international standards (European versus United States?).

Another example is if you want to create a floating point array where each tuple has 10 components but you just want the value of 2.5 to be used for each, then simply use:

    2.5

When creating a Data Array within an Attribute matrix, the tuple dimensions will **always** be taken direct from the Attribute Matrix. This means that the *Set Tuple Dimensions* parameter can be unchecked to hide the tuple dimensions entry table.

If the parent is **NOT an Attribute Matrix**, then the user ***MUST*** set the tuple dimensions themselves.

### Scalar Type Values

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

### Primitive Data Type Valid Ranges

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

The component dimensions should multiply together into a total number of components equal to at least 1. Examples of *Component Dimensions* would be [3] for an RGB Image, [1] for a gray scale image, [1] for a scalar array, [4] for a quaternions array, [10x5] for an array with 10x5 grids at each tuple, etc.  All values of the array will be initialized using the chosen initialization option.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
