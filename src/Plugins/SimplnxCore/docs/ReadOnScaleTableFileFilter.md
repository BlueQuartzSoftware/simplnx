# Read OnScale Table File

## Group (Subgroup)

IO (Input)

## Description

This filter reads an OnScale/PZFLEX table (`.flxtbl`) file and creates a **Rectilinear Grid Geometry**. OnScale table files describe grid coordinates, material names, and the material index for each grid **Cell**.

The reader processes these sections:

- `hedr` and `info` identify the table format.
- `xcrd`, `ycrd`, and `zcrd` contain the floating-point bounds for each grid axis.
- `keypoints` contains PZFLEX keypoint counts. The reader ignores these values.
- `divisions` contains grid division counts. The reader ignores these values and calculates the dimensions from the bounds counts.
- `name` contains the material names.
- `matr` contains one signed 32-bit material index for each grid **Cell**. Each positive value is an index into the material names list. A value of `0` means that the cell has no material.

The sections can occur in any order before `matr`. The `matr` section must be last. Values use whitespace separators and can continue across any number of lines.

### Missing Axes and Units

An axis section can be absent. For an absent axis, the filter creates two bounds from *Fallback Origin* and *Fallback Spacing*: `{origin, origin + spacing}`. The resulting axis dimension is one **Cell**.

The created geometry uses bounds arrays named `X Bounds`, `Y Bounds`, and `Z Bounds`. The filter sets the geometry units to meters to preserve the DREAM.3D 6.6 default. The file does not store units.

### Created Data

The filter creates the selected **Rectilinear Grid Geometry**, its cell **Attribute Matrix**, and a scalar `int32` Feature Ids **Data Array**. The tuple dimensions are `{Z, Y, X}`.

The filter also creates the material **Attribute Matrix** and a `StringArray` that contains the names from the `name` section.

If the count in the `matr` header differs from the number of created cells, the filter reports a warning and reads the number of values required by the geometry. The filter reports an error if the file ends before it supplies all required material values. The filter ignores extra trailing material values and reports a warning.

### Example Input

```text
hedr 0
info 1
xcrd 3
0.00000000E+00 1.00000005E-03 2.00000009E-03
ycrd 2
0.00000000E+00 2.00000009E-03
zcrd 2
0.00000000E+00 3.00000003E-03
keypoints
2 2 2
divisions
2 1 1
name 2
pzt4t11 pzt4t12
matr 2
1 2
```

This input creates the following **Data Structure**:

```text
OnScale Volume (Rectilinear Grid Geometry, dimensions 2 x 1 x 1, meters)
├── X Bounds (float32, 3 tuples)
├── Y Bounds (float32, 2 tuples)
├── Z Bounds (float32, 2 tuples)
├── Cell Data (Attribute Matrix, shape 1 x 1 x 2)
│   └── FeatureIds (int32, 1 component)
└── Material Data (Attribute Matrix, 2 tuples)
    └── Material Names (StringArray: pzt4t11, pzt4t12)
```

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
