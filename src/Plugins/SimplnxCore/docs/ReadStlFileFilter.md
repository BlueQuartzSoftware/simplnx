# Read STL File

## Group (Subgroup)

IO (Input)

## Description

This **Filter**  will read a binary STL File and create a **Triangle Geometry** object in memory. The STL reader is very strict to the STL specification. An explanation of the STL file format can be found on [Wikipedia](https://en.wikipedia.org/wiki/STL). The structure of the file is as follows:

    UINT8[80]     Header
    UINT32     Number of triangles

    foreach triangle
      REAL32[3]     Normal vector
      REAL32[3]     Vertex 1
      REAL32[3]     Vertex 2
      REAL32[3]     Vertex 3
      UINT16     Attribute byte count
    end

The filter will look for specific header information to try and determine the vendor of the STL file. Certain vendors do not write STL files that adhere to the file spec.

## IMPORANT NOTES:

**It is very important that the "Attribute byte Count" is correct as DREAM3D-NX follows the specification strictly.** If you are writing an STL file be sure that the value for the "Attribute byte count" is *zero* (0). If you chose to encode additional data into a section after each triangle then be sure that the "Attribute byte count" is set correctly. DREAM3D-NX will obey the value located in the "Attribute byte count".

## Known Vendors who Write out of spec STL Files

- Materialise Magics [https://www.materialise.com/en/industrial/software/magics-data-build-preparation](https://www.materialise.com/en/industrial/software/magics-data-build-preparation)

    The filter looks in the header for "COLOR=" and "MATERIAL=" strings in the header.

- Creaform VXelements [https://www.creaform3d.com/en/metrology-solutions/3d-applications-software-platforms](https://www.creaform3d.com/en/metrology-solutions/3d-applications-software-platforms)
    
    The filter looks for "VXelements" in the header.

## Code to convert

If you find yourself in a situation where the STL File is non-conforming and is not made by one of the vendors above, this bit of Python
code can clean up the file. This makes the absolute assumption that the **ONLY** thing wrong with the STL file is that the trailing UINT16 value for
each triangle needs to be set to ZERO.

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




% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
