from typing import List
import simplnx as sx

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

  def uuid(self) -> sx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return sx.Uuid('654236f1-1fc7-4a1f-9ade-5a6206ef4283')

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

  def parameters(self) -> sx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = sx.Parameters()

    params.insert(sx.Parameters.Separator("1st Group of Parameters"))
    params.insert_linkable_parameter(sx.BoolParameter(ExampleFilter2.PARAM7_KEY, 'Bool Parameter', 'Example bool help text', True))
    params.insert_linkable_parameter(sx.ChoicesParameter(ExampleFilter2.PARAM3_KEY, 'Choices Parameter', 'Example choices help text', 0, ["foo", "bar", "baz"]))

    params.insert(sx.Parameters.Separator("2nd Group of Parameters"))
    default_table = [[10, 20], [30, 40]]
    row_info = sx.DynamicTableInfo.DynamicVectorInfo(0, "Row {}")
    col_info = sx.DynamicTableInfo.DynamicVectorInfo(2, "Col {}")
    dynamic_table_info = sx.DynamicTableInfo(sx.DynamicTableInfo.VectorInfo(row_info), sx.DynamicTableInfo.VectorInfo(col_info))
    params.insert(sx.DynamicTableParameter(ExampleFilter2.PARAM13_KEY, 'DynamicTableParameter', 'DynamicTableParameter Example Help Text', default_table, dynamic_table_info))

    params.insert(sx.Parameters.Separator("3rd Group of Parameters"))
    params.insert(sx.MultiPathSelectionParameter(ExampleFilter2.PARAM19_KEY, "Objects to copy", "A list of DataPaths to the DataObjects to be copied", [sx.DataPath(["Small IN100", "Scan Data", "Confidence Index"]), sx.DataPath(["Small IN100", "Scan Data", "Euler Angles"])]))
    params.insert(sx.NeighborListSelectionParameter(ExampleFilter2.PARAM20_KEY, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", sx.DataPath([]), set([sx.DataType.int32])))

    params.insert(sx.Parameters.Separator("Calculator Parameter"))
    calc_param = sx.CalculatorParameter.ValueType( sx.DataPath(["Small IN100","Scan Data"]), "Confidence Index * 10", sx.CalculatorParameter.AngleUnits.Radians)
    params.insert(sx.CalculatorParameter(ExampleFilter2.PARAM18_KEY, "CalculatorParameter", "Example help text for calculator parameter", calc_param))

    params.insert(sx.Parameters.Separator("Required Data Objects"))
    params.insert(sx.DataGroupSelectionParameter(ExampleFilter2.PARAM9_KEY, 'DataGroupSelectionParameter', 'Example data group selection help text', sx.DataPath([]), set([sx.BaseGroup.GroupType.DataGroup])))
    params.insert(sx.DataPathSelectionParameter(ExampleFilter2.PARAM10_KEY, 'DataPathSelectionParameter', 'Example data path selection help text', sx.DataPath([])))
    params.insert(sx.ArraySelectionParameter(ExampleFilter2.PARAM6_KEY, 'Array Selection', 'Example array selection help text', sx.DataPath([]), sx.get_all_data_types(), [[1]]))
    params.insert(sx.GeometrySelectionParameter(ExampleFilter2.PARAM11_KEY, 'GeometrySelectionParameter', 'Example geometry selection help text', sx.DataPath([]), set()))
    params.insert(sx.MultiArraySelectionParameter(ExampleFilter2.PARAM12_KEY, 'MultiArraySelectionParameter', 'Example multiarray selection help text', [], set([sx.IArray.ArrayType.Any]), sx.get_all_data_types(), [[1]]))
    params.insert(sx.AttributeMatrixSelectionParameter(ExampleFilter2.PARAM17_KEY, "Cell Attribute Matrix", "Example attribute matrix selection help text", sx.DataPath([])))

    params.link_parameters(ExampleFilter2.PARAM7_KEY, ExampleFilter2.PARAM9_KEY, True)
    params.link_parameters(ExampleFilter2.PARAM3_KEY, ExampleFilter2.PARAM10_KEY, 0)
    params.link_parameters(ExampleFilter2.PARAM3_KEY, ExampleFilter2.PARAM6_KEY, 1)
    params.link_parameters(ExampleFilter2.PARAM3_KEY, ExampleFilter2.PARAM11_KEY, 2)

    params.insert(sx.Parameters.Separator("Created Data Objects"))
    params.insert(sx.DataGroupCreationParameter(ExampleFilter2.PARAM8_KEY, 'DataGroupCreationParameter', 'Example data group creation help text', sx.DataPath([])))
    params.insert(sx.ArrayCreationParameter(ExampleFilter2.PARAM5_KEY, 'Array Creation', 'Example array creation help text', sx.DataPath([])))

    return params

  def preflight_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: sx.IFilter.PreflightResult
    """

    return sx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: sx.IFilter.ExecuteResult
    """

    return sx.Result()

