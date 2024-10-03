# Convert Orientation Representation

## Group (Subgroup)

Orientation Analysis (Conversion)

## Description

This **Filter** generates a new orientation representation (see Data Layout Table below) 
for each **Element**, given the *Input Orientation Representation* for the **Element**.
The following table lists the various orientation representations that are supported. DREAM3D
is capable of converting between any representation with some caveats. The Input and
Output Orientation Type are represented as a zero based index into the combo box widget.

### Orientation Representation Enumeration Index

| Enumeration Index | Orientation Representation |
|-------------------|---------------------------|
| 0                 | EulerAngles                |
| 1                 | Orientation Matrix         |
| 2                 | Quaternions                |
| 3                 | Axis Angle                 |
| 4                 | Rodrigues Vectors          |
| 5                 | Homochoric                 |
| 6                 | Cubochoric                 |
| 7                 | StereoGraphic              |

### Data Layout

| Orientation Representation | No. of Components  | Data Layout |
|----------------------------|---------------------|-------------|
| EulerAngles                | 3 | phi1, Phi, phi2 |
| Orientation Matrix         | 9 | Row Major Format |
| Quaternions                | 4 | ( \[x, y, z\], w ) |
| Axis Angle                 | 4 | ( \[x, y, z\], Angle) |
| Rodrigues Vectors          | 4 | ( \[x, y, z\], w )* |
| Homochoric                 | 3 | \[x, y, z\] |
| Cubochoric                 | 3 | \[x, y, z\] |
| StereoGraphic              | 3 | \[x, y, z\] |

- * Rodrigues Vector: DREAM3D stores the Rodrigues vector as a normalized vector with the lenth as the last component. In order to convert from the 4 component to the typical 3 component Rodrigues
Vector the user can multiply each vector part by the scalar part.

### Data Range

The valid range for Euler angles is (Degrees):

    + phi-1: 0 to 360
    + Phi : 0 to 180
    + phi-2: 0 to 360

Rodrigues Vector: Length must be positive and the vector must be normalized.
Homochoric Vector: Sum of Squares must = 1.0
Quaternion: Scalar part must be positive and have unit norm
Axis Angle: Angle must be in the range of 0-Pi.
Stereographic Vector: Must be unit norm
Orientation Matrix: Determinant must be +1.0

### Data Conversion Notes

If the angles fall outside of this range the **original** Euler Input data **WILL BE CHANGED** to ensure they are within this range.

## Precision Notes

While every effort has been made to ensure the correctness of each transformation algorithm, certain situations may arise where the initial precision of the input data is not large enough for the algorithm to calculate an answer that is intuitive. The user should be acutely aware of their input data and if their data may cause these situations to occur. Combinations of Euler angles close to 0, 180 and 360 can cause these issues to be hit. For instance an Euler angle of [180, 56, 360] is symmetrically the same as [180, 56, 0] and due to calculation errors and round off errors converting that Euler angle between representations may not give the numerical answer the user was anticipating but will give a symmetrically equivalent angle.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ 02_Adaptive Alignment - Misorientation - Zero Shifts
+ (10) SmallIN100 Full Reconstruction
+ INL Export
+ 03_Adaptive Alignment - Mutual Information - SEM Images
+ MassifPipeline
+ InsertTransformationPhase
+ 04_Steiner Compact
+ (03) SmallIN100 Alignment

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
