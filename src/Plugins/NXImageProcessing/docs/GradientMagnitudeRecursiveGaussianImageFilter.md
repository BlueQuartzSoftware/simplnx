# Gradient Magnitude Recursive Gaussian Image Filter

Compute the gradient magnitude of an image using separable recursive-Gaussian (Deriche) derivatives.

## Group (Subgroup)

ImageProcessing (EdgeDetection)

## Description

Computes, at every voxel, the magnitude of the gradient of the Gaussian-smoothed input image: `sqrt(sum over axes of (d/dx_i G*I)^2)`. Along each filtered axis the filter takes the first derivative of the image convolved with a Gaussian, using the Deriche 4th-order recursive IIR approximation (a fast, image-size-independent approximation to a true Gaussian convolution), then combines the per-axis derivatives into the gradient magnitude. The derivative along each axis is divided by the image spacing along that axis, so the result respects anisotropic spacing.

**Sigma** is a single scalar shared by every axis, expressed in the units of image spacing. Larger sigma suppresses more noise and detects coarser edges. A `Z` size of 1 is treated as a 2D image and only the X and Y axes are filtered.

The input array must be single-component (scalar) and may be **any numeric type, integer or floating point**. The output is a fixed **float32** image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Gradient Magnitude Recursive Gaussian Image Filter, and matches it exactly.

The filter requires at least 4 pixels along every filtered axis.

### Sigma

The standard deviation of the Gaussian, in the units of image spacing. Default is 1.

### Normalize Across Scale

Whether to normalize the Gaussian derivative over scale-space (Lindeberg's scale normalization, which makes the derivative magnitude comparable across different sigma). Default is Off.

## Algorithm

The filter applies separable recursive-Gaussian passes along each active image axis and combines the resulting first derivatives into a gradient magnitude. Resident **Data Arrays** retain the direct parallel cascade. For disk-backed arrays, consecutive X and Y passes are fused within one Z plane: the plane is read once, both recurrences run on bounded plane buffers, and the completed derivative is written directly to a bounded working or accumulator store. A Z-axis pass still batches consecutive Y rows across the volume. The last derivative contribution and final square root are combined with the terminal plane write. Store reads and writes remain serial and contiguous; only staged in-memory lines are filtered in parallel.

Each recurrence explicitly materializes its result as `float32` before the next axis. The accumulator is likewise rounded to `float32` before the final square root, preserving the legacy numerical sequence. Working memory is bounded by a fixed number of image planes or a Z slab rather than the full **Image Geometry**. Combined Z-staging buffers target at most 64 MiB; one XZ row is the indivisible minimum. For a genuinely out-of-core endpoint, two full-volume intermediates are used -- an uncompressed working store and an uncompressed accumulator store, both independent of the endpoint's own storage format -- so the running sum accumulates without repeatedly reading and writing the output array's real storage; the finished, square-rooted result is written into the output array once, at the end. Resident arrays forced through this fallback instead use a single intermediate that adopts the input's format, so mixed storage still cannot create a full-volume resident scratch copy.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
