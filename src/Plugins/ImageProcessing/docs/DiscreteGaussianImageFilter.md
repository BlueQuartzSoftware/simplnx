# Discrete Gaussian Image Filter

Blur an image with a separable discrete-Gaussian (FIR) convolution.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Convolves the input with a discrete Gaussian along each axis. The 1-D kernel for each axis is built from the discrete Gaussian (a normalized, symmetric modified-Bessel kernel — a verbatim reproduction of ITK's `GaussianOperator`), sized so that the error from truncating the kernel tails is no greater than **Maximum Error** along that axis and never wider than **Maximum Kernel Width** pixels. The per-axis convolutions are cascaded axis by axis, with a zero-flux (Neumann) boundary condition at the image edges. A `Z` size of 1 is treated as a 2D image and only the X and Y axes are filtered.

The input array must be single-component (scalar) and may be **any numeric type, integer or floating point**. The output has the **same type as the input**; for integer types the intermediate result is truncated to the integer type between axis passes, exactly as the legacy ITK filter does. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Discrete Gaussian Image Filter, and matches it exactly.

### Variance

The variance of the discrete Gaussian kernel, set independently for each dimension (X, Y, Z). If **Use Image Spacing** is on the units are the physical units of the image; if it is off the units are pixels. Default is 1.0 in each dimension.

### Maximum Kernel Width

The kernel is sized to be no wider than this many pixels, even if **Maximum Error** would demand a wider kernel. Default is 32 pixels.

### Maximum Error

The kernel is sized so that the error from truncating its tails is no greater than this value, set independently for each dimension. Default is 0.01 in each dimension.

### Use Image Spacing

Whether the filter accounts for the image spacing when building the kernel. Use On to specify the Gaussian variance in real-world units (each axis's variance is divided by that axis's spacing squared); use Off to specify the variance in voxel units. Default is On.

## Algorithm

The filter builds a normalized, symmetric one-dimensional kernel for each active axis and applies the kernels as a separable convolution in descending axis order. Each line is accumulated in double precision and converted back to the input type between passes. The boundary value is clamped to the nearest edge voxel.

For disk-backed arrays, the Z pass batches consecutive Y rows across the volume into a bounded staging slab. The following Y and X passes are fused into one transaction per Z plane: the Y result is retained in a plane of the input type so integer conversion between axes remains unchanged, then X is applied before the plane is written to the output. This avoids writing and rereading the full disk-backed intermediate between the two plane-local passes. Store reads and writes remain serial and contiguous; only the staged in-memory lines are convolved in parallel. A 2D image uses the same fused plane transaction without creating an intermediate store.

The working memory is bounded by two image planes or a fixed-size Z slab rather than the full **Image Geometry**, and the 3D intermediate array retains the selected storage format. Resident arrays continue to use the direct axis-by-axis cascade.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
