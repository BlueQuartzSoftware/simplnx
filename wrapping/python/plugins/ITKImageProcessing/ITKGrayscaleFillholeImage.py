from typing import List
import complex as cx
import itk
import numpy as np
from ITKArrayHelper import data_check, scalar_pixel_id_type_list

class ITKGrayscaleFillholeImageFilter:
  ARRAY_OPTIONS_TYPE = scalar_pixel_id_type_list
  FULLY_CONNECTED = 'fully_connected'
  SELECTED_IMAGE_GEOM_PATH = 'selected_image_geom_path'
  SELECTED_IMAGE_DATA_PATH = 'selected_image_data_path'
  OUTPUT_IMAGE_DATA_NAME = 'output_image_data_name'

  IMG_GEOM_DIMS_MISMATCH_ERR = -2000
  IMG_COMP_DIMS_MISMATCH_ERR = -2001

  def uuid(self) -> cx.Uuid:
    return cx.Uuid('385c3e88-d8f4-4f9e-bc17-f6104f37003e')

  def human_name(self) -> str:
    return 'ITK: Grayscale Fillhole Image Filter (Python)'

  def class_name(self) -> str:
    return 'ITKGrayscaleFillholeImageFilter'

  def name(self) -> str:
    return 'ITKGrayscaleFillholeImageFilter'

  def default_tags(self) -> List[str]:
    return [self.class_name(), 'python', 'ITKImageProcessing', 'ITKGrayscaleFillholeImage', 'ITKMathematicalMorphology', 'MathematicalMorphology']

  def clone(self):
    return ITKGrayscaleFillholeImageFilter()

  def parameters(self) -> cx.Parameters:
    params = cx.Parameters()

    params.insert(cx.Parameters.Separator('Input Parameters'))
    params.insert(cx.BoolParameter(ITKGrayscaleFillholeImageFilter.FULLY_CONNECTED, 'Fully Connected Components', 'Whether the connected components are defined strictly by face connectivity (False) or by face+edge+vertex connectivity (True). Default is False. For objects that are 1 pixel wide, use True.', False))
    params.insert(cx.Parameters.Separator('Required Input Cell Data'))
    params.insert(cx.GeometrySelectionParameter(ITKGrayscaleFillholeImageFilter.SELECTED_IMAGE_GEOM_PATH, 'Image Geometry', 'Select the Image Geometry Group from the DataStructure.', cx.DataPath('Image Geometry'), [cx.IGeometry.Type.Image]))
    params.insert(cx.ArraySelectionParameter(ITKGrayscaleFillholeImageFilter.SELECTED_IMAGE_DATA_PATH, 'Input Image Data Array', 'The image data that will be processed by this filter.', cx.DataPath())) # complex::ITK::GetScalarPixelAllowedTypes()
    params.insert(cx.Parameters.Separator('Created Cell Data'))
    params.insert(cx.DataObjectNameParameter(ITKGrayscaleFillholeImageFilter.OUTPUT_IMAGE_DATA_NAME, 'Output Image Data Array', 'The result of the processing will be stored in this Data Array.', 'Output Image Data'))

    return params

  def preflight_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.PreflightResult:
    image_geom_path: cx.DataPath = args[ITKGrayscaleFillholeImageFilter.SELECTED_IMAGE_GEOM_PATH]
    selected_input_array_path: cx.DataPath = args[ITKGrayscaleFillholeImageFilter.SELECTED_IMAGE_DATA_PATH]
    output_array_name: str = args[ITKGrayscaleFillholeImageFilter.OUTPUT_IMAGE_DATA_NAME]
    fully_connected: bool = args[ITKGrayscaleFillholeImageFilter.FULLY_CONNECTED]
    output_array_path = selected_input_array_path.get_parent().create_child_path(output_array_name)

    return cx.IFilter.PreflightResult(data_check(ITKGrayscaleFillholeImageFilter.ARRAY_OPTIONS_TYPE, data_structure, selected_input_array_path, image_geom_path, output_array_path))
    
  def execute_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.ExecuteResult:
    # image_geom_path: cx.DataPath = args[ITKGrayscaleFillholeImageFilter.SELECTED_IMAGE_GEOM_PATH]
    # selected_input_array_path: cx.DataPath = args[ITKGrayscaleFillholeImageFilter.SELECTED_IMAGE_DATA_PATH]
    # output_array_name: str = args[ITKGrayscaleFillholeImageFilter.OUTPUT_IMAGE_DATA_NAME]
    # fully_connected: bool = args[ITKGrayscaleFillholeImageFilter.FULLY_CONNECTED]
    # output_array_path = selected_input_array_path.get_parent().create_child_path(output_array_name)

    # selected_input_array: cx.IDataArray = data_structure[selected_input_array_path]
    # selected_input_array_store = selected_input_array.store
    # selected_input_array_view = selected_input_array_store.npview()

    # output_array: cx.IDataArray = data_structure[output_array_path]
    # output_array_store = output_array.store
    # output_array_view = output_array_store.npview()

    # itk_image = itk.image_from_array(selected_input_array_view)
    # filter = itk.GrayscaleFillholeImageFilter.New(itk_image)
    # filter.SetFullyConnected(fully_connected)
    # filter.Update()
    # output_itk_image = filter.GetOutput()
    # output_data = itk.array_from_image(output_itk_image)
    # output_array_view[:] = output_data

    return cx.Result()

