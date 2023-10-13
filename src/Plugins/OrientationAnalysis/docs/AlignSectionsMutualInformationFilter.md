# Align Sections (Mutual Information)

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** segments each 2D slice, creating *Feature Ids* that are used when determining the *mutual information* between neighboring slices. The slices are shifted relative to one another until the position of maximum *mutual information*  is determined for each section.  The *Feature Ids* are temporary, they apply to this **Filter** only and are not related to the *Feature Ids* generated in other **Filters**.  The algorithm of this **Filter** is listed below:

1. Segment *Features* on each 'section' of the sample perpendicular to the Z-direction.  This is done using the same algorithm in the [Segment Features (Misorientation)](@ref alignsectionsmisorientation) **Filter** (only in 2D on each section)  
2. Calculate the *mutual information* between neighboring sections and store that as the misalignment value for that position. *Mutual information* is related to the ratio of joint probability to individual probabilities of variables (i.e., p(x,y)/p(x)p(y) ). Details of the actual *mutual information* calculation can be found in the references below, but can be thought of as the inherent dependence between variables (here the *Feature Ids* on neighboring sections).  
3. Repeat step 2 for each position when shifting the second slice (relative to the first) from three (3) **Cells** to the left
to three (3) **Cells** to the right, as well as from three (3) **Cells** up to three (3) **Cells** down
*Note that this creates a 7x7 grid*
4. Determine the position in the 7x7 grid that has the highest *mutual information* value
5. Repeat steps 2-4 with the center of each (new) 7x7 grid at the best position from the last 7x7 grid until the best position in the current/new 7x7 grid is the same as the last 7x7 grid

6) Repeat steps 2-5 for each pair of neighboring sections

**Note that this is similar to a downhill simplex and can get caught in a local minimum!**

The user choses the level of *misorientation tolerance* by which to align **Cells**, where here the tolerance means the *misorientation* cannot exceed a given value. If the rotation angle is below the tolerance, then the **Cell** is grouped with other **Cells** that satisfy the criterion.

The approach used in this **Filter** is to group neighboring **Cells** on a slice that have a *misorientation* below the tolerance the user entered. *Misorientation* here means the minimum rotation angle of one **Cell's** crystal axis needed to coincide with another **Cell's** crystal axis. When the **Features** in the slices are defined, they are moved until *disks* in neighboring slices align with each other.

If the user elects to use a mask array, the **Cells** flagged as *false* in the mask array will not be considered during the alignment process.  

The user can choose to write the determined shift to an output file by enabling *Write Alignment Shifts File* and providing a file path.  

% Auto generated parameter table will be inserted here

## References

[Scholar Pedia](http://www.scholarpedia.org/article/Mutual_information)

Journal articles on *Mutual Information* that are useful:

+ *Elements of information theory*. John Wiley & Sons, New York, NY.Gray, R.M. (1990).
+ *Entropy and Information Theory*. Springer-Verlag, New York, NY. Nirenberg, S. and Latham, P.E. (2003).
+ *Decoding neuronal spike trains: how important are correlations?* Proc. Natl. Acad. Sci. 100:7348-7353. Shannon, C.E. and Weaver, W. (1949).
+ *The mathematical theory of communication*. University of Illinois Press, Urbana, Illinois. M  zard, M. and Monatanari, A. (2009).
+ *Information, Physics, and Computation*. Oxford University Press, Oxford.

## Example Pipelines

AlignSectionsMutualInformation

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
