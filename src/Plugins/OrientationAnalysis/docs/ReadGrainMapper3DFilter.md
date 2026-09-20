# Read GrainMapper3D File

## Group (Subgroup)

Readers

## Description

This filter reads Version 4 and Version 5 **GrainMapper3D** HDF5 (`.h5`) files. GrainMapper3D is [XNovo Technology](https://xnovotech.com)'s lab diffraction-contrast-tomography (LabDCT) reconstruction software, used to reconstruct 3D crystallographic grain maps from laboratory X-ray measurements.

- Euler data is read in **radians**.
- The **Image Geometry** that is produced is in units of **millimeters**. This differs from the micron length-unit assumption made by many downstream DREAM3D-NX filters (for example, size and spacing computations). Be aware of this difference when chaining this reader into a pipeline.
- The user has the opportunity to create DREAM3D-NX compatible Orientation Data and Phase data. See below.

## Parameter Discussion

GrainMapper3D orientation convention is the same as used by [MTEX](https://mtex-toolbox.github.io), and the inverse of that adapted by DREAM3D-NX.
This requires certain modifications to the orientation related data (Rodrigues and Quaternions) when being read from the
file. These modifications ensure that when DREAM3D computes orientation related data, the correct results
will be output.

Specifically, the Rodrigues vector will be converted into a 4 component and the conjugate computed. The quaternion
order will be changed from wxyz to xyzw and the conjugate will be computed.

PhaseId data will be converted to *int32* (when the *Create Compatible Phase Data* option is checked) to make that data immediately compatible with DREAM3D-NX's filters.

IPF (inverse pole figure) colors can be stored in the file as either *uint8* or *float32* values. To immediately view the IPF colors that came from the file, the user should check the box for *Create Compatible IPFColor Data*, which converts any *float32* color data to *uint8*.

## Algorithm

The reader transfers each LabDCT and AbsorptionCT HDF5 dataset in C-order
hyperslabs.  The hyperslab iterator chooses a dimension whose trailing extent
fits a fixed 65,536-value buffer, so scalar and vector image volumes never
require a complete slice or volume in memory.  Every transfer is written to the
destination DataArray with one bulk operation.

When compatible Phase, Rodrigues, IPF color, or quaternion data is requested,
the conversion is applied to each bounded source chunk before its bulk write.
This preserves the file's tuple and component ordering while allowing the cell
arrays to remain out-of-core.  Phase metadata remains ensemble-sized and is
read normally.

## Special Notes

The IPF colors (if any) that are read in from the file are **NOT** compatible with the IPF color legends provided by DREAM3D-NX or EBSDLib. There are two distinct options at play, and they should not be confused:

- *Create Compatible IPFColor Data* only re-types the colors that were *already computed by GrainMapper3D* so they can be displayed; it does not recompute them against DREAM3D-NX's legends.
- To obtain IPF colors that match the DREAM3D-NX / EBSDLib legends, run [Compute IPF Colors](ComputeIPFColorsFilter.md) downstream on the imported orientation data. Alternatively, the user can obtain the matching IPF legends from XNovo.

Use [Compute IPF Colors](ComputeIPFColorsFilter.md) if you need to specifically understand the crystallographic orientations using DREAM3D-NX's color conventions.

## Required Input Sources

None. This filter reads directly from a `.h5` GrainMapper3D file and creates the **Image Geometry**, **Cell** data, and **Ensemble** data itself.

% Auto generated parameter table will be inserted here

## References

[https://xnovotech.com/3d-crystallographic-imaging-software/](https://xnovotech.com/3d-crystallographic-imaging-software/)

## Example Pipelines


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
