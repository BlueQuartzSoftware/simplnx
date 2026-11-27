# HDF5 Stack Reader

## Group (Subgroup)

Core (IO/Read)

## Description

This filter reads a numbered series of 2D HDF5 datasets (a "stack" of slices) and assembles them into image data in DREAM3D-NX. Each HDF5 dataset is treated as one XY slice, and the slices are stacked along Z in the order they are listed. HDF5 (Hierarchical Data Format version 5) is a binary file format that organizes named datasets inside a tree of groups, much like files inside folders on a disk.

The slices can come from one of two places, chosen with **Input Mode**:

- **Single File (Multiple Datasets)**: every slice is a separate dataset inside one HDF5 file, for example `Slices/slice_000`, `Slices/slice_001`, `Slices/slice_002`, ...
- **Multiple Files (Same Dataset)**: every slice lives in its own HDF5 file, and each file stores its slice at the same dataset path, for example `slice_00.h5`, `slice_01.h5`, `slice_02.h5`, ..., each containing `Data/Image`.

Only the parameter for the selected input mode is used and validated.

### Building the List of Slices

**Single File (Multiple Datasets).** The dataset paths are built from a prefix, a zero-padded index and a suffix:

```
<Path Prefix><index, zero-padded to Padding Digits><Path Suffix>
```

The index runs from **Start Index** to **End Index**, inclusive. For example, a prefix of `Slices/slice_`, an empty suffix, 3 padding digits and indices 0 to 2 produce `Slices/slice_000`, `Slices/slice_001` and `Slices/slice_002`. A suffix makes it possible to reach a dataset inside a numbered group: a prefix of `Stack/Z`, a suffix of `/Image` and 2 padding digits produce `Stack/Z00/Image`, `Stack/Z01/Image`, ...

**Multiple Files (Same Dataset).** The file paths are generated the same way as in other file-stack readers, such as **Read Images [3D Stack]**:

```
<Input Directory>/<File Prefix><index, zero-padded to Padding Digits><File Suffix><File Extension>
```

The index starts at **Start Index**, advances by **Increment Index**, and stops at **End Index**. **Ordering** chooses whether the stack is built from the lowest index to the highest (Low To High) or from the highest to the lowest (High To Low). This decides which file ends up at Z = 0. The same **Dataset Path** is read from every file.

### How Each Dataset Is Interpreted

Each dataset must have at least 2 dimensions. The HDF5 dimensions are read in their stored (row-major) order:

| HDF5 dimension | Meaning |
|----------------|---------|
| 1st | Y (number of rows in the slice) |
| 2nd | X (number of columns in the slice) |
| 3rd and later (optional) | Component shape of each pixel |

For example, a dataset with dimensions `512 x 640` is read as a single-component slice 640 pixels wide (X) and 512 pixels tall (Y). A dataset with dimensions `512 x 640 x 3` is read as the same slice with 3 components per pixel, such as an RGB image.

**Every slice must have exactly the same dimensions**, including the component dimensions. The filter checks each slice against the first one during preflight and reports an error naming the first slice that does not match.

### Output Numeric Type

The data is stored in the array using the chosen **Output Numeric Type**, whatever type the HDF5 datasets store. Values are converted with a plain C++ numeric cast, so:

- converting floating-point data to an integer type drops the fractional part, and
- values outside the range of the output type are not clamped and may wrap or overflow.

Pick an output type that can hold the full range of the source data. Supported HDF5 source types are 8, 16, 32 and 64-bit signed and unsigned integers, 32-bit floats and 64-bit floats.

### Output Layout

The created geometries always use an origin of (0, 0, 0) and a spacing of (1, 1, 1).

**Create Montage off (default).** One **Image Geometry** is created with dimensions X x Y x (number of slices). It holds one **Cell Attribute Matrix** containing a single **Data Array** with tuple dimensions (number of slices, Y, X) and the component dimensions read from the datasets. Slice *i* of the stack becomes Z index *i*.

**Create Montage on.** A **Grid Montage** is created with a tile layout of 1 x 1 x (number of slices). Each slice becomes its own **Image Geometry** of X x Y x 1, stored inside the montage and named `<Created Geometry Name>_<i>`, where *i* is the slice's position in the stack starting at 0. Each of those geometries holds its own Cell Attribute Matrix and Data Array, and is placed in the montage at tile position (0, 0, *i*).

### Worked Example

An HDF5 file `stack.h5` contains three datasets, `Slices/slice_000`, `Slices/slice_001` and `Slices/slice_002`. Each is a `uint8` dataset with dimensions `4 x 5 x 3`.

With **Input Mode** set to *Single File (Multiple Datasets)*, a path prefix of `Slices/slice_`, 3 padding digits, indices 0 to 2, **Output Numeric Type** set to `float32` and **Create Montage** off, the filter creates:

- an Image Geometry with dimensions 5 x 4 x 3 (X x Y x Z), and
- a `float32` Data Array with tuple dimensions (3, 4, 5) and component dimensions (3), where Z = 0 holds `slice_000`, Z = 1 holds `slice_001` and Z = 2 holds `slice_002`.

## Required Input Sources

None. This filter reads directly from one or more `.h5`/`.hdf5` files on disk and creates all of its own output objects.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
