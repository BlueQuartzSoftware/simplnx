# K Means #

## Group (Subgroup) ##

DREAM3D Review (Clustering)

## Description ##

This **Filter** applies the k means algorithm to an **Attribute Array**.  K means is a _clustering algorithm_ that assigns to each point of the **Attribute Array** a _cluster Id_.  The user must specify the number of clusters in which to partition the array.  Specifically, a k means partitioning is a _Voronoi tesselation_; an optimal solution to the k means problem is such that each point in the data set is associated with the cluster that has the closest mean.  This partitioning is the one that minimizes the within cluster variance (i.e., minimizes the within cluster sum of squares differences).  Thus, the "metric" used for k means is the 2-norm (the _Euclidean norm_; the squared Euclidean norm may also be used since this maintains the triangle inequality).

Optimal solutions to the k means partitioning problem are computationally difficult; this **Filter** used _Lloyd's algorithm_ to approximate the solution.  Lloyd's algorithm is an iterative algorithm that proceeds as follows:

1. Choose k points at random to serve as the initial cluster "means"
2. Until convergence, repeat the following steps:

* Associate each point with the closest mean, where "closest" is the smallest 2-norm distance
* Recompute the means based on the new tesselation

Convergence is defined as when the computed means change very little (precisely, when the differences are within machine epsilon).  Since Lloyd's algorithm is iterative, it only serves as an approximation, and may result in different classifications on each execution with the same input data.  The user may opt to use a mask to ignore certain points; where the mask is _false_, the points will be placed in cluster 0.

A clustering algorithm can be considered a kind of segmentation; this implementation of k means does not rely on the **Geometry** on which the data lie, only the _topology_ of the space that the array itself forms.  Therefore, this **Filter** has the effect of creating either **Features** or **Ensembles** depending on the kind of array passed to it for clustering.  If an **Element** array (e.g., voxel-level **Cell** data) is passed to the **Filter**, then **Features** are created (in the previous example, a **Cell Feature Attribute Matrix** will be created).  If a **Feature** array is passed to the **Filter**, then an **Ensemble Attribute Matrix** is created.  The following table shows what type of **Attribute Matrix** is created based on what sort of array is used for clustering:

| Attribute Matrix Source             | Attribute Matrix Created |
|------------------|--------------------|
| Generic | Generic |
| Vertex | Vertex Feature |
| Edge | Edge Feature |
| Face | Face Feature |
| Cell | Cell Feature|
| Vertex Feature | Vertex Ensemble |
| Edge Feature | Edge Ensemble |
| Face Feature | Face Ensemble |
| Cell Feature | Cell Ensemble|
| Vertex Ensemble | Vertex Ensemble |
| Edge Ensemble | Edge Ensemble |
| Face Ensemble | Face Ensemble |
| Cell Ensemble | Cell Ensemble|

This **Filter** will store the means for the final clusters within the created **Attribute Matrix**.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Number of Clusters | int32_t | The number of clusters in which to partition the array |
| Distance Metric | Enumeration | The metric used to determine the distances between points; only 2-norm metrics (i.e., Euclidean or squared Euclidean) may be chosen |
| Use Mask | bool | Whether to use a boolean mask array to ignore certain points flagged as _false_ from the algorithm |

## Required Geometry ###

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any| Any | The **Attribute Array** to cluster |
| **Attrubute Array** | Mask | bool | (1) | Specifies if the point is to be counted in the algorithm, if _Use Mask_ is checked |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix** | ClusterData | Feature/Ensemble | N/A | The **Attribute Matrix** in which to store information associated with the created clusters |
| **Attribute Array** | ClusterIds | int32_t | (1) | Specifies to which cluster each point belongs |
| **Attribute Array** | ClusterMeans | double | (1) | The means of the final clusters |

## References ##

[1] Least squares quantization in PCM, S.P. Lloyd, IEEE Transactions on Information Theory, vol. 28 (2), pp. 129-137, 1982.

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
<https://groups.google.com/forum/?hl=en#!forum/dream3d-users>
