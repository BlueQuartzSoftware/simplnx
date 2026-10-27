# Smoothing Recursive Gaussian Image Filter

Smooth (blur) an image using a separable recursive-Gaussian (Deriche) convolution with a per-axis standard deviation.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Convolves the input image with a Gaussian kernel, computed with the Deriche 4th-order recursive IIR approximation (a fast, image-size-independent approximation to a true Gaussian convolution). One separable 0th-order pass is run along each filtered axis and the passes are cascaded, so the overall effect is a full N-dimensional Gaussian blur. This suppresses noise and fine detail while preserving the overall structure of the image.

**Sigma** is specified per-axis (X, Y, Z), in the units of image spacing, so anisotropic smoothing and anisotropic spacing are both supported. Larger sigma produces stronger blurring along that axis. A `Z` size of 1 is treated as a 2D image and only the X and Y axes are filtered.

The input array must have one component (scalar) and use `int8`, `int16`, `int32`, `int64`, `float32`, or `float64`. Unsigned types are not supported. The output has the **same type, tuple shape, and component shape as the input**. This is an ITK-free implementation of the legacy filter's recursive approximation. Exact agreement for all data and scales is not guaranteed.

The filter requires at least 4 pixels along every filtered axis.

### Sigma

The standard deviation along each axis (X, Y, Z), in the units of image spacing. Default is 1 along every axis. For each active axis, Sigma must be finite and positive. Image spacing must be finite with an absolute value greater than `1e-8`. Finite negative spacing is accepted. The effective scale is Sigma divided by the absolute spacing; this quotient must be finite and positive, and the formed recurrence coefficients must be finite.

These checks reject invalid coefficient formation. They do not impose a maximum Sigma or guarantee accuracy for extreme scales. When Z has size 1, Z Sigma and Z spacing are unused and are not checked.

### Normalize Across Scale

Whether to normalize the Gaussian over scale-space (Lindeberg's scale normalization). This affects derivative magnitudes but does not change 0th-order smoothing. Default is Off.

## Algorithm

The filter applies one separable zero-order recursive-Gaussian pass along each active image axis. Resident **Data Arrays** retain the direct parallel cascade. For a three-dimensional disk-backed image, the Z pass first batches consecutive Y rows across the volume; the following X and Y passes are fused within each Z plane and cast directly to the output. A two-dimensional image fuses its Y and X passes directly from input to output and needs no full-image working array. Store reads and writes remain serial and contiguous; only staged in-memory lines are filtered in parallel.

Each recurrence uses double coefficients and arithmetic, then stores its result as **Float32 before the next axis**. This rounding is part of the filter's contract. Float64 output does not restore detail lost in those Float32 values. In particular, this filter does not preserve arbitrary Int64 intensity detail.

Resident execution retains its Float32 working image and direct parallel path. Three-dimensional disk execution uses a file-backed Float32 working image, a fixed number of plane buffers, and a Z slab whose buffers target at most 64 MiB; one XZ row is the minimum transfer unit. Two-dimensional disk execution uses bounded row or column blocks and file-backed recurrence checkpoints. Split-column plans use a file-backed Float32 working image. Scratch files consume disk space without requiring a resident array for the whole image. Store transfers remain serial.

### Final values and diagnostics

For integer output, finite final values **truncate toward zero, then clip to the native output range only if needed**. For example, an Int8 result of `127.75` becomes `127` without clipping, while `128` clips to `127`. One warning reports any clipping during a filter run. A constant Int32 maximum can round to `2147483648` between passes; its final output clips back to `2147483647` and reports the warning.

A nonfinite final value (`NaN` or either infinity) cannot be stored as an integer. The filter returns an error with the flat value index and output type. Floating output retains floating nonfinite behavior. Finite overflow when narrowing a double recurrence result to Float32 is explicitly mapped to infinity with the same sign; later arithmetic can produce NaN. No integer clipping occurs between axes.

The same terminal policy applies to all five output routes:

| Output route | Final conversion |
| --- | --- |
| Direct resident output | Checked parallel conversion of Float32 values |
| Buffered store output | Checked conversion in bounded transfer buffers |
| Two-dimensional plane cascade | Checked conversion before a plane is written |
| Three-dimensional plane cascade | Checked conversion after the Z, X, and Y passes |
| Bounded two-dimensional disk cascade | Checked conversion before each output block is written |

Warnings from completed transfers remain available if a later operation fails or the run is cancelled. Cancellation and later I/O failures can leave completed output blocks in place; this filter does not promise an all-or-nothing output update.

### Migration and downstream use

Earlier versions could use an unchecked integer conversion, including an invalid result at the Int32 maximum. Review clipping warnings when updating an existing pipeline. Clipping and recursive overshoot can change the result of later thresholding or segmentation. The array size stays unchanged. Use floating input when you need to retain fractional or nonfinite output, while allowing for the unchanged Float32 inter-pass precision.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
