# Curvature Anisotropic Diffusion Image Filter

Denoise an image using the modified-curvature-diffusion-equation (MCDE) anisotropic diffusion equation, a PDE-based finite-difference filter.

## Group (Subgroup)

ImageProcessing (Smoothing)

## Description

Like the Gradient Anisotropic Diffusion Image Filter, anisotropic diffusion evolves the image under a non-linear diffusion equation whose conductance term is a function of the local gradient magnitude:

C(x) = exp(-(|grad(I)|/K)^2)

which reduces the strength of diffusion at edge pixels (where the gradient is large) relative to smooth regions. Unlike the gradient form, the per-axis update here is the "curvature" (modified-curvature-diffusion-equation) form: each axis's forward/backward difference is normalized by its own local gradient-magnitude approximation before being conductance-weighted, and the summed result is further modulated by an upwind gradient-magnitude term. This tends to diffuse more smoothly along edges (less staircasing) than the plain gradient form. The calibration constant K is recomputed every **Conductance Scaling Update Interval** iterations (and always at the first iteration) from a global reduction: the average squared gradient magnitude over the entire image. That reduction is computed as a single deterministic, ordered accumulation, so in-core and out-of-core runs are byte-identical.

The evolution is computed with a finite-difference solver: at every iteration, an update is computed at each voxel from a frozen (Jacobi) neighborhood of the current image using a zero-flux Neumann (edge-clamp) boundary condition, and then `Output = Output + TimeStep * Update` is applied everywhere. The per-axis image spacing is used when computing derivatives, so the result depends on the Image Geometry's spacing.

The input array must be single-component (scalar) and a **floating-point** type (float32 or float64) -- integer input is rejected. The output has the **same type as the input** (type-preserving). This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Curvature Anisotropic Diffusion Image Filter, and matches it (exactly, or within a documented few-ULP floating-point tolerance -- see the filter's unit test for the parity evidence).

The timestep should be small enough to ensure numerical stability. In general, keep the timestep below `(min filtered-axis spacing) / 2^(N+1)`, where `N` is the number of image dimensions (2 for a 2D image, 3 for a 3D image); a Time Step above that bound produces a preflight **warning** (not a hard rejection, matching ITK's own non-fatal check), so the pipeline still runs but the result may be unstable.

## Algorithm

For resident arrays, the solver keeps its intermediate and update images resident, computes each frozen-neighborhood update through contiguous spans, and applies updates in parallel without staging copies. Interior voxels use direct offsets, and each anisotropic update loads its 19 distinct radius-one center/face/edge samples once for reuse. The conductance reduction remains a deterministic ordered pass, and calculations retain the input floating-point precision used by the legacy implementation.

For a 3D image with an out-of-core input or output, the solver first asks the shared cache budget for the complete input, output, intermediate, update, and conversion-plane state. A complete grant reads the input once, runs the same resident Jacobi calculation and ordered conductance reduction, and writes the output once. A partial grant or allocation failure uses the bounded fallback: a three-plane rolling neighborhood plus two raw temporary-record working stores, with initial/final conversions and updates transferred one Z plane at a time. The ordered conductance reduction also uses a bounded rolling window. True 2D retains its checked 64 MiB row-block and X-tile plan.

### Time Step

The time step to be used for each iteration. Default is 0.0625.

### Conductance Parameter

The conductance parameter controls the sensitivity of the conductance term in the basic anisotropic diffusion equation. Default is 3.0.

### Conductance Scaling Update Interval

The interval (in iterations) between conductance recalibrations. Default is 1.

### Number Of Iterations

Specifies the number of iterations (time-step updates) that the solver will perform to produce a solution image. Default is 5.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
