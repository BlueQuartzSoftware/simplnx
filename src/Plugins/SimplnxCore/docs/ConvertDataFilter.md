# Convert AttributeArray DataType

## Group (Subgroup)

Core (Misc)

## Description

This **Filter** converts attribute data from one primitive type to another by using the built in translation of the compiler. This **Filter** can be used if the user needs to convert an array into a type that is accepted by another **Filter**. For example, a **Filter** may need an input array to be of type _int32_t_ but the array that the user would like to use is _uint16_t_. The user may use this **Filter** to create a new array that has the proper target type (_int32_t_).

### Scalar Type

The *Scalar Type* parameter selects the target primitive data type for the converted array:

- **int8**: Signed 8-bit integer (-128 to 127)
- **uint8**: Unsigned 8-bit integer (0 to 255)
- **int16**: Signed 16-bit integer (-32,768 to 32,767)
- **uint16**: Unsigned 16-bit integer (0 to 65,535)
- **int32**: Signed 32-bit integer (-2,147,483,648 to 2,147,483,647)
- **uint32**: Unsigned 32-bit integer (0 to 4,294,967,295)
- **int64**: Signed 64-bit integer
- **uint64**: Unsigned 64-bit integer
- **float32**: 32-bit floating point
- **float64**: 64-bit floating point (double precision)
- **boolean**: Boolean (0 = false, any non-zero value = true)

**This Filter is here for convenience and should be used with great care and understanding of the input and output data. This Filter should rarely be required, and if the user thinks that they require this Filter then a detailed examination of all the data involved should be undertaken to avoid possible undefined behaviors.**

### Important Notes

**Up Casting**

Upcasting is most likely well defined by the compilers. This is the act of creating a new array using a primitive value that is represented by more bytes than the original data. For example, if the user converts 1 byte integers into 4 byte integers or converted 4 byte floats into 8 byte floats.

**Down Casting**

Down casting can have undefined behavior depending on the primitive types involved. Down casting is the opposite of up casting and involves converting data from a larger byte count representation to a representation of lower byte count. For example, converting 4 byte integers into 2 byte integers or 8 byte floats into 4 byte floats. What happens to the data depends on the range of values in the original array. If the target type's range can hold all the values of the original array's values, then the conversion would have a well defined outcome.

**Signed/Unsigned Conversions**

When converting data from signed values to unsigned values or vice-versa, there can also be undefined behavior. For example, if the user were to convert a signed 4 byte integer array to an unsigned 4 byte integer array and the input array has negative values, then the conversion rules are undefined and may differ from operating system to operating system.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
