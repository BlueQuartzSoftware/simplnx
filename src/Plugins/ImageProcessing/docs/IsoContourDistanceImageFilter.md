# Iso Contour Distance Image Filter

Compute a narrow-band signed distance to a level-set iso-contour of the input image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Produces a **float32** image in which the value at each voxel adjacent to the **Level Set Value** iso-contour is the sub-pixel signed distance to that contour, computed by gradient interpolation. Voxels that are not adjacent to a crossing are set to **+Far Value** (where the input is above the level set) or **-Far Value** (below it), and voxels exactly on the level set are 0.

This is the classic initializer for level-set and fast-marching methods: it produces an accurate distance only in a one-voxel band around the contour, leaving the rest of the image at the saturated far value. It is a single neighborhood pass (not a full distance transform); when a full signed distance is required, prefer the Signed Maurer or Signed Danielsson Distance Map Image Filters.

The input array must be single-component and can use any numeric type. The output is always a **float32** image.

This implementation does not use ITK and supports out-of-core data. Integer and float64 inputs exactly match the legacy ITK filter. Float32-input results are within one unit in the last place (ULP).

### Level Set Value

The value of the level set (iso-contour) whose distance is computed. Default is 0.

### Far Value

The signed value written to voxels away from the iso-contour: +Far Value where the input is above the level set, -Far Value where it is below. Default is 10.

## Algorithm

The filter examines each voxel and its axis neighbors, detects level-set crossings, and keeps the smallest gradient-interpolated signed distance. Voxels away from a crossing retain the selected positive or negative **Far Value**.

### In-Core Path

For each voxel, the filter compares its class with the classes of its six face neighbors. Only voxels on the crossing surface run the gradient interpolation. The filter processes rows in parallel plane groups. Each worker reads the resident input span and writes a disjoint output row.

### Out-of-Core Path

For 3-D disk-backed images, the filter sizes plane slabs from the shared working-memory grant. Each slab has a radius-2 input halo and a disjoint output core.

The input buffer uses a ring of planes. It does not copy shared halo planes between slabs. Each slab uses at most two bulk reads and one bulk write. Every input plane is read once.

A complete grant processes the image with one read and one write. The standard minimum slab allocates five input planes and one output plane. It uses this allocation even when the grant is smaller. A thin volume limits the input depth to the volume depth.

The complete-grant size scales with the dimensions and input type. A `512 x 512 x 128` uint8 image uses 160 MiB. The same image with float64 input uses 384 MiB. Disk-backed 2-D images use bounded row blocks or X tiles. All requests share the application cache budget and remain subject to its aggregate 25% limit.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
