from typing import List, Union
import simplnx as nx

def _convert_str_to_num(init_value: str, numeric_type: nx.NumericType) -> Union[int, float, None]:
  try:
    if numeric_type == nx.NumericType.float32 or numeric_type == nx.NumericType.float64:
      return float(init_value)
    else:
      return int(init_value)
  except:
    return None

class CreateArrayFilter:
  NUMERIC_TYPE_KEY = "numeric_type"
  INITILIZATION_VALUE_KEY = "initialization_value"
  NUM_COMPS_KEY = "component_count"
  DATA_PATH_KEY = "output_data_array"
  TUPLE_DIMS_KEY = "tuple_dimensions"

  def uuid(self) -> nx.Uuid:
    return nx.Uuid('14c4ab0e-f7fd-464e-a54b-013346506351')

  def human_name(self) -> str:
    return 'Create Data Array (Python)'

  def class_name(self) -> str:
    return 'CreateArrayFilter'

  def name(self) -> str:
    return 'CreateArrayFilter'

  def default_tags(self) -> List[str]:
    return ['python']

  def clone(self):
    return CreateArrayFilter()

  def parameters(self) -> nx.Parameters:
    params = nx.Parameters()

    params.insert(nx.NumericTypeParameter(CreateArrayFilter.NUMERIC_TYPE_KEY, 'Numeric Type', 'Numeric Type of data to create', nx.NumericType.int32))
    params.insert(nx.StringParameter(CreateArrayFilter.INITILIZATION_VALUE_KEY, 'Initialization Value', 'This value will be used to fill the new array', '0'))
    params.insert(nx.UInt64Parameter(CreateArrayFilter.NUM_COMPS_KEY, 'Number of Components', 'Number of Components', 1))
    params.insert(nx.ArrayCreationParameter(CreateArrayFilter.DATA_PATH_KEY, 'Created Array', 'Array storing the data', nx.DataPath()))
    table_info = nx.DynamicTableInfo()
    table_info.set_rows_info(nx.DynamicTableInfo.StaticVectorInfo(1))
    table_info.set_cols_info(nx.DynamicTableInfo.DynamicVectorInfo(1, 'DIM {}'))
    params.insert(nx.DynamicTableParameter(CreateArrayFilter.TUPLE_DIMS_KEY, 'Data Array Dimensions (Slowest to Fastest Dimensions)', 'Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.', table_info))

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    numeric_type: nx.NumericType = args[CreateArrayFilter.NUMERIC_TYPE_KEY]
    init_value: str = args[CreateArrayFilter.INITILIZATION_VALUE_KEY]
    num_components: int = args[CreateArrayFilter.NUM_COMPS_KEY]
    data_array_path: nx.DataPath = args[CreateArrayFilter.DATA_PATH_KEY]
    table_data: List[List[float]] = args[CreateArrayFilter.TUPLE_DIMS_KEY]

    if init_value == '':
      return nx.IFilter.PreflightResult(errors=[nx.Error(-123, 'Init Value cannot be empty')])

    if _convert_str_to_num(init_value, numeric_type) is None:
      return nx.IFilter.PreflightResult(errors=[nx.Error(-124, 'Init Value cannot be converted')])

    tuple_dims: List[int] = []
    for value in table_data[0]:
      if value == 0.0:
        return nx.IFilter.PreflightResult(errors=[nx.Error(-125, 'Tuple dimension cannot be zero')])
      tuple_dims.append(int(value))

    output_actions = nx.OutputActions()
    output_actions.append_action(nx.CreateArrayAction(nx.convert_numeric_type_to_data_type(numeric_type), tuple_dims, [num_components], data_array_path))

    return nx.IFilter.PreflightResult(output_actions=output_actions)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    numeric_type: nx.NumericType = args[CreateArrayFilter.NUMERIC_TYPE_KEY]
    init_value: str = args[CreateArrayFilter.INITILIZATION_VALUE_KEY]
    data_array_path: nx.DataPath = args[CreateArrayFilter.DATA_PATH_KEY]

    value = _convert_str_to_num(init_value, numeric_type)
    if value is None:
      raise RuntimeError(f'Unable to convert init value "{init_value}" to "{numeric_type}"')

    data_array = data_structure[data_array_path]
    data = data_array.store.npview()

    data[:] = value

    return nx.Result()

