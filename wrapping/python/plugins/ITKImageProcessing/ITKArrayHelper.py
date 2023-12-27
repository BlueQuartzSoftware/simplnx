import simplnx as sx
import numpy as np
from typing import List
import itk

k_ImageGeometryDimensionMismatch = -2000
k_ImageComponentDimensionMismatch = -2001

def do_tuples_match(data_array: sx.IDataArray, image_geom: sx.ImageGeom) -> bool:
  num_of_cells = image_geom.num_x_cells * image_geom.num_y_cells * image_geom.num_z_cells
  num_of_tuples = np.prod(data_array.tuple_shape)
  return num_of_cells == num_of_tuples

def do_dimensions_match(data_array: sx.IDataArray, image_geom: sx.ImageGeom) -> bool:
  # Stored fastest to slowest i.e. X Y Z
  geom_dims = image_geom.dimensions

  # Stored slowest to fastest i.e. Z Y X
  image_array_dims = data_array.tuple_shape

  if len(image_array_dims) == 3:
    ordered_array_dims = [image_array_dims[2], image_array_dims[1], image_array_dims[0]]
  elif len(image_array_dims) == 2 and geom_dims[2] == 1:
    ordered_array_dims = [image_array_dims[1], image_array_dims[0], 1]
  else:
    return False

  return (ordered_array_dims == geom_dims)

class ArrayComponentOptions:
  def __init__(self, use_scalar: bool, use_vector: bool, use_rgb_rgba: bool):
    self.using_scalar: bool = use_scalar
    self.using_vector: bool = use_vector
    self.using_rgb_rgba: bool = use_rgb_rgba

    assert not (use_vector and use_rgb_rgba), "ITKArrayHelper: Vector and RGB/RGBA support cannot both be enabled at the same time"

class ArrayTypeOptions:
  def __init__(self, use_int8: bool, use_uint8: bool, use_int16: bool, use_uint16: bool, use_int32: bool, use_uint32: bool, use_int64: bool, use_uint64: bool, use_float32: bool, use_float64: bool):
    self.using_int8: bool = use_int8
    self.using_uint8: bool = use_uint8
    self.using_int16: bool = use_int16
    self.using_uint16: bool = use_uint16
    self.using_int32: bool = use_int32
    self.using_uint32: bool = use_uint32
    self.using_int64: bool = use_int64
    self.using_uint64: bool = use_uint64
    self.using_float32: bool = use_float32
    self.using_float64: bool = use_float64


class ArrayOptions:
  def __init__(self, component_options: ArrayComponentOptions, type_options: ArrayTypeOptions):
    self.component_options: ArrayComponentOptions = component_options
    self.type_options: ArrayTypeOptions = type_options

array_use_all_types = ArrayTypeOptions(True, True, True, True, True, True, True, True, True, True)
array_use_integer_types = ArrayTypeOptions(True, True, True, True, True, True, True, True, False, False)
array_use_floating_types = ArrayTypeOptions(False, False, False, False, False, False, False, False, True, True)
array_use_signed_types = ArrayTypeOptions(True, False, True, False, True, False, True, False, True, True)
array_use_unsigned_types = ArrayTypeOptions(False, True, False, True, False, True, False, True, False, False)

array_use_scalar_only = ArrayComponentOptions(True, False, False)
array_use_vector_only = ArrayComponentOptions(False, True, False)
array_use_rgb_rgba_only = ArrayComponentOptions(False, False, True)
array_use_scalar_vector = ArrayComponentOptions(True, True, False)

scalar_pixel_id_type_list = ArrayOptions(array_use_scalar_only, array_use_all_types)
vector_pixel_id_type_list = ArrayOptions(array_use_vector_only, array_use_all_types)
scalar_vector_pixel_id_type_list = ArrayOptions(array_use_scalar_vector, array_use_all_types)

integer_scalar_pixel_id_type_list = ArrayOptions(array_use_scalar_only, array_use_integer_types)
floating_scalar_pixel_id_type_list = ArrayOptions(array_use_scalar_only, array_use_floating_types)

integer_vector_pixel_id_type_list = ArrayOptions(array_use_vector_only, array_use_integer_types)
floating_vector_pixel_id_type_list = ArrayOptions(array_use_vector_only, array_use_floating_types)

signed_integer_scalar_pixel_id_type_list = ArrayOptions(array_use_scalar_only, array_use_signed_types)

class FunctorBase:
  def __init__(self, input_pixel_type, output_pixel_type, output_data_type: sx.DataType, dimension):
    self.input_pixel_type = input_pixel_type
    self.output_pixel_type = output_pixel_type
    self.output_data_type = output_data_type
    self.dimension = dimension

class DataCheckImplFunctor(FunctorBase):
  def __init__(self, input_pixel_type, output_pixel_type, output_data_type: sx.DataType, dimension):
    super().__init__(input_pixel_type, output_pixel_type, output_data_type, dimension)

  def __call__(self, data_structure, input_array_path, image_geom_path, output_array_path):
    return data_check_impl(self.input_pixel_type, self.output_pixel_type, self.output_data_type, self.dimension, data_structure, input_array_path, image_geom_path, output_array_path)

def array_switch_func_component_impl(input_pixel_type, output_pixel_type, output_data_type: sx.DataType, dimensions: int, component_options: ArrayComponentOptions, result_type, functor: FunctorBase, n_comp, error_code, *args):
  functor.input_pixel_type = input_pixel_type
  functor.output_pixel_type = output_pixel_type
  
  if component_options.using_scalar and n_comp == 1:
      return functor(*args)
  
  if component_options.using_vector:
      if n_comp in [2, 3, 10, 11, 36]:
          return functor(*args)

  if component_options.using_rgb_rgba:
      if n_comp in [3, 4]:
          return functor(*args)
  
  error_message = f"Vector dimension not supported. cDims[0] = {n_comp} Try converting the selected input image to an image with scalar components"
  return sx.Result([sx.Error(error_code, error_message)])


