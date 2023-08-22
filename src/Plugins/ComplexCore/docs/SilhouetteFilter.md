# Silhouette #

## Group (Subgroup) ##

DREAM3D Review (Clustering)

## Description ##

This **Filter** computes the silhouette for a clustered **Attribute Array**.  The user must select both the original array that has been clustered and the array of cluster Ids.  The silhouette represents a measure for the quality of a clustering.  Specifically, the silhouette provides a measure for how strongly a given point belongs to its own cluster compared to all other clusters.  The silhouette is computed as follows:

\f[ s_{i} = \frac{b_{i} - a_{i}}{\max\{a_{i},b_{i}\}} \f]

where \f$ a \f$ is the average distance between point \f$ i \f$ and all other points in the cluster point \f$ i \f$ belongs to, \f$ b \f$ is the _next closest_ average distance among all other clusters, and \f$ s \f$ is the silhouette value.  Using this definition, \f$ s \f$ exists on the interval \f$ [-1, 1] \f$, where 1 indicates that the point strongly belongs to its current cluster and -1 indicates that the point does not belong well to its current cluster.  The user may select from a variety of options to use as the distance metric.  Additionally, the user may opt to use a mask array to ignore points in the silhouette; these points will contain a silhouette value of 0.

The silhouette can be used to determine how well a particular clustering has performed, such as [k means](@ref kmeans) or [k medoids](@ref kmedoids).

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Distance Metric | Enumeration | The metric used to determine the distances between points |
| Use Mask | bool | Whether to use a boolean mask array to ignore certain points flagged as _false_ from the algorithm |

## Required Geometry ###

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any| Any | The **Attribute Array** to silhouette |
| **Attribute Array** | ClusterIds | int32_t | (1) | Specifies to which cluster each point belongs |
| **Attribute Array** | Mask | bool | (1) | Specifies if the point is to be counted in the algorithm, if _Use Mask_ is checked |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Array** | Silhouette | double | (1) | Silhouette value for each point  |

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
<https://groups.google.com/forum/?hl=en#!forum/dream3d-users>


## Python Filter Arguments

+ module: complex
+ Class Name: SilhouetteFilter
+ Displayed Name: Silhouette

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| distance_metric | Distance Metric | Distance Metric type to be used for calculations | complex.ChoicesParameter |
| feature_ids_array_path | Cluster Ids | The DataPath to the DataArray that specifies which cluster each point belongs | complex.ArraySelectionParameter |
| mask_array_path | Mask | DataPath to the boolean mask array. Values that are true will mark that cell/point as usable. | complex.ArraySelectionParameter |
| selected_array_path | Attribute Array to Silhouette | The DataPath to the input DataArray | complex.ArraySelectionParameter |
| silhouette_array_path | Silhouette | The DataPath to the calculated output Silhouette array values | complex.ArrayCreationParameter |
| use_mask | Use Mask | Specifies whether or not to use a mask array | complex.BoolParameter |

