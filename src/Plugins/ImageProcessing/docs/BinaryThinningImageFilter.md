# Binary Thinning Image Filter

Computes a one-pixel-wide skeleton (edges) of a binary image.

## Group (Subgroup)

ImageProcessing (BinaryMathematicalMorphology)

## Description

Thins the foreground of a binary image down to a one-pixel-wide skeleton using a sequential (Gonzalez-Woods) thinning algorithm.

The input is assumed to be a binary image: any **non-zero** pixel is treated as foreground and any zero pixel as background. Foreground pixels are rescaled to 1 internally to simplify the computation. The output is binary as well, with background value 0 and foreground (skeleton) value 1.

The input array must be single-component (scalar) and may be **any integer type** (signed or unsigned). The output has the **same type as the input** (type-preserving). A 3D image is thinned **per z-slice independently** (the thinning stencil is 2D, so there is no coupling between slices), matching the legacy ITK behavior.

## Algorithm

The filter repeatedly performs four ordered thinning substeps. Each substep examines an unchanged binary source image, marks the foreground pixels that satisfy the Gonzalez-Woods neighborhood conditions, and removes all marked pixels only after the complete substep scan. Image borders use nearest-pixel clamping. This preserves endpoints, junctions, and loops according to the legacy ITK algorithm.

For an image with more than one Z slice, the filter processes independent slices in parallel batches. The active working-memory grant and available compute workers set the batch size. Each worker uses one typed slice, one byte-valued work slice, and packed deletion bits. The filter reads and writes each batch with contiguous bulk transfers. If an out-of-core image cannot fit one worker but can fit the minimum external state, the filter processes each Z slice with two disk-backed byte work images and bounded row blocks or X tiles. A smaller grant returns an error that reports the required minimum.

For a true two-dimensional out-of-core image (`Z=1`), a checked scheduler uses the active working-memory grant. A grant that holds the complete byte-valued work image and packed deletion bits keeps that state in memory. A smaller grant uses two disk-backed byte work images and alternates them after each substep. The external scans use bounded full-width row blocks or one-row X tiles with one-cell halos. If the grant cannot hold one external tile column and its metadata, the filter returns an error that reports the required minimum. All routes retain the same substep order and exact zero/one output values.

The four substeps inside one slice stay sequential. Independent Z slices can run in parallel. Run time depends on the image size and the number of thinning iterations. The algorithm corresponds to the 2D implementation described in:

Rafael C. Gonzalez and Richard E. Woods. Digital Image Processing. Addison Wesley, 491-494, (1993).

This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Binary Thinning Image Filter, and matches it exactly (byte-identical output values).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
