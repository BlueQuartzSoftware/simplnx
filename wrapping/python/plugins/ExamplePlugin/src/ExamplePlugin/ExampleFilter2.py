from typing import List
import simplnx as nx

class ExampleFilter2:
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
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
  PARAM16_KEY = "param16"
  PARAM17_KEY = "param17"
  PARAM18_KEY = "param18"
  PARAM19_KEY = "param19"
  PARAM20_KEY = "param20"

  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('654236f1-1fc7-4a1f-9ade-5a6206ef4283')

  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return 'Example Filter 2 (Python)'

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'ExampleFilter2'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'ExampleFilter2'

  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python']

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  ExampleFilter2
    """
    return ExampleFilter2()

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = nx.Parameters()

    params.insert(nx.Parameters.Separator("1st Group of Parameters"))
    params.insert_linkable_parameter(nx.BoolParameter(ExampleFilter2.PARAM7_KEY, 'Bool Parameter', 'Example bool help text', True))
    params.insert_linkable_parameter(nx.ChoicesParameter(ExampleFilter2.PARAM3_KEY, 'Choices Parameter', 'Example choices help text', 0, ["foo", "bar", "baz"]))

    params.insert(nx.Parameters.Separator("2nd Group of Parameters"))
    default_table = [[10, 20], [30, 40]]
    row_info = nx.DynamicTableInfo.DynamicVectorInfo(0, "Row {}")
    col_info = nx.DynamicTableInfo.DynamicVectorInfo(2, "Col {}")
    dynamic_table_info = nx.DynamicTableInfo(nx.DynamicTableInfo.VectorInfo(row_info), nx.DynamicTableInfo.VectorInfo(col_info))
    params.insert(nx.DynamicTableParameter(ExampleFilter2.PARAM13_KEY, 'DynamicTableParameter', 'DynamicTableParameter Example Help Text', default_table, dynamic_table_info))

    params.insert(nx.Parameters.Separator("3rd Group of Parameters"))
    params.insert(nx.MultiPathSelectionParameter(ExampleFilter2.PARAM19_KEY, "Objects to copy", "A list of DataPaths to the DataObjects to be copied", [nx.DataPath(["Small IN100", "Scan Data", "Confidence Index"]), nx.DataPath(["Small IN100", "Scan Data", "Euler Angles"])]))
    params.insert(nx.NeighborListSelectionParameter(ExampleFilter2.PARAM20_KEY, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", nx.DataPath([]), set([nx.DataType.int32])))

    params.insert(nx.Parameters.Separator("Calculator Parameter"))
    calc_param = nx.CalculatorParameter.ValueType( nx.DataPath(["Small IN100","Scan Data"]), "Confidence Index * 10", nx.CalculatorParameter.AngleUnits.Radians)
    params.insert(nx.CalculatorParameter(ExampleFilter2.PARAM18_KEY, "CalculatorParameter", "Example help text for calculator parameter", calc_param))

    params.insert(nx.Parameters.Separator("Required Data Objects"))
    params.insert(nx.DataGroupSelectionParameter(ExampleFilter2.PARAM9_KEY, 'DataGroupSelectionParameter', 'Example data group selection help text', nx.DataPath([]), set([nx.BaseGroup.GroupType.DataGroup])))
    params.insert(nx.DataPathSelectionParameter(ExampleFilter2.PARAM10_KEY, 'DataPathSelectionParameter', 'Example data path selection help text', nx.DataPath([])))
    params.insert(nx.ArraySelectionParameter(ExampleFilter2.PARAM6_KEY, 'Array Selection', 'Example array selection help text', nx.DataPath([]), nx.get_all_data_types(), [[1]]))
    params.insert(nx.GeometrySelectionParameter(ExampleFilter2.PARAM11_KEY, 'GeometrySelectionParameter', 'Example geometry selection help text', nx.DataPath([]), set()))
    params.insert(nx.MultiArraySelectionParameter(ExampleFilter2.PARAM12_KEY, 'MultiArraySelectionParameter', 'Example multiarray selection help text', [], set([nx.IArray.ArrayType.Any]), nx.get_all_data_types(), [[1]]))
    params.insert(nx.AttributeMatrixSelectionParameter(ExampleFilter2.PARAM17_KEY, "Cell Attribute Matrix", "Example attribute matrix selection help text", nx.DataPath([])))

    params.link_parameters(ExampleFilter2.PARAM7_KEY, ExampleFilter2.PARAM9_KEY, True)
    params.link_parameters(ExampleFilter2.PARAM3_KEY, ExampleFilter2.PARAM10_KEY, 0)
    params.link_parameters(ExampleFilter2.PARAM3_KEY, ExampleFilter2.PARAM6_KEY, 1)
    params.link_parameters(ExampleFilter2.PARAM3_KEY, ExampleFilter2.PARAM11_KEY, 2)

    params.insert(nx.Parameters.Separator("Created Data Objects"))
    params.insert(nx.DataGroupCreationParameter(ExampleFilter2.PARAM8_KEY, 'DataGroupCreationParameter', 'Example data group creation help text', nx.DataPath([])))
    params.insert(nx.ArrayCreationParameter(ExampleFilter2.PARAM5_KEY, 'Array Creation', 'Example array creation help text', nx.DataPath([])))

    return params

  def parameters_version(self) -> int:
    return 1

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """

    return nx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """

    return nx.Result()

