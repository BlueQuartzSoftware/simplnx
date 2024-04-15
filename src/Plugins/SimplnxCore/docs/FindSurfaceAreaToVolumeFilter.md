# Find Surface Area to Volume & Sphericity

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the ratio of surface area to volume for each **Feature** in an **Image Geometry**. First, all the boundary **Cells** are found for each **Feature**. Next, the surface area for each face that is in contact with a different **Feature** is totalled. This number is divided by the volume of each **Feature**, calculated by taking the number of **Cells** of each **Feature** and multiplying by the volume of a **Cell**.

*Note:* The surface area will be the surface area of the **Cells** in contact with the neighboring **Feature** and will be influenced by the aliasing of the structure.  As a result, the surface area to volume will likely be over-estimated with respect to the *real* **Feature**.

This filter also optionally calculate the [Sphericity](https://en.wikipedia.org/wiki/Sphericity) of each feature.

![Equation for Sphericity used in the filter](Images/Sphericity_Equation.png)

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (01) SmallIN100 Morphological Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
