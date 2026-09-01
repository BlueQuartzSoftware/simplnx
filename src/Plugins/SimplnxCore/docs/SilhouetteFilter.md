# Silhouette

## Group (Subgroup)

DREAM3D Review (Clustering)

## Description

This **Filter** computes the silhouette for a clustered **Attribute Array**.  The user must select both the original array that has been clustered and the array of cluster Ids.  The silhouette is a dimensionless measure of the quality of a clustering.  Specifically, the silhouette provides a measure of how strongly a given point belongs to its own cluster compared to all other clusters.  The silhouette is computed as follows:

$$ s_{i} = \frac{b_{i} - a_{i}}{\max\{a_{i},b_{i}\}} $$

where $a$ is the average distance between point $i$ and all other points in the cluster point $i$ belongs to, $b$ is the *next closest* average distance among all other clusters, and $s$ is the silhouette value.  Using this definition, $s$ exists on the interval $[-1, 1]$ (dimensionless), where 1 indicates that the point strongly belongs to its current cluster and -1 indicates that the point does not belong well to its current cluster.  The user may select from a variety of options to use as the distance metric.  Additionally, the user may opt to use a mask array to ignore points in the silhouette; these points will contain a silhouette value of 0.

The filter writes one float silhouette value per input point into the created silhouette array, located alongside the input array. Each value lies in the range $[-1, 1]$ (dimensionless).

The silhouette can be used to determine how well a particular clustering has performed, such as a clustering produced by [Compute K Means](ComputeKMeansFilter.md) or [Compute K Medoids](ComputeKMedoidsFilter.md).

## Algorithm

For in-memory arrays, the filter retains the original direct pairwise implementation. For out-of-core or mixed-storage inputs, it dispatches to a bounded Scanline implementation. The Scanline path reads clustering values, feature IDs, and the optional Bool or UInt8 mask in fixed tuple tiles with bulk `copyIntoBuffer()` calls. It densifies sparse positive feature IDs into feature-scale state, accumulates exact pair distances for one bounded outer tile against bounded inner tiles, and bulk-writes the resulting silhouette tile. It never creates an all-true cell mask or resident cell-sized membership, distance, or output scratch arrays. Feature-zero, self-distance, denominator, and distance-metric behavior follow the direct implementation.

### Distance Metric

The *Distance Metric* parameter controls how the distance between two points is calculated when computing silhouette values:

- **Euclidean [0]**: Standard straight-line distance between two points.
- **Squared Euclidean [1]**: Square of the Euclidean distance, avoiding the square root computation.
- **Manhattan [2]**: Sum of the absolute differences of coordinates (L1 / city-block distance).
- **Cosine [3]**: Measures the cosine of the angle between two vectors.
- **Pearson [4]**: Correlation-based distance derived from the Pearson correlation coefficient.
- **Squared Pearson [5]**: Square of the Pearson distance.

### Required Input Sources

- **Cluster Ids** -- the per-point cluster label array, produced by [Compute K Means](ComputeKMeansFilter.md) or [Compute K Medoids](ComputeKMedoidsFilter.md).
- **Clustered Attribute Array** -- the original **Attribute Array** that was clustered (the same array passed to the clustering filter).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
