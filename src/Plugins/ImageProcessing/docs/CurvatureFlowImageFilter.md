# Curvature Flow Image Filter

Denoise an image using curvature-driven flow, a PDE-based finite-difference filter.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is evolved using a curvature-based speed function:

I_t = kappa * |grad(I)|

where kappa is the curvature of the iso-brightness contour passing through each voxel. The advantage of this approach is that sharp boundaries are preserved while smoothing occurs within a region; continuous application of this scheme will eventually remove all information as each contour shrinks to zero and disappears.

The evolution is computed with a multi-threaded finite-difference solver: at every iteration, an update is computed at each voxel from a frozen (Jacobi) neighborhood of the current image using a zero-flux Neumann (edge-clamp) boundary condition, and then `Output = Output + TimeStep * Update` is applied everywhere. The per-axis image spacing is used when computing derivatives, so the result depends on the Image Geometry's spacing.

The input array must be single-component (scalar). It can use any signed integer, unsigned integer, float32, or float64 type. The output has the **same type and tuple shape as the input**. All working iterations use float64 values. Integer values are converted only once, after the last iteration; they are not truncated after each update.

For integer output, each final value must be finite. The filter truncates the value toward zero, then clips it to the output type's range if needed. For example, 93.75 becomes 93. For uint8 output, -0.5 becomes 0 without clipping, while -1.5 clips to 0. If any value is clipped, the filter reports one warning for the execution. The warning states that the evolution may be unstable. A nonfinite final integer value causes an execution error instead of an unsafe conversion.

Floating output retains NaN and infinity behavior. A finite result beyond the float32 range becomes signed infinity explicitly. float64 output retains the float64 result. Input and working precision still limit accuracy: ordinary evolution can lose integer detail above 2^53, even for int64 and uint64 output. Changing the output type does not restore that detail.

Time Step must be finite and nonnegative. Spacing on each active axis must be finite and nonzero; finite negative spacing remains accepted. X and Y are active in a 2D image. Z is active only when the Z dimension is greater than one. Unused Z spacing is ignored in 2D. The filter validates these controls even for a no-op request. A zero Time Step or zero Number Of Iterations then copies input values exactly, including native 64-bit integers, without float64 conversion.

Pipelines that previously produced an out-of-range integer cast now receive clipped values and a warning, or an error for a nonfinite integer result. Clipping can create plateaus, change masks, and merge segmented regions. Check downstream thresholds and segmentation when the warning occurs. Clipping does not correct an unstable evolution.

The timestep should be small enough to ensure numerical stability. Stability is guaranteed when the timestep meets the CFL (Courant-Friedrichs-Levy) condition; broadly speaking, this condition ensures that each contour does not move more than one grid position per timestep. The timestep typically must be manually tuned to the application.

## Algorithm

For resident arrays, the solver keeps its intermediate and update images resident, computes each frozen-neighborhood update through contiguous spans, and applies updates in parallel without staging copies.

For a 3D image with an out-of-core input or output, the solver first asks the shared cache budget for the complete input, output, intermediate, update, and conversion-plane state. A complete grant reads the input once, runs the same resident Jacobi calculation, and writes the output once. A partial grant or allocation failure uses the bounded fallback: a three-plane rolling neighborhood plus two raw temporary-record working stores, with initial/final conversions and updates transferred one Z plane at a time. For true 2D, the existing checked row-block and X-tile plan remains bounded to 64 MiB.

The final conversion uses the existing resident, plane, or tile buffers. It does not add a full-image resident temporary to the bounded routes. Exact no-op copies use a transfer buffer of at most 1 MiB. Output storage size does not increase.

### Time Step

The timestep between iteration updates. It must be finite and nonnegative. Default is 0.05. Zero copies the input exactly after control validation. The filter does not choose an automatic stability cutoff or adapt this value.

### Number Of Iterations

The number of update iterations to perform. Default is 5.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
