# Avizo Uniform Coordinate Writer  

## Group (Subgroup)

IO (Output)

## Description

This filter writes out a native Avizo Uniform Coordinate data file. Values should be present from segmentation of experimental data or synthetic generation and cannot be determined by this filter. Not having these values will result in the filter to fail/not execute.

### Example Output

    # AmiraMesh BINARY-LITTLE-ENDIAN 2.1
    # Dimensions in x-, y-, and z-direction
    define Lattice
    define Coordinates
    Parameters {
        DREAM3DParams {
            Author "DREAM3D",
             DateTime     
        }
        Units {
              Coordinates "microns"
        }
         Content "   int, uniform coordinates",
         # Bounding Box is xmin xmax ymin ymax zmin zmax
         BoundingBox      ,
     CoordType "uniform"
    }
    Lattice { int FeatureIds } = @1
    # Data section follows
       .. 

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
