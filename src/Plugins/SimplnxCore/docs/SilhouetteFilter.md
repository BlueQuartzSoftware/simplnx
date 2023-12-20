# Silhouette

## Group (Subgroup)

DREAM3D Review (Clustering)

## Description

This **Filter** computes the silhouette for a clustered **Attribute Array**.  The user must select both the original array that has been clustered and the array of cluster Ids.  The silhouette represents a measure for the quality of a clustering.  Specifically, the silhouette provides a measure for how strongly a given point belongs to its own cluster compared to all other clusters.  The silhouette is computed as follows:

\f[ s_{i} = \frac{b_{i} - a_{i}}{\max\{a_{i},b_{i}\}} \f]

where \f$ a \f$ is the average distance between point \f$ i \f$ and all other points in the cluster point \f$ i \f$ belongs to, \f$ b \f$ is the *next closest* average distance among all other clusters, and \f$ s \f$ is the silhouette value.  Using this definition, \f$ s \f$ exists on the interval \f$ [-1, 1] \f$, where 1 indicates that the point strongly belongs to its current cluster and -1 indicates that the point does not belong well to its current cluster.  The user may select from a variety of options to use as the distance metric.  Additionally, the user may opt to use a mask array to ignore points in the silhouette; these points will contain a silhouette value of 0.

The silhouette can be used to determine how well a particular clustering has performed, such as k means or k medoids.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
