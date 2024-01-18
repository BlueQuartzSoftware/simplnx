from typing import List
import simplnx as sx

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
    params.insert(sx.NumericTypeParameter(ExampleFilter1.PARAM6_KEY, 'Numeric Type', 'Example numeric type help text', sx.NumericType.int32))

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

