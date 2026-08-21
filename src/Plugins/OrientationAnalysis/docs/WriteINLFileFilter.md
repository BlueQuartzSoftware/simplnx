# Write INL File

## Group (Subgroup)

IO (Output)

## Description

This **Filter** writes out **Cell** data from an **Image Geometry** to a file format used by the Idaho National Laboratory (INL).  The format is columnar and space delimited, with header lines denoted by the "#" character. The columns are the following:

- phi1 -- first Bunge Euler angle, in **radians**
- Phi -- second Bunge Euler angle, in **radians**
- phi2 -- third Bunge Euler angle, in **radians**
- x Position -- in **microns**
- y Position -- in **microns**
- z Position -- in **microns**
- Feature Id -- the grain (feature) each cell belongs to
- Phase Id -- the ensemble (phase) each cell belongs to
- Symmetry -- the crystal-symmetry code for the cell's phase

The **Symmetry** column holds the integer crystal-symmetry code corresponding to the cell's crystal structure (for example, *43* denotes cubic m-3m and *62* denotes hexagonal 6/mmm). The codes follow the EBSDLib `CrystalStructure` enumeration and are taken from the **Crystal Structures** ensemble array.

Some information about the phase is included in the header section of the file in addition to values for the origin, step size, and dimensions. The origin, **step sizes** (`X_STEP`, `Y_STEP`, `Z_STEP`), and min/max positions in the header are all in **microns**. The header also reports the number of **Features** in the file.

## Example Output

    # File written from DREAM3DLib Version 5.2.1789.6419a8d
    # DateTime: Fri Jun 19 10:13:25 2015
    # X_STEP: 0.250000
    # Y_STEP: 0.250000
    # Z_STEP: 0.250000
    #

    # X_MIN: -36.750004
    # Y_MIN: 10.250000
    # Z_MIN: -29.000000
    #

    # X_MAX: -11.750004
    # Y_MAX: 35.250000
    # Z_MAX: -4.000000
    #

    # X_DIM: 100
    # Y_DIM: 100
    # Z_DIM: 100
    #

    # Phase_1: Nickel 
    # Symmetry_1: 43
    # Features_1: 796
    #

    # Num_Features: 796 
    #

    # phi1 PHI phi2 x y z FeatureId PhaseId Symmetry
    0.266984 0.696910 4.434039 -36.750004 10.250000 -29.000000 639 1 43
    0.266984 0.696910 4.434039 -36.500004 10.250000 -29.000000 639 1 43
    0.266984 0.696910 4.434039 -36.250004 10.250000 -29.000000 639 1 43
    0.266984 0.696910 4.434039 -36.000004 10.250000 -29.000000 639 1 43
    0.267274 0.697210 4.432979 -35.750004 10.250000 -29.000000 639 1 43
    0.266984 0.696910 4.434039 -35.500004 10.250000 -29.000000 639 1 43
    0.266984 0.696910 4.434039 -35.250004 10.250000 -29.000000 639 1 43
    0.266984 0.696910 4.434039 -35.000004 10.250000 -29.000000 639 1 43
    0.266234 0.697020 4.434729 -34.750004 10.250000 -29.000000 639 1 43
    0.266234 0.697020 4.434729 -34.500004 10.250000 -29.000000 639 1 43

## Required Input Sources

- **Cell Euler Angles** -- typically read from EBSD data via [Read H5EBSD File](ReadH5EbsdFilter.md), [Read EDAX EBSD Data (.ang)](ReadAngDataFilter.md), or [Read Oxford Instr. EBSD Data (.ctf)](ReadCtfDataFilter.md). Must be in radians (Bunge Z-X-Z convention).
- **Cell Phases** -- read alongside the Euler angles from the same EBSD reader.
- **Crystal Structures** -- the **Ensemble**-level array read alongside the phases from the same EBSD reader.
- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](../SimplnxCore/ScalarSegmentFeaturesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

- INL Export

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
