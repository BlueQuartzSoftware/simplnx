# Min Max Curvature Flow Image Filter

Denoise an image using min/max curvature flow, a PDE-based finite-difference filter.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is evolved using a curvature-based speed function:

I_t = F_minmax * |grad(I)|

where F_minmax = max(kappa, 0) if the average image value over a ball-shaped stencil of radius **Stencil Radius** around a point is below a threshold, and min(kappa, 0) otherwise; kappa is the curvature of the iso-brightness contour passing through the point. The threshold is the average intensity in the direction perpendicular to the local gradient, sampled at the extrema of the stencil neighborhood.

Switching between max(kappa,0) and min(kappa,0) turns the level-set motion on or off depending on the scale of the noise to be removed: the choice of **Stencil Radius** governs that scale, independent of the timestep used by the underlying curvature-flow update.

The evolution is computed with a finite-difference solver: at every iteration, an update is computed at each voxel from a frozen (Jacobi) neighborhood of the current image using a zero-flux Neumann (edge-clamp) boundary condition, and then `Output = Output + TimeStep * Update` is applied everywhere. The per-axis image spacing is used when computing derivatives, so the result depends on the Image Geometry's spacing.

The input array must be single-component (scalar) and a **floating-point** type (float32 or float64) -- integer input is rejected. The output has the **same type as the input** (type-preserving). This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Min Max Curvature Flow Image Filter, and matches it exactly (bit-exact for float32/float64, verified against live ITK).

The timestep should be small enough to ensure numerical stability. Stability is guaranteed when the timestep meets the CFL (Courant-Friedrichs-Levy) condition; broadly speaking, this condition ensures that each contour does not move more than one grid position per timestep. The timestep typically must be manually tuned to the application.

## Algorithm

For resident arrays, the solver keeps its intermediate and update images resident, computes each frozen-neighborhood update through contiguous spans, and applies updates in parallel without staging copies. For stencil radii up to 16, the ordered sphere-offset list and its normalization factor are built once per filter execution and reused at every voxel. Larger radii use the original allocation-free ordered loops so precomputation cannot introduce unbounded `O(radius^3)` memory.

For a 3D image with an out-of-core input or output, the solver first asks the shared cache budget for the complete input, output, intermediate, update, and conversion-plane state. A complete grant reads the input once, runs the same resident Jacobi calculation, and writes the output once. A partial grant or allocation failure uses the bounded fallback: the complete `(2 * Stencil Radius + 1)`-plane rolling neighborhood plus two raw temporary-record working stores, with initial/final conversions and updates transferred one Z plane at a time. A large stencil radius increases the fallback window. True 2D retains its checked 64 MiB row-block and X-tile plan.

### Time Step

The timestep between iteration updates. Default is 0.05.

### Number Of Iterations

The number of update iterations to perform. Default is 5.

### Stencil Radius

Set/Get the stencil radius. Default is 2.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
