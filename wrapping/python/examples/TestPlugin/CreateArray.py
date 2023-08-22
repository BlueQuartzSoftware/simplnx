from typing import List, Union
import complex as cx

def _convert_str_to_num(init_value: str, numeric_type: cx.NumericType) -> Union[int, float, None]:
  try:
    if numeric_type == cx.NumericType.float32 or numeric_type == cx.NumericType.float64:
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

  def uuid(self) -> cx.Uuid:
    return cx.Uuid('14c4ab0e-f7fd-464e-a54b-013346506351')

  def human_name(self) -> str:
    return 'CreateArrayFilter (Python)'

  def class_name(self) -> str:
    return 'CreateArrayFilter'

  def name(self) -> str:
    return 'CreateArrayFilter'

  def default_tags(self) -> List[str]:
    return ['python']

  def clone(self):
    return CreateArrayFilter()

  def parameters(self) -> cx.Parameters:
    params = cx.Parameters()

    params.insert(cx.NumericTypeParameter(CreateArrayFilter.NUMERIC_TYPE_KEY, 'Numeric Type', 'Numeric Type of data to create', cx.NumericType.int32))
    params.insert(cx.StringParameter(CreateArrayFilter.INITILIZATION_VALUE_KEY, 'Initialization Value', 'This value will be used to fill the new array', '0'))
    params.insert(cx.UInt64Parameter(CreateArrayFilter.NUM_COMPS_KEY, 'Number of Components', 'Number of Components', 1))
    params.insert(cx.ArrayCreationParameter(CreateArrayFilter.DATA_PATH_KEY, 'Created Array', 'Array storing the data', cx.DataPath()))
    table_info = cx.DynamicTableInfo()
    table_info.set_rows_info(cx.DynamicTableInfo.StaticVectorInfo(1))
    table_info.set_cols_info(cx.DynamicTableInfo.DynamicVectorInfo(1, 'DIM {}'))
    params.insert(cx.DynamicTableParameter(CreateArrayFilter.TUPLE_DIMS_KEY, 'Data Array Dimensions (Slowest to Fastest Dimensions)', 'Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.', table_info))

    return params

  def preflight_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.PreflightResult:
    numeric_type: cx.NumericType = args[CreateArrayFilter.NUMERIC_TYPE_KEY]
    init_value: str = args[CreateArrayFilter.INITILIZATION_VALUE_KEY]
    num_components: int = args[CreateArrayFilter.NUM_COMPS_KEY]
    data_array_path: cx.DataPath = args[CreateArrayFilter.DATA_PATH_KEY]
    table_data: List[List[float]] = args[CreateArrayFilter.TUPLE_DIMS_KEY]

    if init_value == '':
      return cx.IFilter.PreflightResult(errors=[cx.Error(-123, 'Init Value cannot be empty')])

    if _convert_str_to_num(init_value, numeric_type) is None:
      return cx.IFilter.PreflightResult(errors=[cx.Error(-124, 'Init Value cannot be converted')])

    tuple_dims: List[int] = []
    for value in table_data[0]:
      if value == 0.0:
        return cx.IFilter.PreflightResult(errors=[cx.Error(-125, 'Tuple dimension cannot be zero')])
      tuple_dims.append(int(value))

    output_actions = cx.OutputActions()
    output_actions.append_action(cx.CreateArrayAction(cx.convert_numeric_type_to_data_type(numeric_type), tuple_dims, [num_components], data_array_path))

    return cx.IFilter.PreflightResult(output_actions=output_actions)

  def execute_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.ExecuteResult:
    numeric_type: cx.NumericType = args[CreateArrayFilter.NUMERIC_TYPE_KEY]
    init_value: str = args[CreateArrayFilter.INITILIZATION_VALUE_KEY]
    data_array_path: cx.DataPath = args[CreateArrayFilter.DATA_PATH_KEY]

    value = _convert_str_to_num(init_value, numeric_type)
    if value is None:
      raise RuntimeError(f'Unable to convert init value "{init_value}" to "{numeric_type}"')

    data_array = data_structure[data_array_path]
    data = data_array.store.npview()

    data[:] = value

    return cx.Result()
