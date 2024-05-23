# Initialize Data

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** allows the user to define the data set in a _DataArray_.

This **Filter** provides several ways to do this:

- Fill Value:
  - This type is fairly straight forward you provide a value, or set of values for a multi-component array, and it gets copied into every tuple in the array.
  - Nuances to note:
    - None for this type aside from aforementioned boolean nuances
- Incremental:
  - This allows the user to supply a start value, define the type (increment or decrement), and set a step value to for the operation. Providing 0 will ensure that that component remains unchanged.
  - Example: Say you have a 3 component array where you want to define 3-D rotations in radians for different components, but you only want to operate on the x and y and not the z; You could define your starting value as '0' (ignoring the apostrophe marks), select addition as the operation, then define the step values like so: '3.141592;6.283185;0'
  - Boolean Nuances:
    - For the _Step Values_ please enter uint8 values, preferably a 0 or 1 only.
    - Addition:
      - Any step value that is greater than 0 will cause all values to be 'true' after the first tuple, 'true' values will remain unchanged.
      - If your start value is 'false' and step value > 0, the array will initialize to | false | true | true | ... |
      - If your start value is 'true' and step value > 0, the array will initialize to | true | true | true | ... |
    - Subtraction:
      - Any step value that is greater than 0 will cause all values to be 'false' after the first tuple, 'false' values will remain unchanged.
      - If your start value is 'true' and step value > 0, the array will initialize to | true | false | false | ... |
      - If your start value is 'false' and step value > 0, the array will initialize to | false | false | false | ... |
- Random:
  - This allows the user to randomly generate values from the actual scalar type min to the scalar type max. If you choose to define the seed for the array the seed in the _Seed Value_ box will be used, otherwise a random seed will be generated. Either way the seed will be saved to the _Stored Seed Value Array_.
  - **Standardizing Seed:**
    - This value allows the user to be able to generate the same sequence of values for every component in the tuple throughout the array when set to true
    - Example:
      - When true it will look like | 3;3;3 | 9;9;9 | 4;4;4 | ... |
      - When false it will look like | 3;9;4 | 7;2;8 | 5;9;6 | ... |
- Random with Range:
  - This is beholden to the same rules as random with some additional considerations in relation to the range.
  - If you require an array that has one stable component and other variable components you can define the lower and upper bounds of the range to be the same and the variable will never change.
  - Example: Say you have a 3 component array where you want the middle component to be always be 6 and the other component to be randomized above six. When defining a range supply '6' (ignoring the apostrophe marks) for the lower bound and '90;6;252' for the upper bound

The filter provides two ways to define the values in a multi-component array:

- Define a single value that will be used for every component in the tuple
- Defining each component's value seperated by the semicolon ';' character

Example: Say you have a 3 component array that you want to range that starts from zero and has unique end values for each component. In the starting value box just supply '0' (ignoring the apostrophe marks) and then define your end values in a list like this: '4;0;7'

**Boolean Entry Notes:**

The **ONLY** two ways to specify a 'false' boolean value are as follows:

- boolean value string types as follows ignoring apostrophe marks: 'False', 'FALSE', 'false'
- all well-formed integers and well-formed floating point definitions of 0

**ANY OTHER** string or number **WILL BE** 'true', although it is good practice to define true values as follows:

- boolean value string types as follows ignoring apostrophe marks: 'True', 'TRUE', 'true'
- all well-formed integers and well-formed floating point definitions of 1

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
