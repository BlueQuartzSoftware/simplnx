# Create Ensemble Info


## Group (Subgroup) ##

Processing (Generation)

## Description ##

This **Filter** allows the user to enter basic crystallographic information about each phase. The Laue class, Phase Type, and Phase Name can all be entered by the user. The information is stored in an EnsembleAttributeMatrix. These values are needed to allow the calculation of certain kinds of crystallographic statistics on the volume, if they have not already been provided by some other means. Each row in the table lists the __Crystal Structure__, __Phase Type__, and __Phase Name__. The proper values for the crystal structure and phase type come from internal constants within DREAM.3D and are listed here:

### Crystal Structure

| String Name | Internal Value | Laue Name |
| ------------|----------------|----------|
| Hexagonal_High | 0 |  Hexagonal-High 6/mmm |
| Cubic_High | 1 |  Cubic Cubic-High m3m |
| Hexagonal_Low | 2 |  Hexagonal-Low 6/m |
| Cubic_Low | 3 |  Cubic Cubic-Low m3 (Tetrahedral) |
| Triclinic | 4 |  Triclinic -1 |
| Monoclinic | 5 |  Monoclinic 2/m |
| OrthoRhombic | 6 |  Orthorhombic mmm |
| Tetragonal_Low | 7 |  Tetragonal-Low 4/m |
| Tetragonal_High | 8 |  Tetragonal-High 4/mmm |
| Trigonal_Low | 9 |  Trigonal-Low -3 |
| Trigonal_High | 10 |  Trigonal-High -3m |
| UnknownCrystalStructure | 999 |  Undefined Crystal Structure |

### Phase Type

| String Name | Internal Value |
| ------------|----------------|
| PrimaryPhase | 0 |
| PrecipitatePhase | 1 |
| TransformationPhase | 2 |
| MatrixPhase | 3 |
| BoundaryPhase | 4 |
| UnknownPhaseType | 999 |


## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Ensemble | Create Ensemble Info | The created Ensemble crystal structures, phase types, and phase names |

## Required Geometry ##

Not Applicable

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimension | Description |
|------|--------------|-------------|---------|--------------|
| **Attribute Matrix** | EnsembleAttributeMatrix | Ensemble | N/A | Created **Ensemble Attribute Matrix** name |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |
| **Ensemble Attribute Array** | PhaseTypes        | uint32_t | (1) | Enumeration representing the phase type for each **Ensemble** |
| **String Data Array**        | PhaseNames        | String | (1)   | The phase names for each **Ensemble** |


## Example Pipelines ##
Import_ASCII


## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: CreateEnsembleInfoFilter
+ Displayed Name: Create Ensemble Info

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_ensemble_attribute_matrix_name | Ensemble Attribute Matrix | The complete path to the attribute matrix in which to store the ensemble phase data arrays | complex.DataGroupCreationParameter |
| crystal_structures_array_name | Crystal Structures | The name of the data array representing the crystal structure for each Ensemble | complex.DataObjectNameParameter |
| ensemble | Created Ensemble Info | The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase. | complex.EnsembleInfoParameter |
| phase_names_array_name | Phase Names | The name of the string array representing the phase names for each Ensemble | complex.DataObjectNameParameter |
| phase_types_array_name | Phase Types | The name of the data array representing the phase types for each Ensemble | complex.DataObjectNameParameter |

