# Compute Feature Clustering

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

% Auto generated parameter table will be inserted here

## Example Pipelines

+ PorosityAnalysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
