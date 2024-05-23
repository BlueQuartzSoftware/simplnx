# Write GBCD Triangles File

## Group (Subgroup)

IO (Output)

## Description

This **Filter** writes relevant information about the Grain Boundary Character Distribution (GBCD) on an existing set of triangles.  The information written includes the inward and outward Euler angles, normals, and areas for each triangle.  The file format was originally defined by Prof. Greg Rohrer (CMU).

## Example Output

    # Triangles Produced from DREAM3D version 5.2
    # Column 1-3:    right hand average orientation (phi1, PHI, phi2 in RADIANS)
    # Column 4-6:    left hand average orientation (phi1, PHI, phi2 in RADIANS)
    # Column 7-9:    triangle normal
    # Column 8:      surface area
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.0000 0.8829 -0.4696 0.0240
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.4532 0.3203 -0.8319 0.0211
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 1.0000 0.0000 0.0000 0.0312
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.9939 0.0780 0.0780 0.0315
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.0000 0.9985 0.0551 0.0332
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.5074 0.7638 -0.3989 0.0260
    0.7993 0.6738 3.5200 0.2662 0.6970 4.4347 0.0000 -0.7792 0.6268 0.0182
    0.7993 0.6738 3.5200 0.2662 0.6970 4.4347 -0.5737 -0.0995 0.8130 0.0221
    3.4109 0.6178 1.0586 0.2662 0.6970 4.4347 1.0000 0.0000 0.0000 0.0312
    3.4109 0.6178 1.0586 0.2662 0.6970 4.4347 0.9822 0.1328 0.1328 0.0256
       ..

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
