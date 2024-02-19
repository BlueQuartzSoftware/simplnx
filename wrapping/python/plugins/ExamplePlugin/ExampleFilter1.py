from typing import List
import simplnx as sx
import orientationanalysis as oa

class ExampleFilter1:
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  INPUT_DIR_KEY = 'input_dir'
  INPUT_FILE_KEY = 'input_file'
  OUTPUT_DIR_KEY = 'output_dir'
  OUTPUT_FILE_KEY = 'output_file'
  PARAM1_KEY = "param1"
  PARAM2_KEY = "param2"
  PARAM3_KEY = "param3"
  PARAM4_KEY = "param4"
  PARAM5_KEY = "param5"
  PARAM6_KEY = "param6"
  PARAM7_KEY = "param7"
  PARAM8_KEY = "param8"
  PARAM9_KEY = "param9"
  PARAM10_KEY = "param10"
  PARAM11_KEY = "param11"
  PARAM12_KEY = "param12"
  PARAM13_KEY = "param13"
  PARAM14_KEY = "param14"
  PARAM15_KEY = "param15"
  PARAM16_KEY = 'param16'
  PARAM17_KEY = 'param17'
  PARAM18_KEY = 'param18'
  PARAM19_KEY = 'param19'
  PARAM20_KEY = 'param20'
  VEC2_KEY = 'vec2_key'
  VEC3_KEY = 'vec3_key'
  VEC4_KEY = 'vec4_key'
  VEC4F_KEY = 'vec4f_key'
  VEC6F_KEY = 'vec6f_key'

  def uuid(self) -> sx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return sx.Uuid('33549513-69d9-441b-bfc1-2744977a1c61')

  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return 'Example Filter 1 (Python)'

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'ExampleFilter1'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'ExampleFilter1'

  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python']

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  ExampleFilter1
    """
    return ExampleFilter1()

  def parameters(self) -> sx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = sx.Parameters()

    params.insert(sx.Parameters.Separator("FileSystem Selections"))
    params.insert(sx.FileSystemPathParameter(ExampleFilter1.INPUT_DIR_KEY, 'Input Directory', 'Example input directory help text', 'Data', set(), sx.FileSystemPathParameter.PathType.InputDir))
    params.insert(sx.FileSystemPathParameter(ExampleFilter1.INPUT_FILE_KEY, 'Input File', 'Example input file help text', '/opt/local/bin/ninja', set(), sx.FileSystemPathParameter.PathType.InputFile, True))
    params.insert(sx.FileSystemPathParameter(ExampleFilter1.OUTPUT_DIR_KEY, 'Output Directory', 'Example output directory help text', 'Output Data', set(), sx.FileSystemPathParameter.PathType.OutputDir))
    params.insert(sx.FileSystemPathParameter(ExampleFilter1.OUTPUT_FILE_KEY, 'Output File', 'Example output file help text', '', set(), sx.FileSystemPathParameter.PathType.OutputFile))

    params.insert(sx.Parameters.Separator("Linked Parameter"))
    params.insert_linkable_parameter(sx.BoolParameter(ExampleFilter1.PARAM2_KEY, 'BoolParameter', 'The 2nd parameter', True))
    params.insert(sx.Float32Parameter(ExampleFilter1.PARAM1_KEY, 'Float32Parameter', 'The 1st parameter', 0.1234))
    params.insert(sx.Int32Parameter(ExampleFilter1.PARAM3_KEY, 'Int32Parameter', 'The 3rd parameter', 0))
    params.link_parameters(ExampleFilter1.PARAM2_KEY, ExampleFilter1.PARAM1_KEY, True)

    params.insert(sx.Parameters.Separator("Vector Parameters"))
    params.insert(sx.VectorInt32Parameter(ExampleFilter1.VEC2_KEY, 'Vect<int,2>', 'Example int32 vector help text', [10, 20], ["X", "Y"]))
    params.insert(sx.VectorInt32Parameter(ExampleFilter1.VEC3_KEY, '3D Dimensions', 'Example int32 vector help text', [-19, -100, 456], ["X", "Y", "Z"]))
    params.insert(sx.VectorUInt8Parameter(ExampleFilter1.VEC4_KEY, 'RGBA', 'Example uint8 vector help text', [0, 255, 128, 255], ["R", "G", "B", "A"]))
    params.insert(sx.VectorFloat32Parameter(ExampleFilter1.VEC4F_KEY, 'Quaternion', 'Example float32 vector help text', [0, 84.98, 234.12, 985.98], ["U", "V", "W", "X"]))
    
    params.insert(sx.Parameters.Separator("Other Parameters"))
    params.insert(sx.StringParameter(ExampleFilter1.PARAM5_KEY, 'StringParameter', 'Example string help text', 'Test String'))
    params.insert(sx.DataObjectNameParameter(ExampleFilter1.PARAM11_KEY, 'DataObjectNameParameter', 'Example help text for DataObjectNameParameter', 'Data Group'))
    params.insert(sx.NumericTypeParameter(ExampleFilter1.PARAM6_KEY, 'Numeric Type', 'Example numeric type help text', sx.NumericType.int32))
    params.insert(sx.DataTypeParameter(ExampleFilter1.PARAM13_KEY, "Data Type", "Example data type help text", sx.DataType.float64))
    params.insert(sx.DataStoreFormatParameter(ExampleFilter1.PARAM12_KEY, 'Data Store Format', 'This value will specify which data format is used by the array\'s data store. An empty string results in in-memory data store.', ""))

    import_data = sx.Dream3dImportParameter.ImportData()
    import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
    import_data.data_paths = None
    params.insert(sx.Dream3dImportParameter(ExampleFilter1.PARAM14_KEY, "Import File Path", "The HDF5 file path the DataStructure should be imported from.", import_data))

    ensemble_info = []
    ensemble_info.append(["Hexagonal-High 6/mmm","Primary","Phase 1"])
    ensemble_info.append(["Cubic-High m-3m","Primary","Phase 2"])
    params.insert(sx.EnsembleInfoParameter(ExampleFilter1.PARAM15_KEY, "Created Ensemble Info", "The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase.", ensemble_info))

    color_control_points = sx.Json('{"RGBPoints": [0,0,0,0,0.4,0.901960784314,0,0,0.8,0.901960784314,0.901960784314,0,1,1,1,1]}')
    params.insert(sx.GenerateColorTableParameter(ExampleFilter1.PARAM16_KEY, "Select Preset...", "Select a preset color scheme to apply to the created array", color_control_points))

    dataset1 = sx.ReadHDF5DatasetParameter.DatasetImportInfo()
    dataset1.dataset_path = "/DataStructure/DataContainer/CellData/Confidence Index"
    dataset1.tuple_dims = "117,201,189"
    dataset1.component_dims = "1"
    dataset2 = sx.ReadHDF5DatasetParameter.DatasetImportInfo()
    dataset2.dataset_path = "/DataStructure/DataContainer/CellData/EulerAngles"
    dataset2.tuple_dims = "117,201,189"
    dataset2.component_dims = "3"
    import_hdf5_param = sx.ReadHDF5DatasetParameter.ValueType()
    import_hdf5_param.input_file = "SmallIN100_Final.dream3d"
    import_hdf5_param.datasets = [dataset1, dataset2]
    params.insert(sx.ReadHDF5DatasetParameter(ExampleFilter1.PARAM20_KEY, "Select HDF5 File", "The HDF5 file data to import", import_hdf5_param))

    params.insert(sx.Parameters.Separator("Generated File List Parameter"))
    generated_file_list_value = sx.GeneratedFileListParameter.ValueType()
    generated_file_list_value.input_path = "DREAM3DNXData/Data/Porosity_Image"
    generated_file_list_value.ordering = sx.GeneratedFileListParameter.Ordering.LowToHigh
    generated_file_list_value.file_prefix = "slice-"
    generated_file_list_value.file_suffix = ""
    generated_file_list_value.file_extension = ".tif"
    generated_file_list_value.start_index = 11
    generated_file_list_value.end_index = 174
    generated_file_list_value.increment_index = 1
    generated_file_list_value.padding_digits = 2
    params.insert(sx.GeneratedFileListParameter(ExampleFilter1.PARAM17_KEY, "Input File List", "The list of files to be read", generated_file_list_value))

    params.insert(sx.Parameters.Separator("Read CSV File Parameter"))
    read_csv_data = sx.ReadCSVDataParameter()
    read_csv_data.input_file_path = "/tmp/test_csv_data.csv"
    read_csv_data.start_import_row = 2
    read_csv_data.delimiters = [',']
    read_csv_data.custom_headers = []
    read_csv_data.column_data_types = [sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.int32]
    read_csv_data.skipped_array_mask = [False,False,False,False,False,False,False]
    read_csv_data.tuple_dims = [37989]
    read_csv_data.headers_line = 1
    read_csv_data.header_mode = sx.ReadCSVDataParameter.HeaderMode.Line
    params.insert(sx.ReadCSVFileParameter(ExampleFilter1.PARAM18_KEY, "CSV Importer Data", "Holds all relevant csv file data collected from the custom interface", read_csv_data))

    params.insert(sx.Parameters.Separator("Read H5EBSD Parameter"))
    read_h5ebsd_data = oa.ReadH5EbsdFileParameter.ValueType()
    read_h5ebsd_data.euler_representation=0
    read_h5ebsd_data.end_slice=117
    read_h5ebsd_data.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
    read_h5ebsd_data.input_file_path="Data/Output/Reconstruction/Small_IN100.h5ebsd"
    read_h5ebsd_data.start_slice=1
    read_h5ebsd_data.use_recommended_transform=True
    params.insert(oa.ReadH5EbsdFileParameter(ExampleFilter1.PARAM19_KEY, "Import H5Ebsd File", "Object that holds all relevant information to import data from the file.", read_h5ebsd_data))

    params.insert(sx.Parameters.Separator("Big Parameters"))
    params.insert(sx.GeneratedFileListParameter(ExampleFilter1.PARAM4_KEY, 'Input File List', 'The values that are used to generate the input file list. See GeneratedFileListParameter for more information.', sx.GeneratedFileListParameter.ValueType()))
    params.insert(sx.ArrayThresholdsParameter(ExampleFilter1.PARAM7_KEY, 'Data Thresholds', 'DataArray thresholds to mask', sx.ArrayThresholdSet()))

    params.insert(sx.Parameters.Separator("Multiple Linked Parameters"))
    params.insert_linkable_parameter(sx.BoolParameter(ExampleFilter1.PARAM8_KEY, 'Bool Parameter', 'A boolean parameter', True))
    params.insert_linkable_parameter(sx.ChoicesParameter(ExampleFilter1.PARAM9_KEY, 'Choices Parameter', 'A choices parameter', 0, ["0", "1", "2"]))
    params.insert(sx.Int32Parameter(ExampleFilter1.PARAM10_KEY, 'Int32 Parameter', 'An Integer Parameter', 42))
    params.link_parameters(ExampleFilter1.PARAM8_KEY, ExampleFilter1.PARAM10_KEY, True)
    params.link_parameters(ExampleFilter1.PARAM9_KEY, ExampleFilter1.PARAM10_KEY, 1)

    return params

  def preflight_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: sx.IFilter.PreflightResult
    """

    input_dir_path: str = [ExampleFilter1.INPUT_DIR_KEY]
    input_file_path: str = [ExampleFilter1.INPUT_FILE_KEY]
    output_dir_path: str = [ExampleFilter1.OUTPUT_DIR_KEY]
    output_file_path: str = [ExampleFilter1.OUTPUT_FILE_KEY]
    param1: float = [ExampleFilter1.PARAM1_KEY]
    param2: bool = [ExampleFilter1.PARAM2_KEY]
    param3: int = [ExampleFilter1.PARAM3_KEY]
    param4: list[str] = [ExampleFilter1.PARAM4_KEY]
    param5: str = [ExampleFilter1.PARAM5_KEY]
    param6: sx.NumericType = [ExampleFilter1.PARAM6_KEY]
    param7: set = [ExampleFilter1.PARAM7_KEY]
    param8: bool = [ExampleFilter1.PARAM8_KEY]
    param9: int = [ExampleFilter1.PARAM9_KEY]
    param10: int = [ExampleFilter1.PARAM10_KEY]
    vec2: list = [ExampleFilter1.VEC2_KEY]
    vec3: list = [ExampleFilter1.VEC3_KEY]
    vec4: list = [ExampleFilter1.VEC4_KEY]
    vec4f: list = [ExampleFilter1.VEC4F_KEY]
    vec6f: list = [ExampleFilter1.VEC6F_KEY]

    message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f'Preflight: {input_dir_path}'))
    return sx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: sx.IFilter.ExecuteResult
    """

    input_dir_path: str = [ExampleFilter1.INPUT_DIR_KEY]
    input_file_path: str = [ExampleFilter1.INPUT_FILE_KEY]
    output_dir_path: str = [ExampleFilter1.OUTPUT_DIR_KEY]
    output_file_path: str = [ExampleFilter1.OUTPUT_FILE_KEY]
    param1: float = [ExampleFilter1.PARAM1_KEY]
    param2: bool = [ExampleFilter1.PARAM2_KEY]
    param3: int = [ExampleFilter1.PARAM3_KEY]
    param4: list[str] = [ExampleFilter1.PARAM4_KEY]
    param5: str = [ExampleFilter1.PARAM5_KEY]
    param6: sx.NumericType = [ExampleFilter1.PARAM6_KEY]
    param7: set = [ExampleFilter1.PARAM7_KEY]
    param8: bool = [ExampleFilter1.PARAM8_KEY]
    param9: int = [ExampleFilter1.PARAM9_KEY]
    param10: int = [ExampleFilter1.PARAM10_KEY]
    vec2: list = [ExampleFilter1.VEC2_KEY]
    vec3: list = [ExampleFilter1.VEC3_KEY]
    vec4: list = [ExampleFilter1.VEC4_KEY]
    vec4f: list = [ExampleFilter1.VEC4F_KEY]
    vec6f: list = [ExampleFilter1.VEC6F_KEY]

    message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f'Execute: {input_dir_path}'))
    return sx.Result()

