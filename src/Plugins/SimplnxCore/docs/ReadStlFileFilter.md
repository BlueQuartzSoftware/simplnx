# Read STL File

## Group (Subgroup)

IO (Input)

## Description

An STL file describes a 3D surface as a list of triangles. This filter reads a **binary** STL file from disk and creates a **Triangle Geometry** (a surface mesh made of triangular faces) in memory, along with a *Face Normals* array (one 3-component normal vector per triangle) and, optionally, a *Face Labels* array.

The vertex coordinates in an STL file are plain numbers with no embedded unit. They are in whatever length unit the file's author intended (often millimeters or inches), so the reader treats them as dimensionless and copies them through unchanged.

**Only binary STL files are supported. ASCII STL files are not read by this filter; the filter will report an error if given one.** An ASCII STL file can be converted to binary using a separate 3D mesh tool before reading.

An explanation of the STL file format can be found on [Wikipedia](https://en.wikipedia.org/wiki/STL_(file_format)). The binary structure is:

    UINT8[80]     Header
    UINT32        Number of triangles

    foreach triangle
      REAL32[3]     Normal vector
      REAL32[3]     Vertex 1
      REAL32[3]     Vertex 2
      REAL32[3]     Vertex 3
      UINT16        Attribute byte count
    end

The filter inspects the header to try to determine the vendor of the STL file, because some vendors do not write files that strictly follow the specification.

## IMPORTANT NOTES

The reader follows the STL specification strictly and obeys the "Attribute byte count" value written after each triangle. If you are writing an STL file, set the "Attribute byte count" to *zero* (0) unless you are deliberately encoding extra data after each triangle, in which case the count must exactly match the number of extra bytes. An incorrect "Attribute byte count" is the most common cause of read failures, because the reader will skip the number of bytes the file claims are present.

## Known Vendors Who Write Out-of-Spec STL Files

- Materialise Magics [https://www.materialise.com/en/industrial/software/magics-data-build-preparation](https://www.materialise.com/en/industrial/software/magics-data-build-preparation)

    The filter looks in the header for "COLOR=" and "MATERIAL=" strings.

- Creaform VXelements [https://www.creaform3d.com/en/metrology-solutions/3d-applications-software-platforms](https://www.creaform3d.com/en/metrology-solutions/3d-applications-software-platforms)

    The filter looks for "VXelements" in the header.

## Code to Convert

If you have a non-conforming STL file that was not made by one of the vendors above, the Python snippet below can clean it up. It assumes the **ONLY** problem with the file is that the trailing UINT16 "Attribute byte count" value for each triangle needs to be set to zero.

        import struct

        def modify_stl(input_file_path, output_file_path):
            with open(input_file_path, 'rb') as input_file, open(output_file_path, 'wb') as output_file:
                # Read and copy header
                header = input_file.read(80)
                output_file.write(header)

                # Read number of triangles
                num_triangles = struct.unpack('<I', input_file.read(4))[0]
                output_file.write(struct.pack('<I', num_triangles))

                # Define the format for one triangle (50 bytes total)
                triangle_format = '<12fH'
                triangle_size = struct.calcsize(triangle_format)

                # Process each triangle
                for _ in range(num_triangles):
                    # Read triangle data
                    triangle_data = input_file.read(triangle_size)

                    # Unpack and modify the last 2 bytes (attribute byte count)
                    data = list(struct.unpack(triangle_format, triangle_data))
                    data[-1] = 0  # Set the attribute byte count to zero

                    # Repack and write the modified triangle data
                    modified_triangle_data = struct.pack(triangle_format, *data)
                    output_file.write(modified_triangle_data)

        # Example usage
        input_stl_path = '/path/to/input.stl'  # Specify the input file path
        output_stl_path = '/path/to/input_FIXED.stl'  # Specify the output file path

        modify_stl(input_stl_path, output_stl_path)

### Required Input Sources

None — this filter reads directly from a binary `.stl` file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
