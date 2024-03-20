# Read Raw Binary File

## Group (Subgroup)

IO (Input)

## Description

This **Filter** is designed to read data stored in files on the users system in *binary* form. The data file should **not** have any type of header before the data in the file. The user should know exactly how the data is stored in the file and properly define this in the user interface. Not correctly identifying the type of data can cause serious issues since this **Filter**  is simply reading the data into a pre-allocated array interpreted as the user defines.

This **Filter**  will error out and block the **Pipeline** from running if the total number of bytes that would need to be read from the file is larger than the actual file itself. The user can use an input file that is actually **larger** than the number of bytes required by the **Filter**; in this case, the **Filter**  will only read the first part of the file unless an amount of bytes to skip is set.

### Scalar Type

Computer data comes in 10 basic types on modern 32 bit and 64 bit operating systems. Data can be categorized as either *integer* or *floating point*. With each of these types, the number of bits that represent the data determine their maximum and minimum values. For integer values, the standard types are 8, 16, 32 and 64 bit (1, 2, 4, and 8 bytes). For floating point values, there are either 32 bit or 64 bit (4 or 8 bytes). Integer types can be either *signed* or *unsigned*. A signed integer can take negative values. An unsigned integer can only take positive values, but will have twice the positive value range as a signed integer.

The types of data that can be read with this **Filter** include:

    signed Int8
    unsigned UInt8
    signed Int16
    unsigned UInt16
    signed Int32
    unsigned UInt32
    signed Int64
    unsigned UInt64
    Float 32 bit
    Double 64 bit

---

### Number of Components

This parameter tells the program how many values are present for each *tuple*. For example, a grayscale image would typically have just a single value of type unsigned 8 bit integer at every pixel/voxel. A color image will have at least 3 components for red (R), breen (G) and blue (B), and sometimes 4 values if the alpha (A) channel is also stored. Euler angles are typically stored as a 3 component vector of 32 bit floating point values.

### Endian

This parameter tells the program which byte is *most significant* for multibyte values. Intel architecture computers are little endian while Power PC, Sun Sparc and DEC Alpha CPUs are big endian. Consider the following example:

**Byte Ordering Example for 32 Bit Signed Integer**

| Byte 0 | Byte 1 | Byte 2 | Byte 3 | Interpretation |
|---|---|---|---|----------------|
| FF | AA | 00 | 00 | -5636096 (Big Endian) |
| 00 | 00 | AA | FF | 43775 (Little Endian) |

This setting is *crucial* to the correct interpretation of the binary data, so the user must be aware of how their binary data was encoded.

### Skip Header Bytes

If the raw binary file you are reading has a *header* before the actual data begins, the user can instruct the **Filter** to skip this header portion of the file. The user needs to know how lond the header is in bytes. Another way to use this value is if the user wants to read data out of the interior of a file by skipping a defined number of bytes.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
