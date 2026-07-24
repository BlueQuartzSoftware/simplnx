# Write Binary Data

## Group (Subgroup)

IO (Output) (Write) (Export) (Binary)

## Description

This filter writes each selected **Data Array** to its own raw binary file in the chosen output directory. One file is written per selected array.

### How It Works

For each array, the filter writes the array's numeric values straight to disk as raw bytes. **No header is written** — the file contains only the array values with no metadata describing the data type, dimensions, byte order, or component layout. Because of this, the user must independently record the data type (for example *float32*, *int32*, *uint8*), the tuple and component dimensions, and the endianness chosen below. That information is required later to read the file back correctly.

The values are written in the same order they are stored in memory: tuple by tuple, with the components of each tuple written together (interleaved). For example, a 3-component array is written as `t0c0, t0c1, t0c2, t1c0, t1c1, t1c2, ...`.

### File Naming

Each output file is named after the array it contains, followed by the value of the *File Extension* parameter, and is placed in the *Output Path* directory. For example, an array named `Confidence Index` written with the default extension `.bin` produces a file named `Confidence Index.bin` in the output directory.

### Reading the Files Back

These raw binary files can be read back into DREAM3D-NX with the [Read Raw Binary](ReadRawBinaryFilter.md) filter. Because no header was written, the user must supply the data type, dimensions, and endianness to that filter manually — they must match the values used when the file was written.

### Endianess

The *Endianess* parameter controls the byte order used when writing numeric values to the binary file:

- *Little Endian*: The least significant byte is stored first. This is the native byte order for x86 and ARM processors and is the most common choice for files consumed on modern desktop systems.
- *Big Endian*: The most significant byte is stored first. This byte order is used by some network protocols and legacy systems.

## Required Input Sources

The arrays to export are user-selected **Data Arrays** from anywhere in the **Data Structure**; there is no specific upstream producer filter required.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
