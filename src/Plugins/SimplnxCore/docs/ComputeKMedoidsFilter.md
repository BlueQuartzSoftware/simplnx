# Compute K Medoids

## Group (Subgroup)

DREAM3D Review (Clustering)

## Description

***Warning:* The randomness in this filter is not currently consistent between operating systems even if the same seed is used. Specifically between Unix and Windows. This does not affect the results, but the IDs will not correspond. For example if the Cluster Identifier at index one on Linux is 1 it could be 2 on Windows, the overarching clusters will be the same, but their IDs will be different.**

This **Filter** applies the k medoids algorithm to an **Attribute Array**.  K medoids is a *clustering algorithm* that assigns each point (each tuple) of the **Attribute Array** to a *cluster Id*.  A **cluster** is a group of data points that are more similar to each other than to points in other groups.  The user must specify the number of clusters (a dimensionless count, *k*) in which to partition the array.  Specifically, a k medoids partitioning is such that each point in the data set is associated with the cluster that minimizes the sum of the pair-wise distances between the data points and their associated cluster centers (medoids).  This approach is analogous to k means, but uses actual data points (the medoids) as the cluster exemplars instead of the means.  Medoids in this context refer to the data point in each cluster that is most like all other data points, i.e., that data point whose average distance to all other data points in the cluster is smallest.  Unlike k means, since pair-wise distances are minimized instead of variance, any arbitrary concept of "distance" may be used; this **Filter** allows for the selection of a variety of distance metrics.

### Distance Metric

The *Distance Metric* parameter determines how distances between data points are measured when assigning points to clusters and selecting medoids:

- **Euclidean [0]**: Standard straight-line distance between two points in the data space.
- **Squared Euclidean [1]**: The square of the Euclidean distance. Avoids a square root computation and gives extra weight to larger differences.
- **Manhattan [2]**: Sum of the absolute differences along each dimension (also known as L1 or city-block distance).
- **Cosine [3]**: One minus the cosine similarity between two points. Measures the angle between vectors, making it invariant to magnitude.
- **Pearson [4]**: One minus the Pearson correlation coefficient. Measures the linear correlation between two points, normalized by their standard deviations.
- **Squared Pearson [5]**: The square of the Pearson distance metric.

This **Filter** uses the *Voronoi iteration* algorithm to produce the clustering.  A **Voronoi** partitioning assigns every point to the cluster whose center (here, the medoid) is closest, carving the data space into regions where each region holds all the points nearer to one medoid than to any other.  The algorithm is iterative and proceeds as follows:

1. Choose k points at random to serve as the initial cluster medoids
2. Associate each point to the closest medoid
3. Until convergence, repeat the following steps:

- For each cluster, change the medoid to the point in that cluster that minimizes the sum of distances between that point and all other points in the cluster
- Reassign each point to the closest medoid

Convergence is defined as when the medoids no longer change position.  Since the algorithm is iterative, it only serves as an approximation, and may result in different classifications on each execution with the same input data.  The user may opt to use a mask to ignore certain points; where the mask is *false*, the points will be placed in cluster 0.

Note: In SIMPLNX there is no explicit positional subtyping for Attribute Matrix, so the next section should be treated as a high-level understanding of what is being created. Naming the Attribute Matrix to include the type listed on the respective line in the 'Attribute Matrix Created' column is encouraged to help with readability and comprehension.

A clustering algorithm can be considered a kind of segmentation; this implementation of k medoids does not rely on the **Geometry** on which the data lie, only on the arrangement of the values within the array itself (the *topology* of the space that the array forms).  Therefore, this **Filter** has the effect of creating either **Features** or **Ensembles** depending on the kind of array passed to it for clustering.  If an **Element** array (e.g., voxel-level **Cell** data) is passed to the **Filter**, then **Features** are created (in the previous example, a **Cell Feature Attribute Matrix** will be created).  If a **Feature** array is passed to the **Filter**, then an **Ensemble Attribute Matrix** is created.  The following table shows what type of **Attribute Matrix** is created based on what sort of array is used for clustering:

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

The filter creates a *cluster Ids* array (one dimensionless integer cluster label per input point) alongside the input array, and stores the final cluster medoids (one medoid vector per cluster, in the units of the input array) within the created **Attribute Matrix**.

### Related Filters

- [Compute K Means](ComputeKMeansFilter.md) uses the arithmetic mean of each cluster as its center instead of a representative data point.
- [DBSCAN](DBSCANFilter.md) is a density-based clustering algorithm that does not require the number of clusters to be specified in advance.
- [Silhouette](SilhouetteFilter.md) evaluates the quality of a clustering produced by this filter.

### Required Input Sources

- **Clustered Attribute Array** -- any **Attribute Array** (cell-level, feature-level, or generic) whose tuples are to be partitioned into clusters. This is typically an output of an earlier computation or import step.

## Algorithm

This filter has two algorithm implementations that are automatically selected at runtime based on how the input data is stored. The user does not need to choose between them.

### In-Core Algorithm (Direct)

When all input arrays reside in memory, the **Direct** algorithm is used. It accesses array elements via direct per-element operator[] calls, which are optimal for in-memory data (essentially pointer dereferences).

The algorithm performs the standard Voronoi iteration:

1. **Initialize**: Randomly select k data points as initial medoids
2. **Assign clusters**: For each data point, compute the distance to all k medoids and assign it to the nearest
3. **Optimize medoids**: For each cluster, find the member that minimizes the total intra-cluster distance
4. **Repeat** steps 2-3 until medoids stop changing (convergence)

### Out-of-Core Algorithm (Scanline)

When any input array is backed by chunked on-disk storage (out-of-core), the **Scanline** algorithm is used. Out-of-core data lives in compressed chunks on disk; each per-element operator[] access would trigger a chunk load/decompress/evict cycle ("chunk thrashing"), making the iterative algorithm extremely slow.

The Scanline algorithm uses bounded bulk I/O throughout:

- **Medoid caching**: The medoids array is small (k points), so it is cached entirely in a local buffer before each cluster assignment pass, eliminating k * N per-element OOC reads per iteration.
- **Chunked cluster assignment**: The input data and cluster IDs are read and written in fixed-size bounded tiles via bulk I/O (copyIntoBuffer/copyFromBuffer). All distance computations for each tile are done in memory.
- **Blocked medoid optimization**: A fixed candidate tile is compared against one fixed inner tile at a time. Cluster IDs and an optional Bool or UInt8 mask are read in the same bulk tiles, and each candidate accumulates only matching cluster IDs. Candidate costs and medoid indices are feature-scale; no cell-sized membership list, temporary mask, or random tuple transfer is retained.
- **Eligibility and errors**: A bounded mask pre-scan rejects empty input or an all-false mask before random initialization, so the seeded rejection sampler cannot hang. The original random generator and draw order are otherwise preserved.

The dispatcher selects Scanline when the clustering input, feature-ID output, medoids output, or enabled mask is out-of-core. The no-mask case carries that choice explicitly rather than creating a full all-true temporary array.

### Performance

The in-core Direct algorithm is faster for in-memory data due to the lower overhead of operator[] access. The out-of-core Scanline algorithm converts random per-element access into sequential bulk I/O, which is essential for data stored on disk in compressed chunks. Both produce identical clustering results.

% Auto generated parameter table will be inserted here

## References

[1] A simple and fast algorithm for K-medoids clustering, H.S. Park and C.H. Jun, Expert Systems with Applications, vol. 28 (2), pp. 3336-3341, 2009.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
