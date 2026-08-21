# Point Sample Edge Geometry

## Group (Subgroup)

Sampling (Geometry)

## Description

This filter walks along each edge of an **Edge Geometry** and places new sample points at a fixed spacing, building a new **Vertex Geometry** (a point cloud) of those samples.

An **Edge Geometry** is a set of straight line segments connecting pairs of vertices. In additive-manufacturing workflows each such segment is a **scan vector** — the path the energy source (e.g. a laser) travels in one pass. This filter converts those continuous segments into evenly spaced discrete points, which is useful for simulating or analyzing the path at a controlled resolution.

For every sample point the filter:

- Copies the **edge ID** of the segment the point lies on, so each point knows which original edge it came from.
- Optionally records the **cumulative sample distance** — the straight-line distance from the start of that edge (scan vector) to the sample point.

Any additional **Edge Data Arrays** the user selects are also copied onto the matching points of the new Vertex Geometry.

### Parameter Guidance

- **Sampling Spacing (mm)** — the distance between successive sample points along each edge, in **millimeters**. A smaller value yields more, more closely spaced points (finer resolution) and a larger output.
- **Calculate Cumulative Sample Distance** — when enabled, stores the start-of-edge distance described above for each sample point.

### Required Input Sources

- **Input Edge Geometry** -- an **Edge Geometry**, typically produced by [Slice Triangle Geometry](SliceTriangleGeometryFilter.md) or [Create AM Scan Paths](CreateAMScanPathsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
