# Create Ensemble Info

## Group (Subgroup)

Processing (Generation)

## Description

This **Filter** allows the user to enter basic crystallographic information about 
each phase. The Laue class, Phase Type, and Phase Name can all be entered by the 
user. The information is stored in an EnsembleAttributeMatrix. These values are 
needed to allow the calculation of certain kinds of crystallographic statistics 
on the volume, if they have not already been provided by some other means. Each 
row in the table lists the **Crystal Structure**, **Phase Type**, and **Phase Name**. 
The proper values for the crystal structure and phase type come from internal
constants within DREAM3D-NX and are listed here:

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

## DREAM3D-NX Full Crystallographic Table

In the table below, the column labeled `Laue Ops` is the internal DREAM3D-NX class name
and is the name that the user will find the combo-box selection when creating a crystallographic
phase.

| Point Group | (H–M)       | Rotation Point Group | Space Group No(s). | Schoenflies   | Crystal system | Laue class  | Laue Ops        | Internal Value |
|-------------|-------------|----------------------|--------------------|---------------|----------------|-------------|-----------------|----------------|
| 1           | 1           | 1                    | 1                  | C₁            | Triclinic      | (\bar{1})   | Triclinic       | 0              |
| 2           | (\bar{1})   | 1                    | 2                  | C(_i)         | Triclinic      | (\bar{1})   |                 |                |
| 3           | 2           | 2                    | 3–5                | C₂            | Monoclinic     | 2/m         |                 |                |
| 4           | m           | 1                    | 6–9                | C(_s)         | Monoclinic     | 2/m         |                 |                |
| 5           | 2/m         | 2                    | 10–15              | C(_{2h})      | Monoclinic     | 2/m         | Monoclinic      | 5              |
| 6           | 222         | 222                  | 16–24              | D₂            | Orthorhombic   | mmm         |                 |                |
| 7           | mm2         | 2                    | 25–46              | C(_{2v})      | Orthorhombic   | mmm         |                 |                |
| 8           | mmm         | 222                  | 47–74              | D(_{2h})      | Orthorhombic   | mmm         | OrthoRhombic    | 6              |
| 9           | 4           | 4                    | 75–80              | C₄            | Tetragonal     | 4/m         |                 |                |
| 10          | (\bar{4})   | 2                    | 81–82              | S₄            | Tetragonal     | 4/m         |                 |                |
| 11          | 4/m         | 4                    | 83–88              | C(_{4h})      | Tetragonal     | 4/m         | Tetragonal_Low  | 7              |
| 12          | 422         | 422                  | 89–98              | D₄            | Tetragonal     | 4/mmm       |                 |                |
| 13          | 4mm         | 4                    | 99–110             | C(_{4v})      | Tetragonal     | 4/mmm       |                 |                |
| 14          | (\bar{4}2m) | 222                  | 111–122            | D(_{2d})      | Tetragonal     | 4/mmm       |                 |                |
| 15          | 4/mmm       | 422                  | 123–142            | D(_{4h})      | Tetragonal     | 4/mmm       | Tetragonal_High | 8              |
| 16          | 3           | 3                    | 143–146            | C₃            | Trigonal       | (\bar{3})   |                 |                |
| 17          | (\bar{3})   | 3                    | 147–148            | C(_{3i}) (S₆) | Trigonal       | (\bar{3})   | Trigonal_Low    | 9              |
| 18          | 32          | 32                   | 149–155            | D₃            | Trigonal       | (\bar{3}m)  |                 |                |
| 19          | 3m          | 3                    | 156–161            | C(_{3v})      | Trigonal       | (\bar{3}m)  |                 |                |
| 20          | (\bar{3}m)  | 32                   | 162–167            | D(_{3d})      | Trigonal       | (\bar{3}m)  | Trigonal_High   | 10             |
| 21          | 6           | 6                    | 168–173            | C₆            | Hexagonal      | 6/m         |                 |                |
| 22          | (\bar{6})   | 3                    | 174                | C(_{3h})      | Hexagonal      | 6/m         |                 |                |
| 23          | 6/m         | 6                    | 175–176            | C(_{6h})      | Hexagonal      | 6/m         | Hexagonal_Low   | 2              |
| 24          | 622         | 622                  | 177–182            | D₆            | Hexagonal      | 6/mmm       |                 |                |
| 25          | 6mm         | 6                    | 183–186            | C(_{6v})      | Hexagonal      | 6/mmm       |                 |                |
| 26          | (\bar{6}m2) | 32                   | 187–190            | D(_{3h})      | Hexagonal      | 6/mmm       |                 |                |
| 27          | 6/mmm       | 622                  | 191–194            | D(_{6h})      | Hexagonal      | 6/mmm       | Hexagonal_High  | 0              |
| 28          | 23          | 23                   | 195–199            | T             | Cubic          | m(\bar{3})  |                 |                |
| 29          | m(\bar{3})  | 23                   | 200–206            | T(_h)         | Cubic          | m(\bar{3})  | Cubic_Low       | 3              |
| 30          | 432         | 432                  | 207–214            | O             | Cubic          | m(\bar{3})m |                 |                |
| 31          | (\bar{4}3m) | 23                   | 215–220            | T(_d)         | Cubic          | m(\bar{3})m |                 |                |
| 32          | m(\bar{3})m | 432                  | 221–230            | O(_h)         | Cubic          | m(\bar{3})m | Cubic_High      | 1              |

### Phase Type

| String Name         | Internal Value |
|---------------------|----------------|
| PrimaryPhase        | 0              |
| PrecipitatePhase    | 1              |
| TransformationPhase | 2              |
| MatrixPhase         | 3              |
| BoundaryPhase       | 4              |
| UnknownPhaseType    | 999            |

% Auto generated parameter table will be inserted here

## Example Pipelines

Import_ASCII

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
