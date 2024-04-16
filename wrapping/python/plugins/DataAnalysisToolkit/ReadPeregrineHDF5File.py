from typing import List
import simplnx as nx
from .common import Result

class Error:
    def __init__(self, code=0, message=""):
        self.code = code
        self.message = message

class Warning:
    def __init__(self, code=0, message=""):
        self.code = code
        self.message = message

class Result:
    def __init__(self, value=None, error=None):
        self.value = value
        self.error = error

    def get_value(self):
        if self.error:
            raise Exception(f"Error {self.error.code}: {self.error.message}")
        return self.value


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
    return 'Read Peregrine HDF5 File (Python)'
 
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

    params.link_parameters(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_X_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Y_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_REGISTERED_DATA_SUBVOLUME_KEY, ReadPeregrineHDF5File.REGISTERED_DATA_SUBVOLUME_MINMAX_Z_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY, ReadPeregrineHDF5File.CAMERA_DATA_0_ARRAY_NAME_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY, ReadPeregrineHDF5File.CAMERA_DATA_1_ARRAY_NAME_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_ANOMALY_DETECTION_KEY, ReadPeregrineHDF5File.ANOMALY_DETECTION_ARRAY_NAME_KEY, True)
    params.link_parameters(ReadPeregrineHDF5File.READ_X_RAY_CT_KEY, ReadPeregrineHDF5File.XRAY_CT_ARRAY_NAME_KEY, True)

    return params
  
  def preflightSliceDatasets(h5FileReader: nx.HDF5.FileReader, origin: List[float], spacing: List[float], filterArgs: dict) -> nx.IFilter.PreflightResult:
    read_camera_data = filterArgs[ReadPeregrineHDF5File.READ_CAMERA_DATA_KEY]
    read_camera_data_subvolume = filterArgs[ReadPeregrineHDF5File.READ_CAMERA_DATA_SUBVOLUME_KEY]
    camera_data_subvolume_minmax_x = filterArgs[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_X_KEY]
    camera_data_subvolume_minmax_y = filterArgs[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Y_KEY]
    camera_data_subvolume_minmax_z = filterArgs[ReadPeregrineHDF5File.CAMERA_DATA_SUBVOLUME_MINMAX_Z_KEY]
    slice_data_image_geom_path = filterArgs[ReadPeregrineHDF5File.SLICE_DATA_KEY]
    slice_data_cell_attr_mat_name = filterArgs[ReadPeregrineHDF5File.SLICE_DATA_CELL_ATTR_MAT_KEY]
    camera_data_0_array_name = filterArgs[ReadPeregrineHDF5File.CAMERA_DATA_0_ARRAY_NAME_KEY]
    camera_data_1_array_name = filterArgs[ReadPeregrineHDF5File.CAMERA_DATA_1_ARRAY_NAME_KEY]

    actions = nx.OutputActions()
    preflight_updated_values = []

  # segmentationResultsStr = StringUtilities::trimmed(segmentationResultsStr);
  # auto segmentationResultsList = StringUtilities::split(segmentationResultsStr, std::vector<char>{','}, true);
  # if(segmentationResultsList.empty())
  # {
  #   return {nonstd::make_unexpected(
  #       std::vector<Error>{Error{-3000, "The segmentation results are empty.  Please input the segmentation results that this filter should read from the input file, separated by commas."}})};
  # }

  # Result<std::vector<usize>> sliceDimsResult = readSliceDimensions(h5FileReader, segmentationResultsList);
  # if(sliceDimsResult.invalid())
  # {
  #   return {nonstd::make_unexpected(sliceDimsResult.errors())};
  # }

  # std::vector<usize> sliceDims = sliceDimsResult.value();
  # std::vector<usize> slicesImageGeomDims = sliceDims;

  # if(readSlicesSubvolume)
  # {
  #   Result<> result = validateSubvolumeDimensions(sliceDims, slicesSubvolumeMinMaxX, slicesSubvolumeMinMaxY, slicesSubvolumeMinMaxZ);
  #   if(result.invalid())
  #   {
  #     return {nonstd::make_unexpected(result.errors())};
  #   }
  #   slicesImageGeomDims = {slicesSubvolumeMinMaxZ[1] - slicesSubvolumeMinMaxZ[0] + 1, slicesSubvolumeMinMaxY[1] - slicesSubvolumeMinMaxY[0] + 1,
  #                          slicesSubvolumeMinMaxX[1] - slicesSubvolumeMinMaxX[0] + 1};
  # }

  # actions.appendAction(
  #     std::make_unique<CreateImageGeometryAction>(sliceDataImageGeomPath, std::vector<usize>(slicesImageGeomDims.rbegin(), slicesImageGeomDims.rend()), origin, spacing, sliceDataCellAttrMatName));

  # for(const auto& segmentationResult : segmentationResultsList)
  # {
  #   DataPath segmentationResultPath = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(segmentationResult);
  #   actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, slicesImageGeomDims, std::vector<usize>{1}, segmentationResultPath));
  # }

  # if(readCameraData)
  # {
  #   Result<> result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_CameraData0H5Path, sliceDims);
  #   if(result.invalid())
  #   {
  #     return {nonstd::make_unexpected(result.errors())};
  #   }

  #   DataPath cameraData0Path = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(cameraData0ArrayName);
  #   actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, slicesImageGeomDims, std::vector<usize>{1}, cameraData0Path));

  #   result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_CameraData1H5Path, sliceDims);
  #   if(result.invalid())
  #   {
  #     return {nonstd::make_unexpected(result.errors())};
  #   }

  #   DataPath cameraData1Path = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(cameraData1ArrayName);
  #   actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, slicesImageGeomDims, std::vector<usize>{1}, cameraData1Path));
  # }

  # if(readPartIds)
  # {
  #   Result<> result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_PartIdsH5Path, sliceDims);
  #   if(result.invalid())
  #   {
  #     return {nonstd::make_unexpected(result.errors())};
  #   }

  #   DataPath partIdsPath = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(partIdsArrayName);
  #   actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint32, slicesImageGeomDims, std::vector<usize>{1}, partIdsPath));
  # }

  # if(readSampleIds)
  # {
  #   Result<> result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_SampleIdsH5Path, sliceDims);
  #   if(result.invalid())
  #   {
  #     return {nonstd::make_unexpected(result.errors())};
  #   }

  #   DataPath sampleIdsPath = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(sampleIdsArrayName);
  #   actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint32, slicesImageGeomDims, std::vector<usize>{1}, sampleIdsPath));
  # }

  # if(readSlicesSubvolume)
  # {
  #   std::stringstream ss;
  #   ss << "Extents:\n"
  #      << "X Extent: 0 to " << sliceDims[2] - 1 << " (dimension: " << sliceDims[2] << ")\n"
  #      << "Y Extent: 0 to " << sliceDims[1] - 1 << " (dimension: " << sliceDims[1] << ")\n"
  #      << "Z Extent: 0 to " << sliceDims[0] - 1 << " (dimension: " << sliceDims[0] << ")\n";
  #   std::string sliceDataDimsStr = ss.str();

  #   preflightUpdatedValues.push_back({"Original Slice Data Dimensions (in pixels)", sliceDataDimsStr});
  # }

  # return {{actions}, std::move(preflightUpdatedValues)};

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """
    input_file_path = args[ReadPeregrineHDF5File.INPUT_FILE_PATH_KEY]

    actions = nx.OutputActions()
    preflight_updated_values = []

    h5_file_reader = nx.HDF5.FileReader(str(input_file_path))
    if not h5_file_reader.is_valid():
      return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-3001, f"Error opening Peregrine HDF5 file: '{str(input_file_path)}'")])

    spacing_result: Result = self._calculate_spacing(h5_file_reader)
    if not spacing_result.valid():
      return nx.IFilter.PreflightResult(nx.OutputActions(), spacing_result.errors)

    origin: List[float] = [0.0, 0.0, 0.0]
    spacing: List[float] = spacing_result.value()

    return nx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """

    value: float = args[ReadPeregrineHDF5File.TEST_KEY]
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Execute: {value}'))
    return nx.Result()

  def _calculate_spacing(self, h5_file_reader: nx.HDF5.FileReader) -> Result:
    attr_reader = h5_file_reader.get_attribute(ReadPeregrineHDF5File.X_REAL_DIMENSION_PATH)
    if not attr_reader.is_valid():
      return Result(errors=[nx.Error(-3007, f"Attribute at path '{ReadPeregrineHDF5File.X_REAL_DIMENSION_PATH}' cannot be found, so the X spacing cannot be calculated!")])
    
    attr_id = attr_reader.get_attr_id()
    name = attr_reader.get_name()

    try:
      x_real_dim = attr_reader.read_as_value()
      print(f"Read value: {x_real_dim}")
    except Exception as e:
        print(f"Failed to read attribute value: {str(e)}")

    attr_reader = h5_file_reader.get_attribute(ReadPeregrineHDF5File.Y_REAL_DIMENSION_PATH)
    if not attr_reader.is_valid():
      return Result(errors=[nx.Error(-3008, f"Attribute at path '{ReadPeregrineHDF5File.Y_REAL_DIMENSION_PATH}' cannot be found, so the Y spacing cannot be calculated!")])
    
    y_real_dim = attr_reader.read_as_value()

    attr_reader = h5_file_reader.get_attribute(ReadPeregrineHDF5File.X_CAMERA_DIMENSION_PATH)
    if not attr_reader.is_valid():
      return Result(errors=[nx.Error(-3009, f"Attribute at path '{ReadPeregrineHDF5File.X_CAMERA_DIMENSION_PATH}' cannot be found, so the X spacing cannot be calculated!")])
    
    x_camera_dim = attr_reader.read_as_value()

    attr_reader = h5_file_reader.get_attribute(ReadPeregrineHDF5File.Y_CAMERA_DIMENSION_PATH)
    if not attr_reader.is_valid():
      return Result(errors=[nx.Error(-3010, f"Attribute at path '{ReadPeregrineHDF5File.Y_CAMERA_DIMENSION_PATH}' cannot be found, so the Y spacing cannot be calculated!")])
    
    y_camera_dim = attr_reader.read_as_value()

    attr_reader = h5_file_reader.get_attribute(ReadPeregrineHDF5File.LAYER_THICKNESS_PATH)
    if not attr_reader.is_valid():
      return Result(errors=[nx.Error(-3011, f"Attribute at path '{ReadPeregrineHDF5File.LAYER_THICKNESS_PATH}' cannot be found, so the Z spacing cannot be calculated!")])
    
    z_spacing = attr_reader.read_as_value()

    return [float(x_real_dim / x_camera_dim), float(y_real_dim / y_camera_dim), float(z_spacing)]