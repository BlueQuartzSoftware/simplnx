# Read Raw Binary

## Group (Subgroup)

IO (Input)

## Description

This **Filter** is designed to read data stored in files on the users system in *binary* form. The data file should **not** have any type of header before the data in the file (or the header must be skipped using the *Skip Header Bytes* parameter). The user should know exactly how the data is stored in the file and properly define this in the user interface. Not correctly identifying the type of data can cause serious issues since this **Filter** is simply reading the data into a pre-allocated array interpreted as the user defines.

This **Filter** will error out and block the **Pipeline** from running if the total number of bytes that would need to be read from the file is larger than the actual file itself. The user can use an input file that is actually **larger** than the number of bytes required by the **Filter**; in this case, the **Filter** will only read the first part of the file unless an amount of bytes to skip is set.

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

### A Note about Tuple and Component Dimensions

The tuple dimensions define the shape of the output **Data Array**. For example, a 3D volume with 100 x 200 x 50 voxels would have tuple dimensions of `50, 200, 100` (slowest to fastest, i.e., Z, Y, X).

When creating the output array inside an **Attribute Matrix**, the tuple dimensions are automatically inherited from the Attribute Matrix shape. In this case, the *Set Tuple Dimensions* checkbox can be unchecked to hide the tuple dimensions entry table.

If the output array is **not** inside an Attribute Matrix, then the user **must** check *Set Tuple Dimensions* and provide the dimensions explicitly.

This parameter tells the program how many values are present for each *tuple* and how they are organized. The component dimensions are specified as a table of values (slowest to fastest dimension).

Examples:

- **Scalar data**: A single component dimension of `1`. A grayscale image would typically have just a single value of type unsigned 8 bit integer at every pixel/voxel.
- **Vector data**: A single component dimension of `3`. A color image has 3 components for red (R), green (G) and blue (B). Euler angles are also typically stored as 3 component vectors of 32 bit floating point values.
- **Multi-dimensional components**: Multiple component dimensions such as `3, 3` for a 3x3 tensor (9 total components per tuple).

### Endian

This parameter tells the program which byte is *most significant* for multibyte values. Intel architecture computers are little endian while Power PC, Sun Sparc and DEC Alpha CPUs are big endian. Consider the following example:

**Byte Ordering Example for 32 Bit Signed Integer**

| Byte 0 | Byte 1 | Byte 2 | Byte 3 | Interpretation |
|---|---|---|---|----------------|
| FF | AA | 00 | 00 | -5636096 (Big Endian) |
| 00 | 00 | AA | FF | 43775 (Little Endian) |

This setting is *crucial* to the correct interpretation of the binary data, so the user must be aware of how their binary data was encoded.

The *Endian* parameter provides the following choices:

- **Little [0]**: Little endian byte order (least significant byte first). Used by Intel/AMD x86 and x86-64 architectures.
- **Big [1]**: Big endian byte order (most significant byte first). Used by network protocols, older RISC architectures, and some scientific instruments.

### Skip Header Bytes

If the raw binary file you are reading has a *header* before the actual data begins, the user can instruct the **Filter** to skip this header portion of the file. The user needs to know how long the header is in bytes. Another way to use this value is if the user wants to read data out of the interior of a file by skipping a defined number of bytes.

### Output Placement

The output array can be placed in two ways:

1. **Inside an Attribute Matrix**: Select an output path that is a child of an existing Attribute Matrix (e.g., `ImageGeom/CellData/MyArray`). The tuple dimensions are inherited from the Attribute Matrix automatically. The file must contain at least enough data to fill the array (tuples x components x scalar type size bytes, after skipping any header bytes).

2. **Standalone**: Select an output path that is not inside an Attribute Matrix (e.g., `MyArray`). The user must provide explicit tuple dimensions via the *Set Tuple Dimensions* checkbox and table.

### Required Input Sources

None — this filter reads directly from a raw binary file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