def array_switch_func_dims_impl(input_pixel_type, output_pixel_type, output_data_type: sx.DataType, component_options: ArrayComponentOptions, result_type, functor: FunctorBase, data_array: sx.IDataArray, image_geom: sx.ImageGeom, error_code, *args):
  t_dims = image_geom.dimensions
  c_dims = data_array.component_shape

  n_comp = c_dims[0]

  # If the image dimensions are X Y 1 i.e. 2D
  if t_dims[2] == 1:
    return array_switch_func_component_impl(input_pixel_type, output_pixel_type, output_data_type, 2, component_options, result_type, functor, n_comp, error_code, *args)
  
  return array_switch_func_component_impl(input_pixel_type, output_pixel_type, output_data_type, 3, component_options, result_type, functor, n_comp, error_code, *args)

def get_component_dimensions(type: np.dtype) -> List[int]:
  return [itk.NumericTraits[type].GetLength()]

def data_check_impl(input_pixel_type, output_pixel_type, output_data_type: sx.DataType, data_structure: sx.DataStructure, input_array_path: sx.DataPath, image_geom_path: sx.DataPath, output_array_path: sx.DataPath) -> sx.Result:
  # Assuming get_component_dimensions and get_data_type are functions passed as arguments
  # which correspond to ITK::GetComponentDimensions and GetDataType in the C++ code.

  image_geom: sx.ImageGeom = data_structure[image_geom_path]
  data_array: sx.IDataArray = data_structure[input_array_path]
  data_store = data_array.store

  if not do_dimensions_match(data_array, image_geom) and not do_tuples_match(data_array, image_geom):
    num_of_cells = image_geom.num_x_cells * image_geom.num_y_cells * image_geom.num_z_cells
    num_of_tuples = np.prod(data_array.tuple_shape)
    err_message = f"DataArray '{str(input_array_path)}' tuple dimensions '{', '.join(data_array.tuple_shape)}' do not match Image Geometry '{str(image_geom_path)}' with dimensions 'XYZ={', '.join(image_geom.dimensions)}' and the total number of ImageGeometry Cells {num_of_cells} does not match the total number of DataArray tuples {num_of_tuples}."
    return sx.Result([sx.Error(k_ImageGeometryDimensionMismatch, err_message)])

  c_dims = data_array.component_shape
  input_pixel_dims = get_component_dimensions(input_pixel_type)

  if c_dims != input_pixel_dims:
    err_message = f"DataArray component dimensions of '{', '.join(input_pixel_dims)}' do not match output image component dimensions of '{', '.join(c_dims)}'"
    return sx.Result([sx.Error(k_ImageComponentDimensionMismatch, err_message)])

  output_actions = sx.OutputActions()

  image_dims = image_geom.dimensions
  t_dims = list(reversed(image_dims))
  output_pixel_dims = get_component_dimensions(output_pixel_type)

  output_actions.append_action(sx.CreateArrayAction(output_data_type, t_dims, output_pixel_dims, output_array_path, data_store.get_data_format()))

  return output_actions

def array_switch_func(functor: FunctorBase, array_options: ArrayOptions, result_type, data_array: sx.IDataArray, image_geom: sx.ImageGeom, error_code: int, *args) -> sx.Result:
  type = data_array.data_type
  c_options: ArrayComponentOptions = array_options.component_options

  if array_options.type_options.using_int8 and type == sx.DataType.int8:
    return array_switch_func_dims_impl(itk.SC, itk.SC, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_uint8 and type == sx.DataType.uint8:
    return array_switch_func_dims_impl(itk.UC, itk.UC, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_int16 and type == sx.DataType.int16:
    return array_switch_func_dims_impl(itk.SS, itk.SS, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_uint16 and type == sx.DataType.uint16:
    return array_switch_func_dims_impl(itk.US, itk.US, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_int32 and type == sx.DataType.int32:
    return array_switch_func_dims_impl(itk.SI, itk.SI, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_uint32 and type == sx.DataType.uint32:
    return array_switch_func_dims_impl(itk.UI, itk.UI, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_int64 and type == sx.DataType.int64:
    return array_switch_func_dims_impl(itk.SL, itk.SL, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_uint64 and type == sx.DataType.uint64:
    return array_switch_func_dims_impl(itk.UL, itk.UL, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_float32 and type == sx.DataType.float32:
    return array_switch_func_dims_impl(itk.F, itk.F, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)
  elif array_options.type_options.using_float64 and type == sx.DataType.float64:
    return array_switch_func_dims_impl(itk.D, itk.D, type, c_options, result_type, functor, data_array, image_geom, error_code, *args)

  return sx.Result([sx.Error(-1000, "Invalid DataType while attempting to execute")])

def data_check(array_options: ArrayOptions, data_structure: sx.DataStructure, input_array_path: sx.DataPath, image_geom_path: sx.DataPath, output_array_path: sx.DataPath) -> sx.IFilter.PreflightResult:
  image_geom = data_structure[image_geom_path]
  input_array = data_structure[input_array_path]

  functor = DataCheckImplFunctor()
  result_type = sx.OutputActions()
  return array_switch_func(functor, array_options, result_type, input_array, image_geom, -1, data_structure, input_array_path, image_geom_path, output_array_path)