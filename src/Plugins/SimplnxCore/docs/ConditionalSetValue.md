# Replace Value in Array (Conditional)

## Group (Subgroup)

Core (Misc)

## Description

This **Filter** replaces values in a user specified **Attribute Array** with a user specified value a second boolean **Attribute Array** specifies, but only when **Use Conditional Mask** is *true*. For example, if the user entered a *Replace Value* of *5.5*, then for every occurence of *true* in the conditional boolean array, the selected **Attribute Array** would be changed to 5.5. If **Use Conditional Mask** is *false*, then **Value to Replace** will be searched for in the provided **Attribute Array** and all instances will be replaced. Below are the ranges for the values that can be entered for the different primitive types of arrays (for user reference). The selected **Attribute Array** must be a scalar array.

### Primitive Data Types

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

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
