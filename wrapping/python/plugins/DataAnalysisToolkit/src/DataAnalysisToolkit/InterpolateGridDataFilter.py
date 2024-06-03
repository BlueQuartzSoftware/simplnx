from typing import List
from scipy.interpolate import griddata
import numpy as np
import simplnx as nx
from enum import Enum
import math

class InterpolateGridDataFilter:
  INPUT_POINTS_ARRAY_PATH = 'input_points_array_path'
  INPUT_DATA_ARRAY_PATH = 'input_data_array_path'
  TARGET_POINTS_ARRAY_PATH = 'target_points_array_path'
  INTERPOLATION_METHOD = 'interpolation_method'
  USE_FILL_VALUE = 'use_fill_value'
  FILL_VALUE = 'fill_value'
  RESCALE_POINTS_TO_UNIT_CUBE = 'rescale_points_to_unit_cube'
  INTERPOLATED_DATA_ARRAY_PATH = 'interpolated_data_array_path'

  class InterpolationMethod(Enum):
    NEAREST = 0
    LINEAR = 1
    CUBIC = 2

  def uuid(self) -> nx.Uuid:
    return nx.Uuid('20f57375-6a0f-46d9-a0fe-59d18c83fb48')

  def human_name(self) -> str:
    return 'Scipy: Interpolate Grid Data (Python)'

  def class_name(self) -> str:
    return 'InterpolateGridDataFilter'

  def name(self) -> str:
    return 'InterpolateGridDataFilter'

  def default_tags(self) -> List[str]:
    return ['python']

  def clone(self):
    return InterpolateGridDataFilter()

  def parameters(self) -> nx.Parameters:
    params = nx.Parameters()

    params.insert(nx.Parameters.Separator("Parameters"))
    params.insert(nx.ChoicesParameter(InterpolateGridDataFilter.INTERPOLATION_METHOD, 'Interpolation Method', 'The method used to interpolate the input data.', InterpolateGridDataFilter.InterpolationMethod.NEAREST.value, ['Nearest', 'Linear', 'Cubic']))
    params.insert_linkable_parameter(nx.BoolParameter(InterpolateGridDataFilter.USE_FILL_VALUE, 'Use Fill Value', 'Determines whether or not to use the Fill Value parameter.', False))
    params.insert(nx.Float32Parameter(InterpolateGridDataFilter.FILL_VALUE, 'Fill Value', 'The value to use for points outside of the convex hull of the input points.  This option has no effect for the "Nearest" interpolation method.', 0.0))
    params.insert(nx.BoolParameter(InterpolateGridDataFilter.RESCALE_POINTS_TO_UNIT_CUBE, 'Rescale Points To Unit Cube', 'Rescale points to unit cube before performing interpolation. This is useful if some of the input dimensions have incommensurable units and differ by many orders of magnitude.', False))
    params.insert(nx.Parameters.Separator("Required Data Objects"))
    params.insert(nx.ArraySelectionParameter(InterpolateGridDataFilter.INPUT_POINTS_ARRAY_PATH, 'Input Points Array', 'This dataset contains coordinates where you have known values.', nx.DataPath(), {nx.DataType.float32, nx.DataType.float64}, [[2], [3]]))
    params.insert(nx.ArraySelectionParameter(InterpolateGridDataFilter.INPUT_DATA_ARRAY_PATH, 'Input Data Array', 'This dataset contains the measurements or data values corresponding to each point in your Input Points array.', nx.DataPath(), nx.get_all_data_types()))
    params.insert(nx.ArraySelectionParameter(InterpolateGridDataFilter.TARGET_POINTS_ARRAY_PATH, 'Target Points Array', 'This dataset contains coordinates where you want to estimate or predict the values based on your input dataset. These points can be, and often are, different from your input points.', nx.DataPath(), {nx.DataType.float32, nx.DataType.float64}, [[2], [3]]))
    params.insert(nx.Parameters.Separator("Created Data Objects"))
    params.insert(nx.ArrayCreationParameter(InterpolateGridDataFilter.INTERPOLATED_DATA_ARRAY_PATH, 'Interpolated Data Array', 'The output array of interpolated values corresponding to each of the target points specified in the Target Points array. This array provides the estimated values at the new points, based on the method of interpolation chosen and the Input Data array provided.', nx.DataPath("Interpolated Data")))

    params.link_parameters(InterpolateGridDataFilter.USE_FILL_VALUE, InterpolateGridDataFilter.FILL_VALUE, True)

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    input_pts_arr_path: nx.DataPath = args[InterpolateGridDataFilter.INPUT_POINTS_ARRAY_PATH]
    input_data_arr_path: nx.DataPath = args[InterpolateGridDataFilter.INPUT_DATA_ARRAY_PATH]
    target_pts_arr_path: nx.DataPath = args[InterpolateGridDataFilter.TARGET_POINTS_ARRAY_PATH]
    interpolation_method: int = args[InterpolateGridDataFilter.INTERPOLATION_METHOD]
    use_fill_value: bool = args[InterpolateGridDataFilter.USE_FILL_VALUE]
    fill_value: float = args[InterpolateGridDataFilter.FILL_VALUE]
    interpolated_data_arr_path: nx.DataPath = args[InterpolateGridDataFilter.INTERPOLATED_DATA_ARRAY_PATH]

    if interpolation_method == InterpolateGridDataFilter.InterpolationMethod.LINEAR and use_fill_value == True:
      return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Warning(200, f'The Fill Value ({fill_value}) will have no effect with the currently selected interpolation method ("Nearest").')])

    input_points_array: nx.IDataArray = data_structure[input_pts_arr_path]
    input_data_array: nx.IDataArray = data_structure[input_data_arr_path]
    target_points_array: nx.IDataArray = data_structure[target_pts_arr_path]

    input_pts_array_size = math.prod(input_points_array.tdims)
    input_data_array_size = math.prod(input_data_array.tdims)
    input_points_array_str = 'x'.join(str(num) for num in input_points_array.tdims)
    input_data_array_str = 'x'.join(str(num) for num in input_data_array.tdims)
    if input_pts_array_size != input_data_array_size:
      return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-1000, f"Array '{str(input_pts_arr_path)}' has tuple dimensions {input_points_array_str} ({input_pts_array_size} tuples) and array '{str(input_data_arr_path)}' has tuple dimensions {input_data_array_str} ({input_data_array_size} tuples).  The total number of tuples for these two arrays should be the same ({input_pts_array_size} != {input_data_array_size}).")])
    
    # data_type: nx.DataType = from_numpy_dtype(input_data_array.dtype)
    # if input_data_array.data_type is None:
    #   return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-1001, f"Array '{str(input_data_arr_path)}' has an unexpected data type ({input_data_array.dtype.name}).")])

    output_actions = nx.OutputActions()
    output_actions.append_action(nx.CreateArrayAction(input_data_array.data_type, target_points_array.tdims, input_data_array.cdims, interpolated_data_arr_path))

    return nx.IFilter.PreflightResult(output_actions)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    input_pts_arr_path: nx.DataPath = args[InterpolateGridDataFilter.INPUT_POINTS_ARRAY_PATH]
    input_data_arr_path: nx.DataPath = args[InterpolateGridDataFilter.INPUT_DATA_ARRAY_PATH]
    target_pts_arr_path: nx.DataPath = args[InterpolateGridDataFilter.TARGET_POINTS_ARRAY_PATH]
    interpolation_method: int = args[InterpolateGridDataFilter.INTERPOLATION_METHOD]
    use_fill_value: bool = args[InterpolateGridDataFilter.USE_FILL_VALUE]
    fill_value: float = args[InterpolateGridDataFilter.FILL_VALUE]
    rescale_pts: bool = args[InterpolateGridDataFilter.RESCALE_POINTS_TO_UNIT_CUBE]
    interpolated_data_arr_path: nx.DataPath = args[InterpolateGridDataFilter.INTERPOLATED_DATA_ARRAY_PATH]

    input_pts_array: nx.IDataArray = data_structure[input_pts_arr_path]
    input_pts_store = input_pts_array.store
    input_pts_view = input_pts_store.npview()

    input_data_array: nx.IDataArray = data_structure[input_data_arr_path]
    input_data_store = input_data_array.store
    input_data_view = input_data_store.npview()

    target_points_array: nx.IDataArray = data_structure[target_pts_arr_path]
    target_points_store = target_points_array.store
    target_points_view = target_points_store.npview()

    interpolated_data_array: nx.IDataArray = data_structure[interpolated_data_arr_path]
    interpolated_data_store = interpolated_data_array.store
    interpolated_data_view = interpolated_data_store.npview()

    method_str = 'nearest'
    if interpolation_method == InterpolateGridDataFilter.InterpolationMethod.LINEAR.value:
      method_str = 'linear'
    elif interpolation_method == InterpolateGridDataFilter.InterpolationMethod.CUBIC.value:
      method_str = 'cubic'
    
    if use_fill_value == False:
      fill_value = np.nan

    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Interpolating Grid Data...'))

    interpolated_data_view[:] = griddata(points=input_pts_view, values=input_data_view, xi=target_points_view, method=method_str, fill_value=fill_value, rescale=rescale_pts)

    return nx.Result()

