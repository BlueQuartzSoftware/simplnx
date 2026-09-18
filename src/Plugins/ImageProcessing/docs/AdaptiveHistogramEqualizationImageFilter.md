# Adaptive Histogram Equalization Image Filter

Adaptive contrast enhancement of an image (Stark 2000).

## Group (Subgroup)

ImageProcessing (ImageStatistics)

## Description

Enhance local contrast using adaptive histogram equalization. The filter first scans the whole image for its minimum and maximum gray value and uses that range to normalize every voxel to the interval `[-0.5, 0.5]`. Then, for each voxel, it accumulates a cumulative function over the box window centered on that voxel and writes the transformed result back. Neighbors that fall outside the image are skipped and the valid part of the window is over-weighted, exactly matching the boundary behavior of the legacy ITK filter. For a 2D image (Z size 1), out-of-plane neighbors are skipped and only the in-plane window contributes.

The behavior is controlled by two parameters, following Stark's generalization:

- **Alpha** interpolates between two classic behaviors. `Alpha = 0` (with `Beta = 0`) produces adaptive histogram equalization; `Alpha = 1` produces an unsharp mask.
- **Beta** controls the balance between the fully adaptive transform and simply passing the input through. As `Beta` approaches 0 the filter behaves as an unsharp mask; with `Alpha = 1` and `Beta = 1` the output matches the input exactly.
- **Radius** is the per-axis (X, Y, Z) radius of the box window; the window spans `2*radius + 1` voxels along each axis.

The input array must be single-component (scalar); the output element type matches the input element type, and all 10 scalar numeric types (integer and floating point) are supported. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Adaptive Histogram Equalization Image Filter. It uses a shared-budget resident route when the complete request is granted and bounded local buffers otherwise, producing bit-identical output between those routes. (The legacy ITK filter rejected out-of-core arrays.)

**Parity with the legacy filter:** results match the legacy ITK filter within a small tolerance rather than bit-for-bit. ITK accumulates the cumulative function over an `unordered_map` in hash order, so the floating-point summation order differs and integer outputs can differ by at most 1 at truncation boundaries.

**Constant-image deviation:** for a constant image (`max == min`) the normalization scale is zero. ITK computes `0/0 = NaN` in that case (a divide-by-zero bug); this filter intentionally passes the input through unchanged instead.

## Algorithm

For a 3D image with an out-of-core input or output, the filter first calculates the complete staged input/output plus its statistics, output-plane, lookup-table, and slab or rolling-ring buffers from the current dimensions, type, radius, and algorithm path. A complete shared reservation performs one full input read, runs the same transform against resident stores, and performs one full output write. The certified uint8 radius-10 path requests 163.5 MiB.

A partial grant or allocation failure uses the established bounded route. Its global gray-range prepass reads disk-backed inputs sequentially in byte-capped chunks of at most 64 MiB. A 3D transform processes one output Z plane at a time using its bounded halo slab; the specialized uint8 linear case uses a rolling plane ring. A single-slice transform with an out-of-core endpoint uses a checked 64 MiB plan. A fitting disk-backed output may use a fixed 48 MiB allocation and one final write while the input and local sums are streamed through the remaining allowance. Larger or mixed-endpoint layouts use full-width row blocks or one-row X tiles with clipped halos. The plan includes the integer lookup table and, for the uint8 linear case, local uint64 horizontal sums. Constant images use bounded copy chunks. All datastore transfers remain serial and outside parallel compute.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
