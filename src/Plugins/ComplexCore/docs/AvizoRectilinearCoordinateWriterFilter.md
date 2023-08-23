# Avizo Rectilinear Coordinate Writer  


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

## Parameters 

| Name | Type | Description |
|------|------| ----------- |
| Output File | File Path | Amira Mesh .am file created|
| Write Binary File | Boolean | Binary file created, if checked |

## Required Geometry 

Image 

## Required Objects 

| Type | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| Cell | FeatureIds | Ids (Int) that specify to which **Feature** each **Cell** belongs. | (1) | Filters Known to Create Data: Segment Features (Misorientation, C-Axis Misorientation, Scalar) (Reconstruction), Read Dx File (IO), Read Ph File (IO), Pack Primary Phases (SyntheticBuilding), Insert Precipitate Phases (SyntheticBuilding), Establish Matrix Phase (SyntheticBuilding)


## Created Objects 

None

## Example Pipelines 



## License & Copyright 

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: AvizoRectilinearCoordinateWriterFilter
+ Displayed Name: Avizo Rectilinear Coordinate Exporter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| feature_ids_array_path | Feature Ids | Data Array that specifies to which Feature each Element belongs | complex.ArraySelectionParameter |
| geometry_path | Image Geometry | The path to the input image geometry | complex.GeometrySelectionParameter |
| output_file | Output File | Amira Mesh .am file created | complex.FileSystemPathParameter |
| units | Units | The units of the data | complex.StringParameter |
| write_binary_file | Write Binary File | Whether or not to write the output file as binary | complex.BoolParameter |

