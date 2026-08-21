# Read EDAX EBSD Data (.ang)

## Group (Subgroup)

IO (Input)

## Description

This **Filter** will read a single .ang file into a new **Image Geometry**, allowing the immediate use of **Filters** on the data instead of having to generate the intermediate .h5ebsd file. A **Cell Attribute Matrix** and **Ensemble Attribute Matrix** will also be created to hold the imported EBSD information. Currently, the user has no control over the names of the created **Attribute Arrays**. The user should be aware that simply reading the file then performing operations that are dependent on the proper crystallographic and sample reference frame will be undefined or simply **wrong**. In order to bring the crystal reference frame and sample reference frame into coincidence, rotations will need to be applied to the data.

### Created Data

The **Image Geometry** dimensions come from the `NCOLS_EVEN`/`NROWS` header values, the spacing from `XSTEP`/`YSTEP` (Z spacing is 1.0), the origin is (0, 0, 0), and the length units are always **micrometers**.

| Location | Array | Type | Notes |
|---|---|---|---|
| Cell | EulerAngles | float32 (3) | phi1, PHI, phi2 columns interleaved, in radians as stored in the file |
| Cell | Phases | int32 | Phase values less than 1 (un-indexed points) are remapped to 1 |
| Cell | Image Quality, Confidence Index, SEM Signal, Fit, X Position, Y Position | float32 | Copied verbatim from the file columns |
| Ensemble | CrystalStructures | uint32 | From each phase's TSL `Symmetry` code; index 0 is reserved for the invalid phase (999) |
| Ensemble | MaterialName | string | Whitespace-trimmed material name; index 0 is "Invalid Phase" |
| Ensemble | LatticeConstants | float32 (6) | a, b, c (Angstroms), alpha, beta, gamma (degrees); index 0 is all zeros |

### Note on Length Units

The created **Image Geometry** is always marked as **micrometers**, matching standard EDAX SEM-based `.ang` files. Legacy DREAM.3D 6.5 detected certain retired TEM/ACOM `.ang` variants and marked them as nanometers; EDAX retired those file formats over a decade ago and this filter does not special-case them. If you are importing such an archival file, set the geometry units manually after import.


![Fig. 1: An EBSD orientation is the rotation (Euler angles) between the sample reference frame (specimen axes) and the crystal reference frame (lattice axes). Import conventions may require realigning the sample frame with Rotate Sample Reference Frame and/or the crystal frame with Rotate Euler Reference Frame.](Images/EBSD_SampleVsCrystalReferenceFrame.png)

### Default TSL Transformations

If the data has come from a TSL acquisition system and the settings of the acquisition software were in the default modes, the following reference frame transformations may need to be performed based on the version of the OIM Analysis software being used to collect the data. These rotations can be applied with [Rotate Sample Reference Frame](../SimplnxCore/RotateSampleRefFrameFilter.md) and [Rotate Euler Reference Frame](RotateEulerRefFrameFilter.md):

+ Sample Reference Frame: 180<sup>o</sup> about the <010> Axis
+ Crystal Reference Frame: 90<sup>o</sup> about the <001> Axis

The user also may want to assign un-indexed pixels to be ignored by flagging them as "bad". The [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md) filter can be used to define this *mask* by thresholding on values such as *Confidence Index* > 0.1 or *Image Quality* > desired quality. **Confidence Index** and **Image Quality** are per-pixel metrics that describe how reliable each individual measurement is.

### Note About Sample Grid

OIMAnalysis can create EBSD data sampled on a hexagonal grid. The user can look in the .ang file into the header (those lines starting with the "#" character) for a line that is:

```text
# GRID: HexGrid
```

If the user's .ang files are hexagonal grid files then they will need to run the [Convert EDAX Hex Grid to Square Grid (.ang)](ConvertHexGridToSquareGridFilter.md) filter first to convert the input files to square-gridded files. This filter rejects hexagonal grid files during preflight with error `-19500`.

### Downstream Processing

Once the reference frames are correct, the imported Euler angles are typically converted to other orientation representations (quaternions, and so on) with [Convert Orientation Representation](ConvertOrientationsFilter.md) before computing misorientations, segmenting grains, or generating pole figures.

### Required Input Sources

None — this filter reads directly from a `.ang` file on disk.

### Note on .ang file Data Ordering

These are the order of the fields for each line in the data section of the .ang file.
```text
phi1
Phi
phi2
x pos
y pos
image quality
confidence index
phase
SEM Signal
Fit of Solution
```

% Auto generated parameter table will be inserted here

## Example Pipelines

+ Read_EDAX_Ang_File
+ Edax_IPF_Colors
+ CI_Histogram
+ EBSD_Hexagonal_Data_Analysis
+ ReplaceElementAttributesWithNeighbor (SimplnxCore)
+ ExtractVertexGeometry (SimplnxCore)
+ ApplyTransformation_Image (SimplnxCore)

## Differences from DREAM.3D 6.5

Behavioral differences from the legacy `ReadAngData` filter (material-name trimming, length-unit handling for retired TEM/ACOM files, error-code changes, and a legacy crash on non-contiguous phase indices) are documented in the source tree at `src/Plugins/OrientationAnalysis/vv/deviations/ReadAngDataFilter.md`.

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
