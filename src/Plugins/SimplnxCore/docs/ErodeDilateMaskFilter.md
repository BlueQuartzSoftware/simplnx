# Erode/Dilate Mask

## Group (Subgroup)

Processing (Cleanup)

## Description

If the mask is _dilated_, the **Filter** grows the _true_ regions by one **Cell** in an iterative sequence for a user
defined number of iterations. During the _dilate_ process, the classification of any **Cell** neighboring a _false_ *
*Cell** will be changed to _true_. If the mask is _eroded_, the **Filter** shrinks the _true_ regions by one **Cell** in
an iterative sequence for a user defined number of iterations. During the _erode_ process, _true_ **Cells** that have
at least one _false_ neighbor are changed to _false_. The **Filter** also offers the option(s) to
turn on/off the erosion or dilation in specific directions (X, Y or Z).

This filter will ONLY change the _Mask_ data array and not any of the other data arrays in the same attribute matrix.

The example images below were generated **AFTER** the execution of the filter and essentially any black pixel is where the **Mask** was false and any other color is where the **Mask** is true. (The colors are the typical IPF Colors using a <001> reference direction)

| Before Dilatation                      | After Dilation                       |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateMask_Before.png) | ![](Images/ErodeDilateMask_Dilate.png) |

| Before Erosion                      | After Erosion                       |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateMask_Before.png) | ![](Images/ErodeDilateMask_Erode.png) |

### Operation

The *Operation* parameter selects which morphological operation to apply to the mask:

- **Dilate [0]**: Expands the masked (true) regions by one **Cell** per iteration. Any **Cell** neighboring a false **Cell** is changed to true.
- **Erode [1]**: Shrinks the masked (true) regions by one **Cell** per iteration. True **Cells** that have at least one false neighbor are changed to false.

## Algorithm

This filter performs iterative morphological erosion or dilation directly on a boolean mask array within an ImageGeom grid. Unlike the Erode/Dilate Bad Data filter, this filter operates only on the mask and does not propagate changes to sibling data arrays.

### Processing Steps

For each iteration, the algorithm scans every voxel in the volume:

1. Identify false (unmasked) voxels.
2. For each false voxel, examine its 6 face-connected neighbors (optionally restricted to specific axes by the user).
3. **Dilation**: If any neighbor is true (masked), set the current false voxel to true. This grows the masked region outward.
4. **Erosion**: If any neighbor is true, set that neighbor to false. This shrinks the masked region inward.

A dual-buffer approach ensures that reads and writes do not interfere within a single iteration: the original mask state is read from one buffer while modifications are accumulated in a separate copy.

### Performance

This algorithm is optimized for both in-memory and out-of-core (OOC) data stores. When data resides on disk in chunked format, random voxel access can cause expensive chunk load/evict cycles. The implementation avoids this by:

- **Sequential Z-slice processing**: The volume is scanned one Z-slice at a time, aligning with typical chunk boundaries.
- **3-slice dual rolling window**: Two sets of three Z-slice buffers (read and write) are maintained in memory, allowing face-neighbor lookups and modification tracking without per-voxel store access.
- **Deferred bulk writes**: Modified slices are written back to the store in bulk after each Z-layer completes, minimizing I/O operations.
- **uint8 intermediary for bool**: Because std::vector<bool> uses bit-packing, uint8 buffers are used for the rolling window with conversion during I/O.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
