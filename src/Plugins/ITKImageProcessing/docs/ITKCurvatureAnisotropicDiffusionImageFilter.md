# ITK Curvature Anisotropic Diffusion Image Filter

Edge-preserving smoothing that blurs within regions but not across edges, using the modified curvature diffusion equation (MCDE).

## Group (Subgroup)

ITKAnisotropicSmoothing (AnisotropicSmoothing)

## Description

**Anisotropic diffusion** is an edge-preserving smoothing technique. Ordinary (Gaussian) blurring smooths everywhere equally and washes out edges. Anisotropic diffusion instead smooths strongly inside uniform regions but weakly across boundaries, so noise is reduced while real edges stay sharp. The image is evolved over a number of small time steps, as if heat were diffusing through it but slowed down wherever the image changes rapidly.

This filter uses the **modified curvature diffusion equation (MCDE)**, a variant that tends to preserve fine, high-curvature detail better than the plain gradient-based formulation (see [ITK Gradient Anisotropic Diffusion Image Filter](ITKGradientAnisotropicDiffusionImageFilter.md) for the classic Perona-Malik version).

The input and output must be a scalar image with a floating-point pixel type (float or double).

### Parameter Guidance

- **Number Of Iterations** — how many diffusion time steps to apply. More iterations produce a more strongly smoothed result. The right number depends on the image and the amount of noise; start small and increase as needed.
- **Time Step** — the size of each diffusion step (dimensionless). It must stay below a stability limit that depends on image dimensionality: for unit pixel spacing, stable values are below **0.125** for 2D and below **0.0625** for 3D images. The filter clamps an unstable value and issues a run-time warning. The default (0.0625) is the maximum theoretically stable value for 3D.
- **Conductance Parameter** — controls how strongly edges are preserved. **Lower values preserve features more strongly** (less diffusion across high-gradient regions); higher values let the filter smooth across features more readily. Typical values range from **0.5 to 2.0**; the best value depends on the data and the iteration count.
- **Conductance Scaling Update Interval** — how often (in iterations) the internal conductance term is recomputed. The default of *1* updates it every iteration; larger values update less frequently and run slightly faster at some cost in accuracy.

### Required Input Sources

Operates on any scalar (floating-point) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

## See Also

- [ITK AnisotropicDiffusionImageFilter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1AnisotropicDiffusionImageFilter.html)
- [ITK CurvatureNDAnisotropicDiffusionFunction (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1CurvatureNDAnisotropicDiffusionFunction.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
