# Read Peregrine HDF5 File

## Group (Subgroup)
Core (IO/Read)

## Description
The `ReadPeregrineHDF5File` filter is designed to read an HDF5 file that is in a format produced by Oak Ridge National Laboratory's Peregrine. The filter handles a multitude of datasets embedded within Peregrine's HDF5 container format, allowing for selective reading based on user input.

## Parameters

### Input Parameters
- **`Input Peregrine HDF5 File`** (string): Specifies the complete file path to the target HDF5 file.

## Slice Data Parameters
- **`Slice Data Geometry`**: Path to the Slice Data image geometry to be created.
- **`Slice Data Cell Attribute Matrix Name`**: Name of the Slice Data cell attribute matrix to be created.
- **`Read Segmentation Results`** (boolean): Toggles the reading of segmentation results stored in the HDF5 file.
- **`Segmentation Results`** (string): Comma-delimited string that specifies which segmentation results datasets should be read.
- **`Read Camera #1 Data`** (boolean): Toggles the reading of camera #1 data stored in the HDF5 file.
- **`Camera #1 Data HDF5 Parent Path`**: The path to the parent group of the camera #1 datasets in the HDF5 file.
- **`Camera #1 Data Datasets`**: Comma-delimited string that specifies which camera #1 data datasets should be read.
- **`Read Camera #2 Data`** (boolean): Toggles the reading of camera #2 data stored in the HDF5 file.
- **`Camera #2 Data HDF5 Parent Path`**: The path to the parent group of the camera #2 datasets in the HDF5 file.
- **`Camera #2 Data Datasets`**: Comma-delimited string that specifies which camera #2 data datasets should be read.
- **`Read Camera #3 Data`** (boolean): Toggles the reading of camera #3 data stored in the HDF5 file.
- **`Camera #3 Data HDF5 Parent Path`**: The path to the parent group of the camera #3 datasets in the HDF5 file.
- **`Camera #3 Data Datasets`**: Comma-delimited string that specifies which camera #3 data datasets should be read.
- **`Read Part Ids`** (boolean): Determines whether part ids data should be read.
- **`Part Ids Array Name`**: Name of the part ids array to be created.
- **`Read Sample Ids`** (boolean): Determines whether sample ids data should be read.
- **`Sample Ids Array Name`**: Name of the sample ids array to be created.
- **`Enable Slices Subvolume`** (boolean): Enables or disables the reading of a specific subvolume of slices data within the HDF5 file. If set to True, additional parameters for specifying the subvolume bounds (`Slices Subvolume X Bounds`, `Slices Subvolume Y Bounds`, `Slices Subvolume Z Bounds`) are required.
- **`Slices Subvolume X Bounds`** (tuple of integers): Defines the minimum and maximum x-coordinates of the slices subvolume to be read.
- **`Slices Subvolume Y Bounds`** (tuple of integers): Defines the minimum and maximum y-coordinates of the slices subvolume to be read.
- **`Slices Subvolume Z Bounds`** (tuple of integers): Defines the minimum and maximum z-coordinates of the slices subvolume to be read.

## Registered Data Parameters
- **`Registered Data Geometry`**: Path to the Registered Data image geometry to be created.
- **`Registered Data Cell Attribute Matrix Name`**: Name of the Registered Data cell attribute matrix to be created.
- **`Read Anomaly Detection`** (boolean): Determines whether anomaly detection data should be read.
- **`Anomaly Detection Array Name`**: Name of the Anomaly Detection array to be created.
- **`Read X-Ray CT`** (boolean): Determines whether X-ray computed tomography data should be read.
- **`X-Ray CT Array Name`**: Name of the X-Ray CT array to be created.
- **`Enable Registered Data Subvolume`** (boolean): Enables or disables the reading of a specific subvolume of registered data within the HDF5 file. If set to True, additional parameters for specifying the subvolume bounds (`Registered Data Subvolume X Bounds`, `Registered Data Subvolume Y Bounds`, `Registered Data Subvolume Z Bounds`) are required.
- **`Registered Data Subvolume X Bounds`** (tuple of integers): Defines the minimum and maximum x-coordinates of the registered data subvolume to be read.
- **`Registered Data Subvolume Y Bounds`** (tuple of integers): Defines the minimum and maximum y-coordinates of the registered data subvolume to be read.
- **`Registered Data Subvolume Z Bounds`** (tuple of integers): Defines the minimum and maximum z-coordinates of the registered data subvolume to be read.

### Scan Data Parameters
- **`Read Scan Datasets`** (boolean): Toggles the reading of the scan datasets stored in the HDF5 file.
- **`Scan Data Geometry`**: Path to the Scan Data edge geometry to be created.
- **`Scan Data Edge Attribute Matrix Name`**: Name of the Scan Data edge attribute matrix to be created.
- **`Scan Data Vertex Attribute Matrix Name`**: Name of the Scan Data vertex attribute matrix to be created.
- **`Scan Data Vertex List Array Name`**: Name of the Scan Data vertex list array to be created.
- **`Scan Data Edge List Array Name`**: Name of the Scan Data edge list array to be created.
- **`Scan Data Time of Travel Array Name`**: Name of the Scan Data Time of Travel array to be created.
- **`Enable Scan Data Subvolume`** (boolean): Enables or disables the reading of specific slices of scan data within the HDF5 file. If set to True, an additional parameter for specifying the slice bounds (`Scan Data Slice Bounds`) is required.
- **`Scan Data Slice Bounds`** (tuple of integers): Defines the minimum and maximum slices of the scan data to be read.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
