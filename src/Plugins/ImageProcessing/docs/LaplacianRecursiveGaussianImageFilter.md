# Laplacian Recursive Gaussian Image Filter

Compute the Laplacian of an image using separable recursive-Gaussian (Deriche) second derivatives.

## Group (Subgroup)

ImageProcessing (EdgeDetection)

## Description

Computes, at every voxel, the Laplacian of the Gaussian-smoothed input image: the sum over axes of the second derivative `sum over axes of (d^2/dx_i^2 G*I)`. Along each filtered axis the filter takes the second derivative of the image convolved with a Gaussian, using the Deriche 4th-order recursive IIR approximation (a fast, image-size-independent approximation to a true Gaussian convolution), then sums the per-axis second derivatives. The second derivative along each axis is divided by the square of the image spacing along that axis, so the result respects anisotropic spacing. Unlike the gradient magnitude, the per-axis terms are summed directly (there is no squaring and no final square root), so the output is signed.

**Sigma** is a single scalar shared by every axis, expressed in the units of image spacing. Larger sigma suppresses more noise and detects coarser features. A `Z` size of 1 is treated as a 2D image and only the X and Y axes are filtered.

The input array must be single-component (scalar) and may be **any numeric type, integer or floating point**. The output is a fixed **float32** image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Laplacian Recursive Gaussian Image Filter, and matches it exactly.

The filter requires at least 4 pixels along every filtered axis.

### Sigma

The standard deviation of the Gaussian, in the units of image spacing. Default is 1.

### Normalize Across Scale

Whether to normalize the Gaussian derivative over scale-space (Lindeberg's scale normalization, which makes the derivative magnitude comparable across different sigma). Default is Off.

## Algorithm

The filter applies separable recursive-Gaussian passes along each active image axis and sums the resulting second derivatives. Resident **Data Arrays** use the direct parallel cascade. For a true-3-D image with an out-of-core endpoint, the filter first asks the shared cache budget for the complete typed input, float32 accumulator, and float32 work image. A complete grant performs one full input read, runs the same resident cascade, and performs one full output write. The certified float32 workload requests 384 MiB; float64 input of the same dimensions requests 512 MiB.

A partial grant or allocation failure retains the disk-backed cascade. Consecutive X and Y passes are fused within one Z plane: the plane is read once, both recurrences run on bounded plane buffers, and the completed derivative is written directly to a bounded working or accumulator store. A Z-axis pass still batches consecutive Y rows across the volume. Store reads and writes remain serial and contiguous; only staged in-memory lines are filtered in parallel. True 2-D keeps its checked row/tile and raw temporary-record routes.

Each fallback recurrence explicitly materializes its result as `float32` before the next axis, and spacing-scaled terms use the same double-expression-to-`float32` accumulation sequence as the resident path. Fallback working memory is bounded by a fixed number of image planes or a Z slab rather than the full **Image Geometry**. Combined Z-staging buffers target at most 64 MiB; one XZ row is the indivisible minimum. For a genuinely out-of-core endpoint, two full-volume disk-backed intermediates are used -- an uncompressed working store and an uncompressed accumulator store, both independent of the endpoint's own storage format -- so the running sum accumulates without repeatedly reading and writing the output array's real storage; the finished result is written into the output array once, at the end. Forced-fallback testing on fully resident endpoints instead uses a single intermediate that adopts the input's format, so mixed storage still cannot create an unbudgeted full-volume resident scratch copy.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
