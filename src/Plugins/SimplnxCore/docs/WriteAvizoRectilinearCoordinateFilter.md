# Write Avizo Rectilinear Coordinate

## Group (Subgroup)

IO (Output)

## Description

This filter writes out a native Avizo Rectilinear Coordinate data file. Values should be present from segmentation of experimental data or synthetic generation and cannot be determined by this filter. Not having these values will result in the filter to fail/not execute.

### Example Output

    # AmiraMesh BINARY-LITTLE-ENDIAN 2.1
    # Dimensions in x-, y-, and z-direction
    define Lattice
    define Coordinates
    Parameters {
        DREAM3DParams {
            Author "DREAM3D",
             DateTime "    . M o n   J u n   1   1 0 : 0 1 : 1 4   2 0 1 5   "     
        }
        Units {
              Coordinates "microns"
        }
         CoordType "rectilinear"
    }
         
    Lattice { int FeatureIds } = @1
    Coordinates { float xyz } = @2
    # Data section follows
       .. 

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
