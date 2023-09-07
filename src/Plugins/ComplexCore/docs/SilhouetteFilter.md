# Silhouette #

## Group (Subgroup) ##

DREAM3D Review (Clustering)

## Description ##

This **Filter** computes the silhouette for a clustered **Attribute Array**.  The user must select both the original array that has been clustered and the array of cluster Ids.  The silhouette represents a measure for the quality of a clustering.  Specifically, the silhouette provides a measure for how strongly a given point belongs to its own cluster compared to all other clusters.  The silhouette is computed as follows:

\f[ s_{i} = \frac{b_{i} - a_{i}}{\max\{a_{i},b_{i}\}} \f]

where \f$ a \f$ is the average distance between point \f$ i \f$ and all other points in the cluster point \f$ i \f$ belongs to, \f$ b \f$ is the *next closest* average distance among all other clusters, and \f$ s \f$ is the silhouette value.  Using this definition, \f$ s \f$ exists on the interval \f$ [-1, 1] \f$, where 1 indicates that the point strongly belongs to its current cluster and -1 indicates that the point does not belong well to its current cluster.  The user may select from a variety of options to use as the distance metric.  Additionally, the user may opt to use a mask array to ignore points in the silhouette; these points will contain a silhouette value of 0.

The silhouette can be used to determine how well a particular clustering has performed, such as k means or k medoids.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Distance Metric | Enumeration | The metric used to determine the distances between points |
| Use Mask | bool | Whether to use a boolean mask array to ignore certain points flagged as *false* from the algorithm |

## Required Geometry ###

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any| Any | The **Attribute Array** to silhouette |
| **Attribute Array** | ClusterIds | int32_t | (1) | Specifies to which cluster each point belongs |
| **Attribute Array** | Mask | bool | (1) | Specifies if the point is to be counted in the algorithm, if *Use Mask* is checked |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Array** | Silhouette | double | (1) | Silhouette value for each point  |

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


