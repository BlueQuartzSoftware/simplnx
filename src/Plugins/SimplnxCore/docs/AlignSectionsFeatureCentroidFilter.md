# Align Sections (Feature Centroid)

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** attempts to align 'sections' of the sample perpendicular to the Z-direction by determining the position that closest aligns the centroid(s) of previously defined "regions".  The "regions" are defined by a boolean array where the **Cells** have been flagged by another **Filter**. Typically, during reading/processing of the data, each **Cell** is subject to a "quality metric" (or threshold) that defines if the **Cell** is *good*.  This threshold can be used to define areas of each slice that are bad, either due to actual features in the microstructure or external references inserted by the user/experimentalist.  If these "regions" of *bad* **Cells** are believed to be consistent through sections, then this **Filter** will preserve that by aligning those "regions" on top of one another on consecutive sections. The algorithm of this **Filter** is as follows:

1. Determine the centroid of all **Cells** that are flagged with a boolean value equal to *true* for each section
2. Determine the shifts that place centroids of consecutive sections on top of one another
3. Round the shifts determined in step 2 to the nearest multiple of the **Cell** resolution. (This forces the sections to be shifted by full **Cell** increments)

If the user elects to *Use Reference Slice*, then each section's centroid is shifted to lie on top of the reference slice's centroid position and not its neighboring section's centroid position.

**Note that this is algorithm cannot get caught in a local minimum!**

The user can choose to write the determined shift to an output file by enabling *Write Alignment Shifts File* and providing a file path.  

The user can also decide to remove a *background shift* present in the sample. The process for this is to fit a line to the X and Y shifts along the Z-direction of the sample.  The individual shifts are then modified to make the slope of the fit line be 0.  Effectively, this process is trying to keep the top and bottom section of the sample fixed.  Some combinations of sample geometry and internal features can result in this algorithm introducing a 'shear' in the sample and the *Linear Background Subtraction* will attempt to correct for this.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (10) SmallIN100 Full Reconstruction
+ (03) SmallIN100 Alignment

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
