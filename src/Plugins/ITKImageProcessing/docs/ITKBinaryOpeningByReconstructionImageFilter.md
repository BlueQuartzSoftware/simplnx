# ITK Binary Opening By Reconstruction Image Filter

Removes small foreground objects from a binary image while preserving the exact shape of the objects that remain.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

Like a standard morphological **opening**, this filter removes foreground objects that are smaller than the **structuring element** (the small probe shape — a ball, box, etc. — that is swept over the image). Unlike a plain opening, it then uses **morphological reconstruction** to restore the surviving objects to their original shape.

A plain opening erodes (shrinks) the image and then dilates (regrows) it. The erosion deletes small objects, but it also rounds off and shrinks the corners of the larger objects that survive. Opening *by reconstruction* avoids that side effect: after the erosion deletes the small objects, the reconstruction step regrows each surviving object back to exactly its original boundary instead of the rounded, dilated approximation. The net effect is "delete the small objects, leave the rest untouched."

The result is defined as `Opening(f) = ReconstructionByDilation(Erosion(f))`.

![Binary opening by reconstruction.](Images/ITKOpeningByReconstruction.png)

### Parameter Guidance

- **Foreground Value** — the pixel value that counts as object/foreground (everything else is background). For a segmented or thresholded image, this is the label of the region to process.
- **Background Value** — the value written where foreground is removed.
- **Kernel Radius** — the radius of the structuring element, **in pixels** (one value per axis). Objects smaller than this are removed.
- **Fully Connected** — controls neighbor connectivity during reconstruction (face-only versus face + edge + corner). Turn it on for thin, one-pixel-wide features.

#### Kernel Type

The *Kernel Type* parameter selects the structuring element shape:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

### Required Input Sources

Operates on a binary/segmented image — typically the output of a thresholding filter such as [ITK Binary Threshold Image Filter](ITKBinaryThresholdImageFilter.md) or [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md). Compare with the standard [ITK Binary Morphological Opening Image Filter](ITKBinaryMorphologicalOpeningImageFilter.md), which does not preserve object shape.

### Author

Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France. (Insight Journal: <https://www.insight-journal.org/browse/publication/176>)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
