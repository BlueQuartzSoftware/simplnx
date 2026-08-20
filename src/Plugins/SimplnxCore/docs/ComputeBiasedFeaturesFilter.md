# Compute Biased Features (Bounding Box)

## Group (Subgroup)

Generic (Spatial)

## Description

This **Filter** determines which **Features** are *biased* by the outer surfaces of the sample. Larger **Features** are more likely to intersect the outer surfaces and thus it is not sufficient to only note which **Features** touch the outer surfaces of the sample. Denoting which **Features** are biased is important so that they may be excluded from any statistical analyses.

The **Feature** *centroids* and the surface-**Feature** flags are computed by upstream **Filters** (*Compute Feature Centroids* and *Compute Surface Features*); this **Filter** consumes them and shrinks a box away from the surface **Features**:

1. The box is initialized to the full bounding box of the selected **Image Geometry**, that is `[origin, origin + dimensions * spacing]` on each axis.
2. Each surface **Feature** is visited in ascending **Feature** index order. If its *centroid* lies strictly inside all six faces, the single **nearest** face is pulled inward onto that *centroid*; every other face is left alone. If the *centroid* lies on or outside any face, that **Feature** shrinks nothing.
3. Each **Feature**'s *centroid* is then compared against the final box. The comparison is **inclusive**: a *centroid* lying exactly on a face is *biased*.

*If a **Feature**'s *centroid* lies strictly inside the box, then the **Feature** is said to be *unbiased*; if it lies on or outside the box, then the **Feature** is said to be *biased*.*

No **Feature** that intersects an outer surface of the sample can be considered *unbiased*, but **Features** that do not intersect the outer surfaces may still be considered *biased*. This algorithm works to determine the biased **Features** because all **Features** have one (and only one) centroid, no matter their size. Generally, this method will deem more small **Features** biased than the set of **Features** that just intersect the outer surfaces - and this corrects for the increased likelihood that larger **Features** will touch an outer surface.

*Note:* This **Filter** is a modification of an algorithm from Dave Rowenhorst (Naval Research Laboratory).

### Behaviors To Be Aware Of

+ **The shrink is greedy and order dependent.** Because each surface **Feature** pulls only its own nearest face, and because pulling a face changes which face is nearest for the **Features** processed after it, the final box depends on the order the **Features** are stored in. It is therefore *not* guaranteed to be the largest box that excludes every surface-**Feature** *centroid*. Two identical **Feature** sets differing only in index order can produce different *Biased Features* output.
+ **Feature 0 is always skipped** and is always reported as unbiased, because index 0 is the "unassigned" bucket rather than a real **Feature**.
+ **2D geometries.** If any dimension of the **Image Geometry** is 1, the flat axis is dropped and the box becomes a rectangle over the two remaining in-plane axes, using the origin, dimension and spacing of those two axes and the corresponding two *centroid* components. The flat axis takes no part in the classification.
+ **Apply Phase by Phase has no effect on 2D geometries.** The 2D code path does not read the *Phases* array at all; the box is computed once over every surface **Feature** regardless of phase.
+ **Apply Phase by Phase on 3D geometries** recomputes the box independently for each phase from 1 up to the largest value in the *Phases* array, and within each pass only classifies **Features** belonging to that phase. **Features** whose phase is 0 or negative are never classified and are always reported as unbiased.

The images below show the feature ids before and after running this filter. The image on the right shows the biased features colored in red, the unbiased features colored by their feature id, the bounding box (described in step 3 of the algorithm above), and the feature centroids (white for unbiased and purple for biased).
![2D Before and After Biased Features](Images/ComputeBiasedFeaturesBeforeAndAfter.png)

% Auto generated parameter table will be inserted here

## Example Pipelines

+ ComputeBiasedFeatures.d3dpipeline

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
