# Align Sections (Misorientation)

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** attempts to align consecutive 'sections' perpendicular to the Z-direction of the sample by determining the position that results in the minimum amount of misorientation between **Cells** directly above-below each other. The algorithm of this **Filter** is as follows:

1. Calculate the misorientation between each **Cell** in a section and the **Cell** directly above it in the next section  
2. Count the number of **Cell** pairs that have a misorientation above the user defined tolerance and store that as the misalignment value for that position
3. Repeat steps 1 and 2 for each position when shifting the second slice (relative to the first) from three (3) **Cells** to the left to three (3) **Cells** to the right, as well as from three (3) **Cells** up to three (3) **Cells** down. *Note that this creates a 7x7 grid*
4. Determine the position in the 7x7 grid that has the lowest misalignment value. (It will be the position with the fewest different **Cell** pairs)
5. Repeat steps 1-4 with the center of each (new) 7x7 grid at the best position from the last 7x7 grid until the best position in the current/new 7x7 grid is the same as the last 7x7 grid
6. Repeat steps 1-5 for each pair of neighboring sections

**Note that this is similar to a downhill simplex and can get caught in a local minimum!**

If the user elects to use a mask array, the **Cells** flagged as *false* in the mask array will not be considered during the alignment process.  

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
