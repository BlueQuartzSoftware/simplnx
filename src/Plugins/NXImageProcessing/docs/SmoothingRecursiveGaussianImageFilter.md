# Smoothing Recursive Gaussian Image Filter

Smooth (blur) an image using a separable recursive-Gaussian (Deriche) convolution with a per-axis standard deviation.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Convolves the input image with a Gaussian kernel, computed with the Deriche 4th-order recursive IIR approximation (a fast, image-size-independent approximation to a true Gaussian convolution). One separable 0th-order pass is run along each filtered axis and the passes are cascaded, so the overall effect is a full N-dimensional Gaussian blur. This suppresses noise and fine detail while preserving the overall structure of the image.

**Sigma** is specified per-axis (X, Y, Z), in the units of image spacing, so anisotropic smoothing and anisotropic spacing are both supported. Larger sigma produces stronger blurring along that axis. A `Z` size of 1 is treated as a 2D image and only the X and Y axes are filtered.

The input array must be single-component (scalar) and must be a **signed** type: `int8`, `int16`, `int32`, `int64`, `float32`, or `float64`. Unsigned types are not allowed because the recursive filter has a small negative overshoot that would underflow an unsigned round-trip. The output has the **same type as the input** (type-preserving). This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Smoothing Recursive Gaussian Image Filter, and matches it exactly for integer and float64 inputs.

The filter requires at least 4 pixels along every filtered axis.

### Sigma

The standard deviation of the Gaussian along each axis (X, Y, Z), in the units of image spacing. Default is 1 along every axis.

### Normalize Across Scale

Whether to normalize the Gaussian over scale-space (Lindeberg's scale normalization). This affects derivative magnitudes but does not change 0th-order smoothing. Default is Off.

## Algorithm

The filter applies one separable zero-order recursive-Gaussian pass along each active image axis. Resident **Data Arrays** retain the direct parallel cascade. For a three-dimensional disk-backed image, the Z pass first batches consecutive Y rows across the volume; the following X and Y passes are fused within each Z plane and cast directly to the output. A two-dimensional image fuses its Y and X passes directly from input to output and needs no full-image working array. Store reads and writes remain serial and contiguous; only staged in-memory lines are filtered in parallel.

Each recurrence explicitly materializes its result as `float32` before the next axis, and the terminal plane is cast back to the input type. Three-dimensional working memory is bounded by a fixed number of image planes plus a Z slab whose combined buffers target at most 64 MiB; one XZ row is the indivisible minimum. The single full-volume intermediate adopts an out-of-core endpoint's format. Two-dimensional disk-backed execution uses only plane buffers and therefore creates no full-volume scratch array.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
