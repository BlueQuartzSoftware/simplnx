from enum import IntEnum
from typing import List
import simplnx as nx
import numpy as np

def _get_min_max_of(dtype):
  if dtype == np.float32 or dtype == np.float64:
    min_value = np.finfo(dtype).min
    max_value = np.finfo(dtype).max
  else:
    min_value = np.iinfo(dtype).min
    max_value = np.iinfo(dtype).max
  return (min_value, max_value)

class InitializeDataPythonFilter:
  class InitType(IntEnum):
    MANUAL = 0
    RANDOM = 1
    RANDOM_WITH_RANGE = 2
    INDICES = 3

  CELL_ARRAY_PATHS_KEY = 'cell_arrays'
  IMAGE_GEOMETRY_PATH_KEY = 'input_image_geometry_path'
  MIN_POINT_KEY = 'min_point'
  MAX_POINT_KEY = 'max_point'
  INIT_TYPE_KEY = 'init_type'
  INIT_VALUE_KEY = 'init_value'
  INIT_RANGE_KEY = 'init_range'
  INIT_INDICES_OFFSET_KEY = 'indices_offset'

  def uuid(self) -> nx.Uuid:
    return nx.Uuid('7e43c3e1-1f94-4115-9ca4-d41171315d71')

  def human_name(self) -> str:
    return 'Initialize Data (Python)'

  def class_name(self) -> str:
    return 'InitializeDataPythonFilter'

  def name(self) -> str:
    return 'InitializeDataPythonFilter'

  def default_tags(self) -> List[str]:
    return ['python']

  def clone(self):
    return InitializeDataPythonFilter()

  def parameters(self) -> nx.Parameters:
    params = nx.Parameters()

    params.insert(nx.MultiArraySelectionParameter(InitializeDataPythonFilter.CELL_ARRAY_PATHS_KEY, 'Cell Arrays', '', [], {nx.IArray.ArrayType.DataArray}, nx.get_all_data_types(), []))
    params.insert(nx.GeometrySelectionParameter(InitializeDataPythonFilter.IMAGE_GEOMETRY_PATH_KEY, 'Image Geometry', '', nx.DataPath(), {nx.IGeometry.Type.Image}))
    params.insert(nx.VectorUInt64Parameter(InitializeDataPythonFilter.MIN_POINT_KEY, 'Min Point', '', [0, 0, 0], ['X (Column)', 'Y (Row)', 'Z (Plane)']))
    params.insert(nx.VectorUInt64Parameter(InitializeDataPythonFilter.MAX_POINT_KEY, 'Max Point', '', [0, 0, 0], ['X (Column)', 'Y (Row)', 'Z (Plane)']))
    params.insert_linkable_parameter(nx.ChoicesParameter(self.INIT_TYPE_KEY, 'Initialization Type', '', InitializeDataPythonFilter.InitType.MANUAL, ['Manual', 'Random', 'Random With Range', 'Indices']))
    params.insert(nx.Float64Parameter(InitializeDataPythonFilter.INIT_VALUE_KEY, 'Initialization Value', '', 0.0))
    params.insert(nx.VectorFloat64Parameter(InitializeDataPythonFilter.INIT_RANGE_KEY, 'Initialization Range', '', [0.0, 0.0]))
    params.insert(nx.Float64Parameter(InitializeDataPythonFilter.INIT_INDICES_OFFSET_KEY, 'Index offset', '', 0.0))

    params.link_parameters(InitializeDataPythonFilter.INIT_TYPE_KEY, InitializeDataPythonFilter.INIT_VALUE_KEY, InitializeDataPythonFilter.InitType.MANUAL)
    params.link_parameters(InitializeDataPythonFilter.INIT_TYPE_KEY, InitializeDataPythonFilter.INIT_RANGE_KEY, InitializeDataPythonFilter.InitType.RANDOM_WITH_RANGE)
    params.link_parameters(InitializeDataPythonFilter.INIT_TYPE_KEY, InitializeDataPythonFilter.INIT_INDICES_OFFSET_KEY, InitializeDataPythonFilter.InitType.INDICES)

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Preflighting InitializeData'))

    cell_array_paths: List[nx.DataPath] = args[InitializeDataPythonFilter.CELL_ARRAY_PATHS_KEY]
    input_image_geometry_path: nx.DataPath = args[InitializeDataPythonFilter.IMAGE_GEOMETRY_PATH_KEY]
    min_point: List[int] = args[InitializeDataPythonFilter.MIN_POINT_KEY]
    max_point: List[int] = args[InitializeDataPythonFilter.MAX_POINT_KEY]
    init_type: int = args[InitializeDataPythonFilter.INIT_TYPE_KEY]
    init_value: float = args[InitializeDataPythonFilter.INIT_VALUE_KEY]
    init_range: List[float] = args[InitializeDataPythonFilter.INIT_RANGE_KEY]
    init_indices_offset: float = args[InitializeDataPythonFilter.INIT_INDICES_OFFSET_KEY]

    x_min = min_point[0]
    y_min = min_point[1]
    z_min = min_point[2]

    x_max = max_point[0]
    y_max = max_point[1]
    z_max = max_point[2]

    errors = []

    if len(cell_array_paths) == 0:
      return (-5550, 'At least one data array must be selected.')

    if x_max < x_min:
      errors.append(nx.Error(-5551, f'X Max ({x_max}) less than X Min ({x_min})'))

    if y_max < y_min:
      errors.append(nx.Error(-5552, f'Y Max ({y_max}) less than Y Min ({y_min})'))

    if z_max < z_min:
      errors.append(nx.Error(-5553, f'Z Max ({z_max}) less than Z Min ({z_min})'))

    image_geom = data_structure[input_image_geometry_path]

    if x_max > (image_geom.num_x_cells - 1):
      errors.append(nx.Error(-5557, f'The X Max you entered of {x_max} is greater than your Max X Point of {image_geom.num_x_cells - 1}'))

    if y_max > (image_geom.num_y_cells - 1):
      errors.append(nx.Error(-5558, f'The Y Max you entered of {y_max} is greater than your Max Y Point of {image_geom.num_y_cells - 1}'))

    if z_max > (image_geom.num_z_cells - 1):
      errors.append(nx.Error(-5559, f'The Z Max you entered of {z_max} is greater than your Max Z Point of {image_geom.num_z_cells - 1}'))

    image_dims = image_geom.dimensions

    reversed_image_dims = tuple(reversed(image_dims))

    for path in cell_array_paths:
      data_array = data_structure[path]
      if len(data_array.tuple_shape) != len(reversed_image_dims):
        errors.append(nx.Error(-5560, f'DataArray at \'{path}\' does not match dimensions of ImageGeometry at \'{input_image_geometry_path}\''))

      dtype = data_array.dtype
      min_value, max_value = _get_min_max_of(dtype)
      if init_type == InitializeDataPythonFilter.InitType.MANUAL:
        if init_value < min_value or init_value > max_value:
          errors.append(nx.Error(-4000, f'{data_array.name}: The initialization value could not be converted. The valid range is {min_value} to {max_value}'))
      elif init_type == InitializeDataPythonFilter.InitType.RANDOM_WITH_RANGE:
        range_min = init_range[0]
        range_max = init_range[1]
        if range_min > range_max:
          errors.append(nx.Error(-5550, 'Invalid initialization range. Minimum value is larger than maximum value.'))
        if range_min < min_value or range_max > max_value:
          errors.append(nx.Error(-4001, f'{data_array.name}: The initialization range can only be from {min_value} to {max_value}'))
        if range_min == range_max:
          errors.append(nx.Error(-4002, 'The initialization range must have differing values'))
      elif init_type == InitializeDataPythonFilter.InitType.INDICES:
        if init_indices_offset < min_value or init_indices_offset > max_value:
          errors.append(nx.Error(-4000, f'{data_array.name}: The initialization value could not be converted. The valid range is {min_value} to {max_value}'))

    if len(errors) != 0:
      return nx.IFilter.PreflightResult(errors=errors)

    return nx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Executing InitializeData'))

    cell_array_paths: List[nx.DataPath] = args[InitializeDataPythonFilter.CELL_ARRAY_PATHS_KEY]
    min_point: List[int] = args[InitializeDataPythonFilter.MIN_POINT_KEY]
    max_point: List[int] = args[InitializeDataPythonFilter.MAX_POINT_KEY]
    init_type: int = args[InitializeDataPythonFilter.INIT_TYPE_KEY]
    init_value: float = args[InitializeDataPythonFilter.INIT_VALUE_KEY]
    init_range: List[float] = args[InitializeDataPythonFilter.INIT_RANGE_KEY]
    init_indices_offset: float = args[InitializeDataPythonFilter.INIT_INDICES_OFFSET_KEY]

    x_min = min_point[0]
    y_min = min_point[1]
    z_min = min_point[2]

    x_max = max_point[0]
    y_max = max_point[1]
    z_max = max_point[2]

    for path in cell_array_paths:
      data_array = data_structure[path]
      dtype = data_array.dtype
      data = data_array.store.npview()
      data_slice = data[z_min:z_max + 1, y_min:y_max + 1, x_min:x_max + 1]
      if init_type == InitializeDataPythonFilter.InitType.MANUAL:
        data_slice[:] = init_value
      elif init_type == InitializeDataPythonFilter.InitType.INDICES:
        data_slice[:] = np.arange(init_indices_offset, data_slice.size, dtype=dtype).reshape(data_slice.shape)
      else:
        if init_type == InitializeDataPythonFilter.InitType.RANDOM_WITH_RANGE:
          range_min = init_range[0]
          range_max = init_range[1]
        else:
          range_min, range_max = _get_min_max_of(dtype)
        rng = np.random.default_rng()
        if dtype == np.float32 or dtype == np.float64:
          data_slice[:] = rng.uniform(range_min, range_max, size=data_slice.size).reshape(data_slice.shape)
        else:
          data_slice[:] = rng.integers(range_min, range_max, size=data_slice.size).reshape(data_slice.shape)

    return nx.Result()

