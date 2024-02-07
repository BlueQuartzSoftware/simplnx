# Find GBCD (Metric-Based Approach)

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** computes a section through the five-dimensional grain boundary distirbution for a fixed misorientation. An example of such a section is shown in Fig. 1. Differently than **Find GBCD Filter**, which uses a method based on partition of the boundary space into bins, this **Filter** implements an alternative metric-based approach described by K. Glowinski and A. Morawiec in [Analysis of experimental grain boundary distributions based on boundary-space metrics, Metall. Mater. Trans. A 45, 3189-3194 (2014)](https://link.springer.com/article/10.1007/s11661-014-2325-y)

![Fig. 1: Section for the 17.9 deg./[111] misorientation through the grain boundary distribution obtained using this Filter for the small IN100 data set. Units are multiples of random distribution (MRDs).](Images/FindGBCDMetricBased_dist.png)

Metrics in the boundary space can be defined in a number of ways, but it is essential that two boundaries are close (distant) if they have similar (different) geometric features, and that symmetrically equivalent representations of boundaries are taken into consideration. Formally, the boundary space is a Cartesian product of the misorientation and boundary-normal subspaces. For computational reasons and because of considerably different resolutions in determinination of grain misorientation and boundary-plane parameters, it is convenient to use a separate metric in each subspace. With separate metrics, the procedure for computing distribution values for a selected misorientation has two stages. First, boundary segments with misorientations located not farther from the fixed misorientation than a limiting distance &rho;<sub>m</sub> are selected. In the second stage, the distribution is probed at evenly distributed normal directions (see Fig. 2), and areas of boundaries whose normals deviate from a given direction by less than &rho;<sub>p</sub> are summed. (The radii &rho;<sub>m</sub> and &rho;<sub>p</sub> should be tailored to resolution, amount, and quality of data and set.) Eventually, the obtained distribution is normalized in order to express it in the conventional units, i.e., multiples of the random distribution.

| Image |
|-------|
| ![](Images/FindGBCDMetricBased_samplpts.png)|
|Fig. 2: End-points (drawn in stereographic projection) of sampling directions used for probing distribution values; the number of points here is about 1500. Additionally, distributions are probed at points lying at the equator (marked with red); this is helpful for some plotting software.|

This **Filter** also calculates statistical errors of the distributions using the formula

&epsilon; = ( *f* *n* *v* )<sup>1/2</sup>

where &epsilon; is the relative error of the distribution function at a given point, *f* is the value of the function at that point, *n* stands for the number of grain boundaries (**not** the number of mesh triangles) in the considered network, and *v* denotes the volume restricted by &rho;<sub>m</sub> and &rho;<sub>p</sub>. The errors can be calculated either as their absolute values, i.e., &epsilon; &times; *f* (Fig. 3a) or as relative errors, i.e., 100% &times; &epsilon; (Fig. 3b). The latter are computed in a way that if the relative error exceeds 100%, it is rounded down to 100%.

| Image |
|-------|
| ![](Images/FindGBCDMetricBased_err2.png)|
|![Fig. 3: (a) Errors (absolute values of one standard deviation) corresponding to the distribution shown in Fig. 1. Levels are given in MRDs. (b) Relative errors (given in %) of the distribution from Fig. 1.](Images/FindGBCDMetricBased_err2.png)|

## Format of Output Files

Output files are formatted to be readable by GMT plotting program. The first line contains the fixed misorientation axis and angle. Each of the remaining lines contains three numbers. The first two columns are angles (in degrees) describing a given sampling direction; let us denote them  *col*<sub>1</sub> and *col*<sub>2</sub>, respectively. The third column is either the value of the GBCD (in MRD) for that direction or its error (in MRD or %, depending on user's selection). If you use other software, you can retrive spherical angles &theta; and &phi; of the sampling directions in the following way:

&theta; = 90&deg; - *col*<sub>1</sub>

&phi; = *col*<sub>2</sub>

Then, the directions are given as [ sin &theta; &times; cos &phi; , sin &theta; &times; sin &phi; , cos &theta; ].


## Feedback

In the case of any questions, suggestions, bugs, etc.,  please feel free to email the author of this **Filter** at kglowinski *at* ymail.com
% Auto generated parameter table will be inserted here

## References

[1] K. Glowinski and A. Morawiec, Analysis of experimental grain boundary distributions based on boundary-space metrics, Metall. Mater. Trans. A 45, 3189-3194 (2014)

## Example Pipelines

`(05) SmallIN100 GBCD Metric.d3dpipeline`

This pipeline depends on previous pipelines in the Small IN100 reconstruction pipeline series.

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
