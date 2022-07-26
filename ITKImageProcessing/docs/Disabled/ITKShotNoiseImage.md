# ITK::Shot Noise Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Alter an image with shot noise.

The shot noise follows a Poisson distribution:

\par
\f$ I = N(I_0) \f$

\par
where \f$ N(I_0) \f$ is a Poisson-distributed random variable of mean \f$ I_0 \f$ . The noise is thus dependent on the pixel intensities in the image.

The intensities in the image can be scaled by a user provided value to map pixel values to the actual number of particles. The scaling can be seen as the inverse of the gain used during the acquisition. The noisy signal is then scaled back to its input intensity range:

\par
\f$ I = \frac{N(I_0 \times s)}{s} \f$

\par
where \f$ s \f$ is the scale factor.

The Poisson-distributed variable \f$ \lambda \f$ is computed by using the algorithm:

\par
\f$ \begin{array}{l} k \leftarrow 0 \\ p \leftarrow 1 \\ \textbf{repeat} \\ \left\{ \begin{array}{l} k \leftarrow k+1 \\ p \leftarrow p \ast U() \end{array} \right. \\ \textbf{until } p > e^{\lambda} \\ \textbf{return} (k) \end{array} \f$

\par
where \f$ U() \f$ provides a uniformly distributed random variable in the interval \f$ [0,1] \f$ .

This algorithm is very inefficient for large values of \f$ \lambda \f$ , though. Fortunately, the Poisson distribution can be accurately approximated by a Gaussian distribution of mean and variance \f$ \lambda \f$ when \f$ \lambda \f$ is large enough. In this implementation, this value is considered to be 50. This leads to the faster algorithm:

\par
\f$ \lambda + \sqrt{\lambda} \times N()\f$

\par
where \f$ N() \f$ is a normally distributed random variable of mean 0 and variance 1.

\author Gaetan Lehmann

This code was contributed in the Insight Journal paper "Noise
Simulation". https://hdl.handle.net/10380/3158

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Scale | double| Set/Get the value to map the pixel value to the actual particle counting. The scaling can be seen as the inverse of the gain used during the acquisition. The noisy signal is then scaled back to its input intensity range. Defaults to 1.0. |
| Seed | double| N/A |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None |  | (1)  | Array containing filtered image

## References ##

[1] T.S. Yoo, M. J. Ackerman, W. E. Lorensen, W. Schroeder, V. Chalana, S. Aylward, D. Metaxas, R. Whitaker. Engineering and Algorithm Design for an Image Processing API: A Technical Report on ITK - The Insight Toolkit. In Proc. of Medicine Meets Virtual Reality, J. Westwood, ed., IOS Press Amsterdam pp 586-592 (2002). 
[2] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Design and Functionality. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-28-3
[3] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Introduction and Development Guidelines. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-27-6

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
