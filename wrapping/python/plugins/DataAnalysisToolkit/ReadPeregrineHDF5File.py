from typing import List, Tuple
import numpy as np
import simplnx as nx
import h5py
from .common.Result import Result, make_error_result

class ReadPeregrineHDF5File:
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  # Parameter Keys
  INPUT_FILE_PATH_KEY = 'input_file_path'
  READ_CAMERA_DATA_SUBVOLUME_KEY = 'read_camera_data_subvolume'
  CAMERA_DATA_SUBVOLUME_MINMAX_X_KEY = 'camera_data_subvolume_minmax_x'
  CAMERA_DATA_SUBVOLUME_MINMAX_Y_KEY = 'camera_data_subvolume_minmax_y'
  CAMERA_DATA_SUBVOLUME_MINMAX_Z_KEY = 'camera_data_subvolume_minmax_z'
  READ_CAMERA_DATA_KEY = 'read_camera_data'
  READ_ANOMALY_DETECTION_KEY = 'read_anomaly_detection'
  READ_X_RAY_CT_KEY = 'read_x_ray_ct'
  SLICE_DATA_KEY = 'slice_data'
  SLICE_DATA_CELL_ATTR_MAT_KEY = 'slice_data_cell_attr_mat'
  CAMERA_DATA_0_ARRAY_NAME_KEY = 'camera_data_0_array_name'
  CAMERA_DATA_1_ARRAY_NAME_KEY = 'camera_data_1_array_name'
  REGISTERED_DATA_KEY = 'registered_data'
  REGISTERED_DATA_CELL_ATTR_MAT_KEY = 'registered_data_cell_attr_mat'
  ANOMALY_DETECTION_ARRAY_NAME_KEY = 'anomaly_detection_array_name'
  XRAY_CT_ARRAY_NAME_KEY = 'xray_ct_array_name'
  READ_REGISTERED_DATA_SUBVOLUME_KEY = 'read_registered_data_subvolume'
  REGISTERED_DATA_SUBVOLUME_MINMAX_X_KEY = 'registered_data_subvolume_minmax_x'
  REGISTERED_DATA_SUBVOLUME_MINMAX_Y_KEY = 'registered_data_subvolume_minmax_y'
  REGISTERED_DATA_SUBVOLUME_MINMAX_Z_KEY = 'registered_data_subvolume_minmax_z'

  # HDF5 Dataset Paths
  CAMERA_DATA_0_H5_PATH = '/slices/camera_data/visible/0'
  CAMERA_DATA_1_H5_PATH = '/slices/camera_data/visible/1'
  REGISTERED_ANOMALY_DETECTION_H5_PATH = '/slices/registered_data/anomaly_detection'
  REGISTERED_XRAY_CT_H5_PATH = '/slices/registered_data/x-ray_ct'

  # HDF5 Attribute Keys
  X_REAL_DIMENSION_PATH = 'printer/x_real_dimension'
  Y_REAL_DIMENSION_PATH = 'printer/y_real_dimension'
  X_CAMERA_DIMENSION_PATH = 'printer/x_camera_dimension'
  Y_CAMERA_DIMENSION_PATH = 'printer/y_camera_dimension'
  LAYER_THICKNESS_PATH = 'material/layer_thickness'
  X_UNITS_PATH = 'printer/x_camera_dimension/units'
  Y_UNITS_PATH = 'printer/y_camera_dimension/units'

  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('7a936f3f-9364-4993-9a2c-89207010a1f5')

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'ReadPeregrineHDF5File'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'ReadPeregrineHDF5File'

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  ReadPeregrineHDF5File
    """
    return ReadPeregrineHDF5File()

# -----------------------------------------------------------------------------
# These methods CAN (and probably should) be updated. For instance, the 
# human_name() is what users of the filter will see in the DREAM3D-NX GUI. You
# might want to consider putting spaces between workd, using proper capitalization
# and putting "(Python)" at the end of the name (or beginning if you want the 
# filter list to group your filters togther)
# -----------------------------------------------------------------------------
  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return 'Read Peregrine Camera and Registered Data (Python)'
 
  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python', 'ReadPeregrineHDF5File', 'peregrine', 'hdf5', 'read', 'import']
   
  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = nx.Parameters()

    params.insert(nx.Parameters.Separator("Input Parameters"))
    params.insert(nx.FileSystemPathParameter(ReadPeregrineHDF5File.INPUT_FILE_PATH_KEY, 'Input Peregrine HDF5 File', 'The input Peregrine HDF5 file that will be read.', '', {'.hdf5', '.h5'}, nx.FileSystemPathParameter.PathType.InputFile, False))
    params.insert_linkable_parameter(nx.BoolParameter(ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY, 'Read Camera Data', 'Specifies whether or not to read the camera data from the input file.', True))
    params.insert_linkable_parameter(nx.BoolParameter(ReadPeregrineHDF5File.READ_ANOMALY_DETECTION_KEY, 'Read Anomaly Detection', 'Specifies whether or not to read the anomaly detection (part of the registered data) from the input file.', True))
    params.insert_linkable_parameter(nx.BoolParameter(ReadPeregrineHDF5File.READ_X_RAY_CT_KEY, 'Read X-Ray CT', 'Specifies whether or not to read the x-ray CT (part of the registered data) from the input file.', True))
    params.insert_linkable_parameter(nx.BoolParameter(ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY, 'Read Camera Data Subvolume', 'Specifies whether or not to read a subvolume of the camera data from the input file.', False))
    params.insert(nx.VectorUInt64Parameter(ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_X_KEY, 'Camera Data Subvolume Min/Max X Dimension', 'The min/max indices of the X dimension for the Camera Data subvolume.', [0, 99], ['X Min', 'X Max']))
    params.insert(nx.VectorUInt64Parameter(ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Y_KEY, 'Camera Data Subvolume Min/Max Y Dimension', 'The min/max indices of the Y dimension for the Camera Data subvolume.', [0, 99], ['Y Min', 'Y Max']))
    params.insert(nx.VectorUInt64Parameter(ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Z_KEY, 'Camera Data Subvolume Min/Max Z Dimension', 'The min/max indices of the Z dimension for the Camera Data subvolume.', [0, 99], ['Z Min', 'Z Max']))
    params.insert_linkable_parameter(nx.BoolParameter(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, 'Read Registered Data Subvolume', 'Specifies whether or not to read a subvolume of the registered data from the input file.', False))
    params.insert(nx.VectorUInt64Parameter(ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_X_KEY, 'Registered Data Subvolume Min/Max X Dimension', 'The min/max indices of the X dimension for the Registered Data subvolume.', [0, 99], ['X Min', 'X Max']))
    params.insert(nx.VectorUInt64Parameter(ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Y_KEY, 'Registered Data Subvolume Min/Max Y Dimension', 'The min/max indices of the Y dimension for the Registered Data subvolume.', [0, 99], ['Y Min', 'Y Max']))
    params.insert(nx.VectorUInt64Parameter(ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Z_KEY, 'Registered Data Subvolume Min/Max Z Dimension', 'The min/max indices of the Z dimension for the Registered Data subvolume.', [0, 99], ['Z Min', 'Z Max']))

    params.insert(nx.Parameters.Separator("Created Data Objects"))
    params.insert(nx.DataGroupCreationParameter(ReadPeregrineHDF5File.SLICE_DATA_KEY, 'Slice Data Geometry', 'The path to the newly created Slice Data image geometry', nx.DataPath(['Slice Data'])))
    params.insert(nx.DataObjectNameParameter(ReadPeregrineHDF5File.SLICE_DATA_CELL_ATTR_MAT_KEY, 'Slice Data Cell Attribute Matrix Name', 'The name of the Slice Data cell attribute matrix', 'Cell Data')) # ImageGeom::k_CellDataName
    params.insert(nx.DataObjectNameParameter(ReadPeregrineHDF5File.CAMERA_DATA_0_ARRAY_NAME_KEY, 'Camera Data 0 Array Name', 'The name of the camera data 0 array.', 'Camera Data 0'))
    params.insert(nx.DataObjectNameParameter(ReadPeregrineHDF5File.CAMERA_DATA_1_ARRAY_NAME_KEY, 'Camera Data 1 Array Name', 'The name of the camera data 1 array.', 'Camera Data 1'))
    params.insert(nx.DataGroupCreationParameter(ReadPeregrineHDF5File.REGISTERED_DATA_KEY, 'Registered Data Geometry', 'The path to the newly created Registered Data image geometry', nx.DataPath(['Registered Data'])))
    params.insert(nx.DataObjectNameParameter(ReadPeregrineHDF5File.REGISTERED_DATA_CELL_ATTR_MAT_KEY, 'Registered Data Cell Attribute Matrix Name', 'The name of the Registered Data cell attribute matrix', 'Cell Data')) # ImageGeom::k_CellDataName
    params.insert(nx.DataObjectNameParameter(ReadPeregrineHDF5File.ANOMALY_DETECTION_ARRAY_NAME_KEY, 'Anomaly Detection Array Name', 'The name of the Anomaly Detection array.', 'Anomaly Detection'))
    params.insert(nx.DataObjectNameParameter(ReadPeregrineHDF5File.XRAY_CT_ARRAY_NAME_KEY, 'X-Ray CT Array Name', 'The name of the X-Ray CT array.', 'X-Ray CT'))

    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_X_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Y_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Z_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_X_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Y_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Z_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY, ReadPeregrineHDF5File.CAMERA_DATA_0_ARRAY_NAME_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY, ReadPeregrineHDF5File.CAMERA_DATA_1_ARRAY_NAME_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_ANOMALY_DETECTION_KEY, ReadPeregrineHDF5File.ANOMALY_DETECTION_ARRAY_NAME_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_X_RAY_CT_KEY, ReadPeregrineHDF5File.XRAY_CT_ARRAY_NAME_KEY, True)

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """
    input_file_path = args[ReadPeregrineHDF5File.INPUT_FILE_PATH_KEY]

    actions = nx.OutputActions()
    preflight_updated_values: List[nx.IFilter.PreflightValue] = []

    try:
      h5_file_reader = h5py.File(str(input_file_path), "r")
    except OSError as e:
      return nx.IFilter.PreflightResult(errors=[nx.Error(-2011, f"Error opening file '{str(input_file_path)}': {e}")])
    except Exception as e:
       return nx.IFilter.PreflightResult(errors=[nx.Error(-2012, f"Error opening file '{str(input_file_path)}': {e}")])

    spacing_result: Result = self._calculate_spacing(h5_file_reader)
    if not spacing_result.valid():
      return nx.IFilter.PreflightResult(errors=spacing_result.errors)

    origin: List[float] = [0.0, 0.0, 0.0]
    spacing: List[float] = spacing_result.value

    result: Result = self._preflight_slice_datasets(h5_file_reader, origin, spacing, args, actions, preflight_updated_values)
    if result.invalid():
      return nx.IFilter.PreflightResult(errors=result.errors)

    result: Result = self._preflight_registered_datasets(h5_file_reader, origin, spacing, args, actions, preflight_updated_values)
    if result.invalid():
      return nx.IFilter.PreflightResult(errors=result.errors)

    return nx.IFilter.PreflightResult(output_actions=actions, preflight_values=preflight_updated_values)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.Result:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """
    input_file_path = args[ReadPeregrineHDF5File.INPUT_FILE_PATH_KEY]

    try:
      h5_file_reader = h5py.File(str(input_file_path), "r")
    except OSError as e:
      return nx.Result(errors=[nx.Error(-2011, f"Error opening file '{str(input_file_path)}': {e}")])
    except Exception as e:
       return nx.Result(errors=[nx.Error(-2012, f"Error opening file '{str(input_file_path)}': {e}")])

    result: Result = self._read_slice_datasets(h5_file_reader, data_structure, args, message_handler, should_cancel)
    if result.invalid():
      return nx.Result(errors=result.errors)

    result = self._read_registered_datasets(h5_file_reader, data_structure, args, message_handler, should_cancel)
    if result.invalid():
      return nx.Result(errors=result.errors)

    return nx.Result()
  
  def _preflight_slice_datasets(self, h5_file_reader: h5py.File, origin: List[float], spacing: List[float], filter_args: dict, actions: nx.OutputActions, preflight_updated_values: List[nx.IFilter.PreflightValue]) -> Result:
    read_camera_data: bool = filter_args[ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY]
    read_camera_data_subvolume: bool = filter_args[ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY]
    camera_data_subvolume_minmax_x: list = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_X_KEY]
    camera_data_subvolume_minmax_y: list = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Y_KEY]
    camera_data_subvolume_minmax_z: list = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Z_KEY]
    slice_data_image_geom_path: nx.DataPath = filter_args[ReadPeregrineHDF5File.SLICE_DATA_KEY]
    slice_data_cell_attr_mat_name: str = filter_args[ReadPeregrineHDF5File.SLICE_DATA_CELL_ATTR_MAT_KEY]
    camera_data_0_array_name: str = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_0_ARRAY_NAME_KEY]
    camera_data_1_array_name: str = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_1_ARRAY_NAME_KEY]

    if read_camera_data:
      dims_result = self._read_dataset_dimensions(h5_file_reader, ReadPeregrineHDF5File.CAMERA_DATA_0_H5_PATH)
      if dims_result.invalid():
        return Result(errors=dims_result.errors)
      slice_dims = dims_result.value

      validate_result = self._validate_dataset_dimensions(h5_file_reader, ReadPeregrineHDF5File.CAMERA_DATA_1_H5_PATH, slice_dims)
      if validate_result.invalid():
        return Result(errors=validate_result.errors)

      if read_camera_data_subvolume:
        camera_data_dims_str = (
          f"Extents:\n"
          f"X Extent: 0 to {slice_dims[2] - 1} (dimension: {slice_dims[2]})\n"
          f"Y Extent: 0 to {slice_dims[1] - 1} (dimension: {slice_dims[1]})\n"
          f"Z Extent: 0 to {slice_dims[0] - 1} (dimension: {slice_dims[0]})\n"
          )

        preflight_value = nx.IFilter.PreflightValue()
        preflight_value.name = "Original Camera Data Dimensions (in pixels)"
        preflight_value.value = camera_data_dims_str
        preflight_updated_values.append(preflight_value)
      
        result: Result = self._validate_subvolume_dimensions(slice_dims, camera_data_subvolume_minmax_x, camera_data_subvolume_minmax_y, camera_data_subvolume_minmax_z)
        if result.invalid():
          return Result(errors=result.errors)

        slice_dims = [camera_data_subvolume_minmax_z[1] - camera_data_subvolume_minmax_z[0] + 1, camera_data_subvolume_minmax_y[1] - camera_data_subvolume_minmax_y[0] + 1,
                              camera_data_subvolume_minmax_x[1] - camera_data_subvolume_minmax_x[0] + 1]

      actions.append_action(nx.CreateImageGeometryAction(slice_data_image_geom_path, slice_dims[::-1], origin, spacing, slice_data_cell_attr_mat_name))

      camera_data_0_path: nx.DataPath = slice_data_image_geom_path.create_child_path(slice_data_cell_attr_mat_name).create_child_path(camera_data_0_array_name)
      actions.append_action(nx.CreateArrayAction(nx.DataType.float32, slice_dims, [1], camera_data_0_path))

      camera_data_1_path: nx.DataPath = slice_data_image_geom_path.create_child_path(slice_data_cell_attr_mat_name).create_child_path(camera_data_1_array_name)
      actions.append_action(nx.CreateArrayAction(nx.DataType.float32, slice_dims, [1], camera_data_1_path))

    return Result()
  
  def _preflight_registered_datasets(self, h5_file_reader: h5py.File, origin: List[float], spacing: List[float], filter_args: dict, actions: nx.OutputActions, preflight_updated_values: List[nx.IFilter.PreflightValue]) -> Result:
    registered_data_image_geom_path: nx.DataPath = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_KEY]
    registered_data_cell_attr_mat_name: str = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_CELL_ATTR_MAT_KEY]
    read_registered_data_subvolume: bool = filter_args[ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY]
    registered_data_subvolume_minmax_x: list = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_X_KEY]
    registered_data_subvolume_minmax_y: list = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Y_KEY]
    registered_data_subvolume_minmax_z: list = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Z_KEY]
    read_anomaly_detection: bool = filter_args[ReadPeregrineHDF5File.READ_ANOMALY_DETECTION_KEY]
    anomaly_detection_array_name: str = filter_args[ReadPeregrineHDF5File.ANOMALY_DETECTION_ARRAY_NAME_KEY]
    read_x_ray_ct: bool = filter_args[ReadPeregrineHDF5File.READ_X_RAY_CT_KEY]
    xray_ct_array_name: str = filter_args[ReadPeregrineHDF5File.XRAY_CT_ARRAY_NAME_KEY]

    anomaly_detection_dims_result: Result[Tuple[int]] = self._read_dataset_dimensions(h5_file_reader, ReadPeregrineHDF5File.REGISTERED_ANOMALY_DETECTION_H5_PATH)
    if anomaly_detection_dims_result.invalid():
      return Result(errors=anomaly_detection_dims_result.errors)
    registered_dims: List[int] = anomaly_detection_dims_result.value

    xray_ct_dims_validation_result: Result = self._validate_dataset_dimensions(h5_file_reader, ReadPeregrineHDF5File.REGISTERED_XRAY_CT_H5_PATH, registered_dims)
    if xray_ct_dims_validation_result.invalid():
      return Result(errors=xray_ct_dims_validation_result.errors)

    if read_registered_data_subvolume:
      registered_dims_str = (
          f"Extents:\n"
          f"X Extent: 0 to {registered_dims[2] - 1} (dimension: {registered_dims[2]})\n"
          f"Y Extent: 0 to {registered_dims[1] - 1} (dimension: {registered_dims[1]})\n"
          f"Z Extent: 0 to {registered_dims[0] - 1} (dimension: {registered_dims[0]})\n"
          )

      preflight_value = nx.IFilter.PreflightValue()
      preflight_value.name = "Original Registered Data Dimensions (in pixels)"
      preflight_value.value = registered_dims_str
      preflight_updated_values.append(preflight_value)
      
      subvolume_validation_result: Result = self._validate_subvolume_dimensions(registered_dims, registered_data_subvolume_minmax_x, registered_data_subvolume_minmax_y, registered_data_subvolume_minmax_z)
      if subvolume_validation_result.invalid():
        return Result(errors=subvolume_validation_result.errors)

      registered_dims = [registered_data_subvolume_minmax_z[1] - registered_data_subvolume_minmax_z[0] + 1, registered_data_subvolume_minmax_y[1] - registered_data_subvolume_minmax_y[0] + 1,
                                  registered_data_subvolume_minmax_x[1] - registered_data_subvolume_minmax_x[0] + 1]

    actions.append_action(nx.CreateImageGeometryAction(registered_data_image_geom_path, registered_dims[::-1], origin, spacing, registered_data_cell_attr_mat_name))

    if read_anomaly_detection:
      anomaly_detection_path: nx.DataPath = registered_data_image_geom_path.create_child_path(registered_data_cell_attr_mat_name).create_child_path(anomaly_detection_array_name)
      actions.append_action(nx.CreateArrayAction(nx.DataType.uint8, registered_dims, [1], anomaly_detection_path))

    if read_x_ray_ct:
      xray_ct_path: nx.DataPath = registered_data_image_geom_path.create_child_path(registered_data_cell_attr_mat_name).create_child_path(xray_ct_array_name)
      actions.append_action(nx.CreateArrayAction(nx.DataType.uint8, registered_dims, [1], xray_ct_path))

    return Result()
  
  def _open_hdf5_dataset(self, h5_file_reader: h5py.File, h5_dataset_path: str) -> Result[h5py.Dataset]:
    if h5_dataset_path not in h5_file_reader:
      return make_error_result(code=-4001, message=f"Error opening dataset at path '{h5_dataset_path}' in HDF5 file '{h5_file_reader.name}': Dataset does not exist!")

    try:
      dataset = h5_file_reader[h5_dataset_path]
    except KeyError as e:
      return make_error_result(code=-4002, message=f"Error opening dataset at path '{h5_dataset_path}' in HDF5 file '{h5_file_reader.name}': {e}")
    except Exception as e:
      return make_error_result(code=-4003, message=f"Error opening dataset at path '{h5_dataset_path}' in HDF5 file '{h5_file_reader.name}': {e}")

    return Result(value=dataset)

  def _read_dataset_dimensions(self, h5_file_reader: h5py.File, h5_dataset_path: str) -> Result[List[int]]:
    result: Result[h5py.Dataset] = self._open_hdf5_dataset(h5_file_reader, h5_dataset_path)
    if result.invalid():
      return Result(errors=result.errors)
    dataset: h5py.Dataset = result.value

    return Result(value=list(dataset.shape))

  def _validate_dataset_dimensions(self, h5_file_reader: h5py.File, h5_dataset_path: str, sliceDims: List[int]) -> Result:
    dims_result = self._read_dataset_dimensions(h5_file_reader, h5_dataset_path)
    if dims_result.invalid():
      return nx.IFilter.PreflightResult(nx.OutputActions(), dims_result.errors)
    
    dims = dims_result.value
    if dims != sliceDims:
      return make_error_result(code=-3013, message=f"Dataset at path '{h5_dataset_path}' has dimensions ({dims}) that do not match the slice dimensions ({sliceDims})")

    return Result()
  
  def _validate_subvolume_dimensions(self, volume_dims: List[int], subvolume_min_max_x: List[int], subvolume_min_max_y: List[int], subvolume_min_max_z: List[int]) -> Result:
    subvolume_x = subvolume_min_max_x[1] - subvolume_min_max_x[0] + 1
    subvolume_y = subvolume_min_max_y[1] - subvolume_min_max_y[0] + 1
    subvolume_z = subvolume_min_max_z[1] - subvolume_min_max_z[0] + 1

    if subvolume_min_max_x[0] > subvolume_min_max_x[1]:
      return make_error_result(code=-3020, message=f"Subvolume minimum X dimension '{subvolume_min_max_x[0]}' is larger than the subvolume maximum X dimension '{subvolume_min_max_x[1]}'.")

    if subvolume_x > volume_dims[2]:
      return make_error_result(code=-3021, message=f"Subvolume X dimension '{subvolume_x}' ({subvolume_min_max_x[1]} - {subvolume_min_max_x[0]} + 1) is larger than the X dimension value found in the input file data ('{volume_dims[2]}')")

    if subvolume_min_max_y[0] > subvolume_min_max_y[1]:
      return make_error_result(code=-3022, message=f"Subvolume minimum Y dimension '{subvolume_min_max_y[0]}' is larger than the subvolume maximum Y dimension '{subvolume_min_max_y[1]}'.")

    if subvolume_y > volume_dims[1]:
      return make_error_result(code=-3023, message=f"Subvolume Y dimension '{subvolume_y}' ({subvolume_min_max_y[1]} - {subvolume_min_max_y[0]} + 1) is larger than the Y dimension value found in the input file data ('{volume_dims[1]}')")

    if subvolume_min_max_z[0] > subvolume_min_max_z[1]:
      return make_error_result(code=-3024, message=f"Subvolume minimum Z dimension '{subvolume_min_max_z[0]}' is larger than the subvolume maximum Z dimension '{subvolume_min_max_z[1]}'.")

    if subvolume_z > volume_dims[0]:
      return make_error_result(code=-3025, message=f"Subvolume Z dimension '{subvolume_z}' ({subvolume_min_max_z[1]} - {subvolume_min_max_z[0]} + 1) is larger than the Z dimension value found in the input file data ('{volume_dims[0]}')")

    return Result()

  def _calculate_spacing(self, h5_file_reader: h5py.File) -> Result[List[float]]:
    if ReadPeregrineHDF5File.X_REAL_DIMENSION_PATH not in h5_file_reader.attrs:
       return make_error_result(code=-3007, message=f"Attribute at path '{ReadPeregrineHDF5File.X_REAL_DIMENSION_PATH}' does not exist, so the X spacing cannot be calculated!")
    try:
      x_real_dim = h5_file_reader.attrs[ReadPeregrineHDF5File.X_REAL_DIMENSION_PATH]
    except KeyError:
      return make_error_result(code=-3008, message=f"Attribute at path '{ReadPeregrineHDF5File.X_REAL_DIMENSION_PATH}' cannot be accessed, so the X spacing cannot be calculated!")

    if ReadPeregrineHDF5File.Y_REAL_DIMENSION_PATH not in h5_file_reader.attrs:
       return make_error_result(code=-3009, message=f"Attribute at path '{ReadPeregrineHDF5File.Y_REAL_DIMENSION_PATH}' does not exist, so the Y spacing cannot be calculated!")
    try:
      y_real_dim = h5_file_reader.attrs[ReadPeregrineHDF5File.Y_REAL_DIMENSION_PATH]
    except KeyError:
      return make_error_result(code=-3010, message=f"Attribute at path '{ReadPeregrineHDF5File.Y_REAL_DIMENSION_PATH}' cannot be accessed, so the Y spacing cannot be calculated!")

    if ReadPeregrineHDF5File.X_CAMERA_DIMENSION_PATH not in h5_file_reader.attrs:
       return make_error_result(code=-3011, message=f"Attribute at path '{ReadPeregrineHDF5File.X_CAMERA_DIMENSION_PATH}' does not exist, so the X spacing cannot be calculated!")
    try:
      x_camera_dim = h5_file_reader.attrs[ReadPeregrineHDF5File.X_CAMERA_DIMENSION_PATH]
    except KeyError:
      return make_error_result(code=-3012, message=f"Attribute at path '{ReadPeregrineHDF5File.X_CAMERA_DIMENSION_PATH}' cannot be accessed, so the X spacing cannot be calculated!")

    if ReadPeregrineHDF5File.Y_CAMERA_DIMENSION_PATH not in h5_file_reader.attrs:
       return make_error_result(code=-3013, message=f"Attribute at path '{ReadPeregrineHDF5File.Y_CAMERA_DIMENSION_PATH}' does not exist, so the Y spacing cannot be calculated!")
    try:
      y_camera_dim = h5_file_reader.attrs[ReadPeregrineHDF5File.Y_CAMERA_DIMENSION_PATH]
    except KeyError:
      return make_error_result(code=-3014, message=f"Attribute at path '{ReadPeregrineHDF5File.Y_CAMERA_DIMENSION_PATH}' cannot be accessed, so the Y spacing cannot be calculated!")

    if ReadPeregrineHDF5File.LAYER_THICKNESS_PATH not in h5_file_reader.attrs:
       return make_error_result(code=-3015, message=f"Attribute at path '{ReadPeregrineHDF5File.LAYER_THICKNESS_PATH}' does not exist, so the Z spacing cannot be calculated!")
    try:
      z_spacing = h5_file_reader.attrs[ReadPeregrineHDF5File.LAYER_THICKNESS_PATH]
    except KeyError:
      return make_error_result(code=-3016, message=f"Attribute at path '{ReadPeregrineHDF5File.LAYER_THICKNESS_PATH}' cannot be accessed, so the Z spacing cannot be calculated!")

    spacing = [float(x_real_dim / x_camera_dim), float(y_real_dim / y_camera_dim), float(z_spacing)]
    return Result(value=spacing)
  
  def _read_slice_datasets(self, h5_file_reader: h5py.File, data_structure: nx.DataStructure, filter_args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> Result:
    read_camera_data: bool = filter_args[ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY]
    read_camera_data_subvolume: bool = filter_args[ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY]
    camera_data_subvolume_minmax_x: list = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_X_KEY]
    camera_data_subvolume_minmax_y: list = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Y_KEY]
    camera_data_subvolume_minmax_z: list = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Z_KEY]
    slice_data_image_geom_path: nx.DataPath = filter_args[ReadPeregrineHDF5File.SLICE_DATA_KEY]
    slice_data_cell_attr_mat_name: str = filter_args[ReadPeregrineHDF5File.SLICE_DATA_CELL_ATTR_MAT_KEY]
    camera_data_0_array_name: str = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_0_ARRAY_NAME_KEY]
    camera_data_1_array_name: str = filter_args[ReadPeregrineHDF5File.CAMERA_DATA_1_ARRAY_NAME_KEY]

    # Read the camera data
    if read_camera_data:
      if should_cancel:
        return Result()

      message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, 'Reading Camera Dataset 0...'))
      camera_data_0_nx_path: nx.DataPath = slice_data_image_geom_path.create_child_path(slice_data_cell_attr_mat_name).create_child_path(camera_data_0_array_name)
      camera_data_0_h5_result: Result[h5py.Dataset] = self._open_hdf5_dataset(h5_file_reader, ReadPeregrineHDF5File.CAMERA_DATA_0_H5_PATH)
      if camera_data_0_h5_result.invalid():
        return Result(errors=camera_data_0_h5_result.errors)
      camera_data_0_h5 = camera_data_0_h5_result.value
      camera_data_0_nx: np.array = data_structure[camera_data_0_nx_path].npview()
      camera_data_0_nx = np.squeeze(camera_data_0_nx)

      if read_camera_data_subvolume:
        camera_data_0_nx[:] = camera_data_0_h5[camera_data_subvolume_minmax_z[0]:camera_data_subvolume_minmax_z[1]+1, camera_data_subvolume_minmax_y[0]:camera_data_subvolume_minmax_y[1]+1, camera_data_subvolume_minmax_x[0]:camera_data_subvolume_minmax_x[1]+1]
      else:
        camera_data_0_nx[:] = camera_data_0_h5

      if should_cancel:
        return Result()

      message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, 'Reading Camera Dataset 1...'))
      camera_data_1_nx_path: nx.DataPath = slice_data_image_geom_path.create_child_path(slice_data_cell_attr_mat_name).create_child_path(camera_data_1_array_name)
      camera_data_1_h5_result: Result[h5py.Dataset] = self._open_hdf5_dataset(h5_file_reader, ReadPeregrineHDF5File.CAMERA_DATA_1_H5_PATH)
      if camera_data_1_h5_result.invalid():
        return Result(errors=camera_data_1_h5_result.errors)
      camera_data_1_h5 = camera_data_1_h5_result.value
      camera_data_1_nx: np.array = data_structure[camera_data_1_nx_path].npview()
      camera_data_1_nx = np.squeeze(camera_data_1_nx)
      
      if read_camera_data_subvolume:
        camera_data_1_nx[:] = camera_data_1_h5[camera_data_subvolume_minmax_z[0]:camera_data_subvolume_minmax_z[1]+1, camera_data_subvolume_minmax_y[0]:camera_data_subvolume_minmax_y[1]+1, camera_data_subvolume_minmax_x[0]:camera_data_subvolume_minmax_x[1]+1]
      else:
        camera_data_1_nx[:] = camera_data_1_h5

    return Result()
  
  def _read_registered_datasets(self, h5_file_reader: h5py.File, data_structure: nx.DataStructure, filter_args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> Result:
    registered_data_image_geom_path: nx.DataPath = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_KEY]
    registered_data_cell_attr_mat_name: str = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_CELL_ATTR_MAT_KEY]
    read_registered_data_subvolume: bool = filter_args[ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY]
    registered_data_subvolume_minmax_x: list = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_X_KEY]
    registered_data_subvolume_minmax_y: list = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Y_KEY]
    registered_data_subvolume_minmax_z: list = filter_args[ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Z_KEY]
    read_anomaly_detection: bool = filter_args[ReadPeregrineHDF5File.READ_ANOMALY_DETECTION_KEY]
    anomaly_detection_array_name: str = filter_args[ReadPeregrineHDF5File.ANOMALY_DETECTION_ARRAY_NAME_KEY]
    read_x_ray_ct: bool = filter_args[ReadPeregrineHDF5File.READ_X_RAY_CT_KEY]
    xray_ct_array_name: str = filter_args[ReadPeregrineHDF5File.XRAY_CT_ARRAY_NAME_KEY]
    
    if should_cancel:
        return Result()

    # Read the anomaly detection dataset
    if read_anomaly_detection:
      message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, 'Reading Anomaly Detection...'))
      anomaly_detection_nx_path: nx.DataPath = registered_data_image_geom_path.create_child_path(registered_data_cell_attr_mat_name).create_child_path(anomaly_detection_array_name)
      anomaly_detection_h5_result: Result[h5py.Dataset] = self._open_hdf5_dataset(h5_file_reader, ReadPeregrineHDF5File.REGISTERED_ANOMALY_DETECTION_H5_PATH)
      if anomaly_detection_h5_result.invalid():
        return Result(errors=anomaly_detection_h5_result.errors)
      anomaly_detection_h5 = anomaly_detection_h5_result.value
      anomaly_detection_nx = data_structure[anomaly_detection_nx_path].npview()
      anomaly_detection_nx = np.squeeze(anomaly_detection_nx)

      if read_registered_data_subvolume:
        anomaly_detection_nx[:] = anomaly_detection_h5[registered_data_subvolume_minmax_z[0]:registered_data_subvolume_minmax_z[1]+1, registered_data_subvolume_minmax_y[0]:registered_data_subvolume_minmax_y[1]+1, registered_data_subvolume_minmax_x[0]:registered_data_subvolume_minmax_x[1]+1]
      else:
        anomaly_detection_nx[:] = anomaly_detection_h5

    if should_cancel:
        return Result()

    # Read the x-ray CT dataset
    if read_x_ray_ct:
      message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, 'Reading X-Ray CT...'))
      xray_ct_nx_path: nx.DataPath = registered_data_image_geom_path.create_child_path(registered_data_cell_attr_mat_name).create_child_path(xray_ct_array_name)
      xray_ct_h5_result: Result[h5py.Dataset] = self._open_hdf5_dataset(h5_file_reader, ReadPeregrineHDF5File.REGISTERED_XRAY_CT_H5_PATH)
      if xray_ct_h5_result.invalid():
        return Result(errors=xray_ct_h5_result.errors)
      xray_ct_h5 = xray_ct_h5_result.value
      xray_ct_nx: np.array = data_structure[xray_ct_nx_path].npview()
      xray_ct_nx = np.squeeze(xray_ct_nx)

      if read_registered_data_subvolume:
        xray_ct_nx[:] = xray_ct_h5[registered_data_subvolume_minmax_z[0]:registered_data_subvolume_minmax_z[1]+1, registered_data_subvolume_minmax_y[0]:registered_data_subvolume_minmax_y[1]+1, registered_data_subvolume_minmax_x[0]:registered_data_subvolume_minmax_x[1]+1]
      else:
        xray_ct_nx[:] = xray_ct_h5

    return Result()