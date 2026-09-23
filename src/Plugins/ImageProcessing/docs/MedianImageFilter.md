# Median Image Filter

Computes a pixel-wise median over a rectangular neighborhood.

## Group (Subgroup)

ImageProcessing (Neighborhood)

## Description

For every voxel, the output value is the median of the values in the box-shaped neighborhood defined by **Radius** (in X, Y, and Z), where the neighborhood is `(2*Rx+1) x (2*Ry+1) x (2*Rz+1)` voxels centered on that voxel. Neighbors that fall outside the image bounds are clamped to the nearest edge value (ZeroFluxNeumann boundary condition), matching the legacy ITK behavior. The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Median Image Filter.

## Algorithm

Resident input and output arrays are processed directly. For a 3D out-of-core image, the filter bulk-reads each output Z-plane with its clipped Z halo, evaluates neighborhoods from local memory, and bulk-writes the completed output plane. For a single-slice out-of-core image, a checked 64 MiB plan uses full-width row blocks containing the typed input Y halo and typed output. If a complete row cannot fit, the plan uses one-row X tiles with clipped X/Y halos. The budget also includes one complete neighborhood scratch array per bounded worker. Datastore access remains serial and outside the parallel voxel loop.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
