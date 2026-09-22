# Gradient Magnitude Image Filter

Compute the gradient magnitude of an image using order-1 central-difference derivatives.

## Group (Subgroup)

ImageProcessing (EdgeDetection)

## Description

Computes, at every voxel, the magnitude of the gradient of the input image: `sqrt(sum over axes of (d/dx_i I)^2)`. Along each axis the derivative is the order-1 central difference `0.5 * (I[x_i - 1] - I[x_i + 1])`, i.e. the discrete `[0.5, 0, -0.5]` derivative operator, and the per-axis derivatives are combined into the gradient magnitude (accumulated in double precision). Image edges use a zero-flux (Neumann) boundary condition (the out-of-bounds neighbor is clamped to the edge voxel). A `Z` size of 1 is treated as a 2D image and only the X and Y axes contribute.

The input array must be single-component (scalar) and may be **any numeric type, integer or floating point**. The output is a fixed **float32** image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Gradient Magnitude Image Filter, and matches it exactly.

### Use Image Spacing

Whether the filter uses the spacing of the input image when computing the derivatives. Use On to compute the gradient in physical space (each axis's derivative is divided by the image spacing along that axis); use Off to ignore image spacing and compute the gradient in isotropic voxel space. Default is On.

## Algorithm

For in-memory images, the filter streams through Z using a three-plane rolling input window and one output plane. Each output voxel is independent once its neighboring values are resident, so computation within a plane is parallel.

For a 3D image with an out-of-core input or output, the filter first asks the shared cache budget for enough temporary working memory to hold the complete input, output, and rolling planes. A complete grant lets the filter read the input once, run the same calculation in memory, and write the output once. A partial grant is released immediately and the filter uses the bounded rolling-plane route instead. This keeps the faster route coordinated with other cache users and preserves a low-memory fallback.

For a true 2D image with an out-of-core input or output, a complete XY plane could exceed available memory. The filter instead uses a checked 64 MiB working-memory target. Normal-width images are processed as full-width row blocks with one input halo row above and below. If even three input rows plus one output row cannot fit, each row is processed as bounded X tiles with one-column and one-row halos. Both routes use bulk storage transfers and preserve the same central-difference order, image-spacing scaling, and edge clamping as the in-memory calculation.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
