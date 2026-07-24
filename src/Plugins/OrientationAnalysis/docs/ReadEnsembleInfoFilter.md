# Read Ensemble Info File

## Group (Subgroup)

IO (Input)

## Description

An **Ensemble** in DREAM3D-NX is a *phase*: a distinct material or crystal type present in the sample. This filter reads a small ASCII text file that describes each phase and creates the per-phase metadata that many statistics filters require. Specifically, for every phase it records a **Crystal Structure** (the crystal symmetry, which determines how orientations and misorientations are computed) and a **Phase Type** (the role the phase plays, such as a primary matrix phase or a precipitate phase).

The filter does not create a new geometry. The *Data Container* parameter selects an existing geometry or data group, and the filter adds a new **Ensemble Attribute Matrix** (per-phase data) containing the **Crystal Structures** and **Phase Types** arrays into it. Downstream statistics and synthetic-structure filters read these arrays to know how to treat each phase. This information is needed to compute statistics on the volume when it has not already been provided by some other means (for example, by an EBSD reader).

The input file is a simple ASCII text file with either a `.ini` or `.txt` extension; both use the same format. The first group is named `[EnsembleInfo]` in square brackets with the key `Number_Phases=`*number of phases* contained in the volume. Subsequent groups list the **Phase Number**, **Crystal Structure** and **Phase Type**. The valid string values for the crystal structure and phase type come from internal constants within DREAM3D-NX and are listed below. These tables describe the *content of the input file*, not the filter's parameters.

A **Crystal Structure** value names the Laue (point-group) symmetry of the phase. A **Phase Type** value names the role the phase plays in subsequent analysis.

**Crystal Structure**

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

**Phase Type**

| String Name | Internal Value |
| ------------|----------------|
| PrimaryPhase | 0 |
| PrecipitatePhase | 1 |
| TransformationPhase | 2 |
| MatrixPhase | 3 |
| BoundaryPhase | 4 |
| UnknownPhaseType | 999 |

## Example Input

**Phase numbering starts at One (1). Phase Zero (0) is reserved for internal use in DREAM3D-NX**
For example, if you have a structure that has 2 phases that consist of a Cubic Primary phase and a Hexagonal Matrix phase the file would be the following:

    [EnsembleInfo]
    Number_Phases=2

    [1]
    CrystalStructure=Cubic_High
    PhaseType=PrimaryPhase

    [2]
    CrystalStructure=Hexagonal_High
    PhaseType=MatrixPhase

### Required Input Sources

None — this filter reads directly from a `.ini` or `.txt` file on disk. It does require an existing **Data Container** (a geometry or data group) into which the created **Ensemble Attribute Matrix** is placed.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
