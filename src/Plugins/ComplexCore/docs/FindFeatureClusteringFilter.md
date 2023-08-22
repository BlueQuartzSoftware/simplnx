# Find Feature Clustering


## Group (Subgroup)

Statistics (Morphological)

## Description

This Filter determines the radial distribution function (RDF), as a histogram, of a given set of **Features**. Currently, the **Features** need to be of the same **Ensemble** (specified by the user), and the resulting RDF is stored as **Ensemble** data. This Filter also returns the clustering list (the list of all the inter-**Feature** distances) and the minimum and maximum separation distances. The algorithm proceeds as follows:

1. Find the Euclidean distance from the current **Feature** centroid to all other **Feature** centroids of the same specified phase
2. Put all caclulated distances in a clustering list
3. Repeat 1-2 for all **Features**
4. Sort the data into the specified number of bins, all equally sized in distance from the minimum distance to the maximum distance between **Features**. For example, if the user chooses 10 bins, and the minimum distance between **Features** is 10 units and the maximum distance is 80 units, each bin will be 8 units 
5. Normalize the RDF by the probability of finding the **Features** if distributed randomly in the given box 

*Note:* Because the algorithm iterates over all the **Features**, each distance will be double counted. For example, the distance from **Feature** 1 to **Feature** 2 will be counted along with the distance from **Feature** 2 to **Feature** 1, which will be identical. 

## Parameters

| Name | Type | Description |
|------|------| ----------- |
| Number of Bins for RDF | int32_t | Number of bins to split the RDF |
| Phase Index | int32_t | **Ensemble** number for which to calculate the RDF and clustering list |
| Remove Biased Features | bool | Default=OFF |
| Set Random Seed | bool | When checked, allows the user to set the seed value used to randomly generate the points in the RDF (Default=ON) |
| Seed Value | uint64_t | The seed value used to generate the points in the RDF when the Set Random Seed option is ON |

## Required Geometry

Image

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | EquivalentDiameters | float | (1) | Diameter of a sphere with the same volume as the **Feature** |
| **Feature Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| **Feature Attribute Array** | Centroids | float | (3) | X, Y, Z coordinates of **Feature** center of mass |
| **Feature Attribute Array** | BiasedFeatures | bool | (1) | Specifies which features are biased and therefor should be removed if the Remove Biased Features option is on |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | ClusteringList | float | (1) | Distance of each **Features**'s centroid to ever other **Features**'s centroid |
| **Ensemble Attribute Array** | RDF | float | (Number of Bins) | A histogram of the normalized frequency at each bin | 
| **Ensemble Attribute Array** | RDFMaxMinDistances | float | (2) | The max and min distance found between **Features** |

## Example Pipelines

+ PorosityAnalysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: FindFeatureClusteringFilter
+ Displayed Name: Find Feature Clustering

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| biased_features_array_path | Biased Features | Specifies which features are biased and therefor should be removed if the Remove Biased Features option is on | complex.ArraySelectionParameter |
| cell_ensemble_attribute_matrix_name | Cell Ensemble Attribute Matrix | The path to the cell ensemble attribute matrix where the RDF and RDF min and max distance arrays will be stored | complex.AttributeMatrixSelectionParameter |
| centroids_array_path | Centroids | X, Y, Z coordinates of Feature center of mass | complex.ArraySelectionParameter |
| clustering_list_array_name | Clustering List | Distance of each Features's centroid to ever other Features's centroid | complex.DataObjectNameParameter |
| equivalent_diameters_array_path | Equivalent Diameters | Diameter of a sphere with the same volume as the Feature | complex.ArraySelectionParameter |
| feature_phases_array_path | Phases | Specifies to which Ensemble each Feature belongs | complex.ArraySelectionParameter |
| max_min_array_name | Max and Min Separation Distances | The max and min distance found between Features | complex.DataObjectNameParameter |
| number_of_bins | Number of Bins for RDF | Number of bins to split the RDF | complex.Int32Parameter |
| phase_number | Phase Index | Ensemble number for which to calculate the RDF and clustering list | complex.Int32Parameter |
| rdf_array_name | Radial Distribution Function | A histogram of the normalized frequency at each bin | complex.DataObjectNameParameter |
| remove_biased_features | Remove Biased Features | Remove the biased features | complex.BoolParameter |
| seed_value | Seed Value | The seed value used to randomly generate the points in the RDF | complex.UInt64Parameter |
| selected_image_geometry | Selected Image Geometry | The target geometry | complex.GeometrySelectionParameter |
| set_random_seed | Set Random Seed | When checked, allows the user to set the seed value used to randomly generate the points in the RDF | complex.BoolParameter |

