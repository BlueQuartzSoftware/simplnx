# Create Ensemble Info

## Group (Subgroup)

Processing (Generation)

## Description

This **Filter** allows the user to enter basic crystallographic information about each phase. The Laue class, Phase Type, and Phase Name can all be entered by the user. The information is stored in an EnsembleAttributeMatrix. These values are needed to allow the calculation of certain kinds of crystallographic statistics on the volume, if they have not already been provided by some other means. Each row in the table lists the **Crystal Structure**, **Phase Type**, and **Phase Name**. The proper values for the crystal structure and phase type come from internal constants within DREAM3D-NX and are listed here:

## DREAM3D-NX Laue Group to Point Group Table

| Internal Value | String Name              | HM Sym | Point Group | Rotation Point Group |
|----------------|--------------------------|--------|-------------|----------------------|
| 0              | Hexagonal_High           | 6/mmm  | 27          | 622                  |
| 1              | Cubic_High               | m-3m   | 32          | 432                  |
| 2              | Hexagonal_Low            | 6/m    | 23          | 6                    |
| 3              | Cubic_Low                | m-3    | 29          | 23                   |
| 4              | Triclinic                | -1     | 2           | 1                    |
| 5              | Monoclinic               | 2/m    | 5           | 2                    |
| 6              | OrthoRhombic             | mmm    | 8           | 222                  |
| 7              | Tetragonal_Low           | 4/m    | 11          | 4                    |
| 8              | Tetragonal_High          | 4/mmm  | 15          | 422                  |
| 9              | Trigonal_Low             | -3     | 17          | 3                    |
| 10             | Trigonal_High            | -3m    | 20          | 32                   |
| 999            | UnknownCrystalStructure  |        |             |                      |

### Phase Type

| String Name | Internal Value |
| ------------|----------------|
| PrimaryPhase | 0 |
| PrecipitatePhase | 1 |
| TransformationPhase | 2 |
| MatrixPhase | 3 |
| BoundaryPhase | 4 |
| UnknownPhaseType | 999 |

% Auto generated parameter table will be inserted here

## Example Pipelines

Import_ASCII

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
