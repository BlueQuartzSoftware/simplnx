# Find Biased Features (Bounding Box)


## Group (Subgroup)

Generic (Spatial)

## Description

This **Filter** determines which **Features** are _biased_ by the outer surfaces of the sample. Larger **Features** are more likely to intersect the outer surfaces and thus it is not sufficient to only note which **Features** touch the outer surfaces of the sample. Denoting which **Features** are biased is important so that they may be excluded from any statistical analyses. The algorithm for determining whether a **Feature** is _biased_ is as follows: 

1. The _centroids_ of all **Features** are calculated
2. All **Features** are tested to determine if they touch an outer surface of the sample
3. The largest box is found that does not include any of the _centroids_ of **Features** that intersect an outer surface of the sample
4. Each **Feature**'s _centroid_ is checked to see whether it lies within the box.  

*If a **Feature**'s _centroid_ lies within the box, then the **Feature** is said to be _unbiased_, and if it lies outside the box, then the **Feature** is said to be _biased_.* 

By definition of the box, no **Feature** that intersects an outer surface of the sample can be considered _unbiased_, but it should be noted that **Features** that do not intersect the outer surfaces may still be considered _biased_. This algorithm works to determine the biased **Features** because all **Features** have one (and only one) centroid, no matter their size. Generally, this method will deem more small **Features** biased than the set of **Features** that just intersect the outer surfaces - and this corrects for the increased likelihood that larger **Features** will touch an outer surface.

*Note:* This **Filter** is a modification of an algorithm from Dave Rowenhorst (Naval Research Laboratory).

The images below show the feature ids before and after running this filter. The image on the right shows the biased features colored in red, the unbiased features colored by their feature id, the bounding box (described in step 3 of the algorithm above), and the feature centroids (white for unbiased and purple for biased).
![2D Before and After Biased Features](Images/FindBiasedFeaturesBeforeAndAfter.png)

## Parameters

| Name             | Type | Description |
|------------------|------|-------------|
| Apply Phase by Phase | bool | Whether to apply the biased **Features** algorithm per **Ensemble** | 

## Required Geometry

Image

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | Centroids | float | (3) | X, Y, Z coordinates of **Feature** center of mass |
| **Feature Attribute Array** | SurfaceFeatures | bool | (1) | Flag of 1 if **Feature** touches an outer surface or of 0 if it does not |
| **Feature Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs. Only required if _Apply Phase by Phase_ is checked |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | BiasedFeatures | bool | (1) | Flag of 1 if **Feature** is biased or of 0 if it is not |

## Example Pipelines

+ FindBiasedFeatures.d3dpipeline

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)





## Python Filter Arguments

+ module: complex
+ Class Name: FindBiasedFeaturesFilter
+ Displayed Name: Find Biased Features (Bounding Box)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| biased_features_array_name | Biased Features | Flag of 1 if Feature is biased or of 0 if it is not | complex.DataObjectNameParameter |
| calc_by_phase | Apply Phase by Phase | Whether to apply the biased Features algorithm per Ensemble | complex.BoolParameter |
| centroids_array_path | Centroids | X, Y, Z coordinates of Feature center of mass | complex.ArraySelectionParameter |
| image_geometry_path | Image Geometry | The selected geometry in which to the (biased) features belong | complex.GeometrySelectionParameter |
| phases_array_path | Phases | Specifies to which Ensemble each Feature belongs. Only required if Apply Phase by Phase is checked | complex.ArraySelectionParameter |
| surface_features_array_path | Surface Features | Flag of 1 if Feature touches an outer surface or of 0 if it does not | complex.ArraySelectionParameter |

