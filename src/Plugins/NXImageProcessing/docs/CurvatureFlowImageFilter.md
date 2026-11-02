# Curvature Flow Image Filter

Denoise an image using curvature-driven flow, a PDE-based finite-difference filter.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is evolved using a curvature-based speed function:

I_t = kappa * |grad(I)|

where kappa is the curvature of the iso-brightness contour passing through each voxel. The advantage of this approach is that sharp boundaries are preserved while smoothing occurs within a region; continuous application of this scheme will eventually remove all information as each contour shrinks to zero and disappears.

The evolution is computed with a multi-threaded finite-difference solver: at every iteration, an update is computed at each voxel from a frozen (Jacobi) neighborhood of the current image using a zero-flux Neumann (edge-clamp) boundary condition, and then `Output = Output + TimeStep * Update` is applied everywhere. The per-axis image spacing is used when computing derivatives, so the result depends on the Image Geometry's spacing.

The input array must be single-component (scalar) and may be **any** scalar type (signed/unsigned integer or floating point). The output has the **same type as the input** (type-preserving); for an integer input, each iteration's update is truncated to the input type exactly as the legacy filter does. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Curvature Flow Image Filter, and matches it exactly (bit-exact for integer types; exact/near-exact for floating point).

The timestep should be small enough to ensure numerical stability. Stability is guaranteed when the timestep meets the CFL (Courant-Friedrichs-Levy) condition; broadly speaking, this condition ensures that each contour does not move more than one grid position per timestep. The timestep typically must be manually tuned to the application.

## Algorithm

For resident arrays, the solver keeps its intermediate and update images resident, computes each frozen-neighborhood update through contiguous spans, and applies updates in parallel without staging copies.

For a 3D image with an out-of-core input or output, the solver first asks the shared cache budget for the complete input, output, intermediate, update, and conversion-plane state. A complete grant reads the input once, runs the same resident Jacobi calculation, and writes the output once. A partial grant or allocation failure uses the bounded fallback: a three-plane rolling neighborhood plus two raw temporary-record working stores, with initial/final conversions and updates transferred one Z plane at a time. For true 2D, the existing checked row-block and X-tile plan remains bounded to 64 MiB.

### Time Step

The timestep between iteration updates. Default is 0.05.

### Number Of Iterations

The number of update iterations to perform. Default is 5.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
