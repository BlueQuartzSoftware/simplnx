# Read Oxford Aztec Data (.h5oina)

## Group (Subgroup)

IO (Input)

## Description

This filter reads data from a single `.h5oina` file (the HDF5-based export from Oxford Instruments' AZtec software) into a new **Image Geometry**. Reading the file directly lets the data be used immediately by other filters, instead of having to first build an intermediate `.h5ebsd` file. A **Cell Attribute Matrix** (per-pixel data) and an **Ensemble Attribute Matrix** (per-phase data) are created to hold the imported EBSD information. The user currently has no control over the names of the created **Attribute Arrays**.

### What This Filter Produces

The file is EBSD (Electron Backscatter Diffraction) scan data. The most important imported arrays are:

- **Orientation** — stored as three **Euler angles** per pixel (the three angles, in Bunge Z-X-Z convention, that describe how each measured crystal is rotated relative to the sample). Orientation is the basis for almost all downstream crystallographic analysis.
- **Phase** — a per-pixel index identifying which material (phase) was measured at that point.
- **Pattern-quality metrics** — values such as Band Contrast, Band Slope, Bands, and Mean Angular Deviation describe how clear and reliable each measurement is. These are commonly used to flag unreliable pixels (see *Reference Frames* below).
- **Per-phase (Ensemble) data** — the crystal structure, lattice constants, and material name for each phase. An **Ensemble** here means one distinct material/crystal type.

### Limitations of the Filter

The current implementation only understands **FORMAT VERSION 2.0** of the H5OINA file. A user can still read a newer H5OINA file, but the filter will only extract the VERSION 2.0 headers and data. If additional data is needed from the file, the [Read HDF5 Dataset](../SimplnxCore/ReadHDF5DatasetFilter.md) filter can be used to augment this filter.

![Overview of the user interface.](Images/ImportH5OinaFilter_1.png)

## Notes About Reference Frames

In order to bring the crystal reference frame and the sample reference frame into coincidence, rotations **MAY** need to be applied to the data. Two filters can perform the necessary rotations:

- [Rotate Euler Reference Frame](RotateEulerRefFrameFilter.md)
- [Rotate Sample Reference Frame](../SimplnxCore/RotateSampleRefFrameFilter.md)

Historical reference frame operations for Oxford data are the following:

+ Sample Reference Frame: 180<sup>o</sup> about the <010> Axis
+ Crystal Reference Frame: None

The user also may want to assign un-indexed pixels to be ignored by flagging them as "bad". The [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md) filter can be used to define this *mask* by thresholding on values such as *Error* = 0.

### Radians and Degrees

All orientation data in the H5OINA file are in radians.

### The Axis Alignment Issue for Hexagonal Symmetry [1]

+ The issue with hexagonal materials is the alignment of the Cartesian coordinate system used for calculations with the crystal coordinate system (the Bravais lattice).
+ In one convention (e.g. EDAX.TSL), the x-axis, i.e. [1,0,0], is aligned with the crystal a1 axis, i.e. the [2,-1,-1,0] direction. In this case, the y-axis is aligned with the [0,1,-1,0] direction. (Green Axis in Figure 1)
+ In the other convention, (e.g. Oxford Instr, Univ. Metz software), the x-axis, i.e. [1,0,0], is aligned with the crystal [1,0,-1,0] direction. In this case, the y-axis is aligned with the [-1,2,-1,0] direction. (Red Axis in Figure 1)
+ This is important because texture analysis can lead to an ambiguity as to the alignment of [2,-1,-1,0] versus [1,0,-1,0], with apparent **30 Degree** shifts in the data.
+ Caution: it appears that the axis alignment is a choice that must be made when installing TSL software so determination of which convention is in use must be made on a case-by-case basis. It is fixed to the y-convention in the HKL software.
+ The main clue that something is wrong in a conversion is that either the 2110 & 1010 pole figures are transposed, or that a peak in the inverse pole figure that should be present at 2110 has shifted over to 1010.
+ DREAM3D-NX uses the TSL/EDAX convention.
+ __The result of this is that the filter will by default add 30 degrees to the second Euler Angle (phi2) when reading Oxford `.h5oina` files. This can be disabled by the user if necessary.__

| Figure 1 |
|--------|
| ![Figure showing 30 Degree conversions](Images/Hexagonal_Axis_Alignment.png) |
| **Figure 1:** showing TSL and Oxford Instr. conventions. EDAX/TSL is in **Green**. Oxford Inst. is in **Red** |

### Downstream Processing

Once the reference frames are correct, the imported Euler angles are typically converted to other orientation representations (quaternions, etc.) with [Convert Orientation Representation](ConvertOrientationsFilter.md) before computing misorientations, segmenting grains, or generating pole figures.

## Required Input Sources

None — this filter reads directly from a `.h5oina` file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## References

[1] Rollett, A.D. Lecture Slides located at [http://pajarito.materials.cmu.edu/rollett/27750/L17-EBSD-analysis-31Mar16.pdf](http://pajarito.materials.cmu.edu/rollett/27750/L17-EBSD-analysis-31Mar16.pdf)

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
