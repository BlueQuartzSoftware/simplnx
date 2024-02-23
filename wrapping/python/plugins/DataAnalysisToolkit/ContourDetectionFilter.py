import simplnx as nx
import numpy as np
import cv2
from typing import List

class ContourDetectionFilter:
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  INPUT_IMAGE_ARRAY_KEY = 'input_image_array'
  FEATURE_ATTR_MATRIX_PATH_KEY = 'feature_attr_matrix_path'
  CONTOUR_PERIMETERS_ARRAY_NAME_KEY = 'contour_perimeters_array_name'
  CONTOUR_AREAS_ARRAY_NAME_KEY = 'contour_areas_array_name'
  THRESHOLD_VALUE_KEY = 'threshold_value'
  FEATURE_ID_ARRAY_NAME_KEY = 'feature_id_array_name'

  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('d042f4c3-cd46-4854-aa4a-690e26d92a22')

  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return 'Contour Detection Filter (Python)'

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'ContourDetectionFilter'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'ContourDetectionFilter'

  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python', 'contour', 'detection', 'opencv']

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  ContourDetectionFilter
    """
    return ContourDetectionFilter()

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = nx.Parameters()

    params.insert(nx.Parameters.Separator("Required Data Objects"))
    params.insert(nx.ArraySelectionParameter(ContourDetectionFilter.INPUT_IMAGE_ARRAY_KEY, 'Input Image Data Array', 'The input binary image array that will be used to detect contours.', nx.DataPath([]), set([nx.DataType.uint8]), [[1]]))
    params.insert(nx.Float64Parameter(ContourDetectionFilter.THRESHOLD_VALUE_KEY, 'Threshold Value', "", 50.0))

    params.insert(nx.Parameters.Separator("Created Data Objects"))
    params.insert(nx.DataObjectNameParameter(ContourDetectionFilter.FEATURE_ID_ARRAY_NAME_KEY, 'Feature IDs', 'FeatureID Data Array Name', "FeatureIDs"))
    params.insert(nx.DataGroupCreationParameter(ContourDetectionFilter.FEATURE_ATTR_MATRIX_PATH_KEY, 'Contour Statistics Attribute Matrix Path', 'The path where the Feature Attribute Matrix will be created to store the contour statistics.', nx.DataPath([])))
    params.insert(nx.DataObjectNameParameter(ContourDetectionFilter.CONTOUR_PERIMETERS_ARRAY_NAME_KEY, "Contour Perimeters Array Name", "The name of the Feature Data Array created to store the contour perimeters.", "Contour Perimeters"))
    params.insert(nx.DataObjectNameParameter(ContourDetectionFilter.CONTOUR_AREAS_ARRAY_NAME_KEY, "Contour Areas Array Name", "The name of the Feature Data Array created to store the contour areas.", "Contour Areas"))
  
    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """
    # warnings = []
    input_image_array_path: nx.DataPath = args[ContourDetectionFilter.INPUT_IMAGE_ARRAY_KEY]
    # feature_id_array_name: str = args[ContourDetectionFilter.FEATURE_ID_ARRAY_NAME_KEY]  
    # feature_id_array_path = str(input_image_array_path).split('/')
    # feature_id_array_path[-1] = feature_id_array_name
    

    #warnings.append(nx.Warning(200, f"{feature_id_array_path}"))
    #feature_id_array_path = nx.DataPath(feature_id_array_path)

    feature_attr_matrix_path: nx.DataPath = args[ContourDetectionFilter.FEATURE_ATTR_MATRIX_PATH_KEY]
    perimeters_array_name: str = args[ContourDetectionFilter.CONTOUR_PERIMETERS_ARRAY_NAME_KEY]
    areas_array_name: str = args[ContourDetectionFilter.CONTOUR_AREAS_ARRAY_NAME_KEY]

    threshold_value: float = args[ContourDetectionFilter.THRESHOLD_VALUE_KEY]
    if threshold_value < 0 or threshold_value > 255: 
      return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-1000, f"Threshold value needs to be between 0 and 255")])

    output_actions = nx.OutputActions()

    # We don't know how many contours there are yet, so set the attribute matrix dimensions to (0)
    output_actions.append_action(nx.CreateAttributeMatrixAction(feature_attr_matrix_path, [10]))

    # Create the perimeters feature array
    perimeters_array_path = feature_attr_matrix_path.create_child_path(perimeters_array_name)
    output_actions.append_action(nx.CreateArrayAction(nx.DataType.float64, [10], [1], perimeters_array_path))

    # Create the areas feature array
    areas_array_path = feature_attr_matrix_path.create_child_path(areas_array_name)
    output_actions.append_action(nx.CreateArrayAction(nx.DataType.uint64, [10], [1], areas_array_path))
  
    # Create teh feature ID array in cell data 
    input_data_array = data_structure[input_image_array_path]
    output_actions.append_action(nx.CreateArrayAction(nx.DataType.int32, input_data_array.tdims, [1], nx.DataPath(["ImageDataContainer", "Cell Data", "FeatureIDs"])))


    return nx.IFilter.PreflightResult(output_actions)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """
    input_image_array_path: nx.DataPath = args[ContourDetectionFilter.INPUT_IMAGE_ARRAY_KEY]
    feature_attr_matrix_path: nx.DataPath = args[ContourDetectionFilter.FEATURE_ATTR_MATRIX_PATH_KEY]
    perimeters_array_name: str = args[ContourDetectionFilter.CONTOUR_PERIMETERS_ARRAY_NAME_KEY]
    areas_array_name: str = args[ContourDetectionFilter.CONTOUR_AREAS_ARRAY_NAME_KEY]
    threshold_value: float = args[ContourDetectionFilter.THRESHOLD_VALUE_KEY]

    perimeters_array_path = feature_attr_matrix_path.create_child_path(perimeters_array_name)
    areas_array_path = feature_attr_matrix_path.create_child_path(areas_array_name)

    # Get a reference to the input image array from the data structure
    input_image_array: nx.IDataArray = data_structure[input_image_array_path]

    # Get a numpy view of the input image array
    input_image_data: np.array = input_image_array.npview()

    # Remove any extraneous dimensions of size 1
    input_image_data = np.squeeze(input_image_data)

    # Convert the input image from BGR color space to grayscale
    # imgray = cv2.cvtColor(input_image_data, cv2.COLOR_BGR2GRAY)

    # Apply a binary threshold to the grayscale image.
    # Pixels with intensity greater than 50 are set to the maximum value (255, white), and pixels with intensity less than or equal to 50 are set to 0 (black).
    # This operation converts the grayscale image into a binary image, enhancing the contrast between objects of interest and the background, facilitating further analysis like contour detection.
    ret, thresh = cv2.threshold(input_image_data, threshold_value, 255, cv2.THRESH_BINARY)

    # Find the contours
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, 'Finding contours...'))
    contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE)
    contour_count = len(contours)
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'{contour_count} contours found.'))

    # Get a reference to the feature attribute matrix and feature arrays from the data structure
    feature_attr_matrix: nx.AttributeMatrix = data_structure[feature_attr_matrix_path]
    perimeters_array: nx.IDataArray = data_structure[perimeters_array_path]
    areas_array: nx.IDataArray = data_structure[areas_array_path]

    feature_id_array = data_structure[nx.DataPath(["ImageDataContainer", "Cell Data", "FeatureIDs"])].npview()
    feature_id_array = np.squeeze(feature_id_array)

    # Iterate over contours and draw each one with a unique label
    for i, contour in enumerate(contours):
    # Use the contour index as a label (ensure it's not zero to distinguish from background)
    # Note: i+1 is used because 0 label is for background
      cv2.drawContours(feature_id_array, [contour], -1, color=(i), thickness=cv2.FILLED)

    # Resize the feature attribute matrix and feature arrays
    # feature_attr_matrix.resize_tuples([contour_count])
    # perimeters_array.resize_tuples([contour_count])
    # areas_array.resize_tuples([contour_count])

    # Get numpy views of the feature arrays
    perimeters_data = perimeters_array.npview()
    areas_data = areas_array.npview()

    # Do the calculations and copy the data into the feature arrays
    # The data needs to be reshaped because the simplnx data array is expecting dimensions with a trailing 1.
    perimeters_data[:] = np.array([cv2.arcLength(contour, True) for contour in contours]).reshape((contour_count,1))
    areas_data[:] = np.array([cv2.contourArea(contour) for contour in contours]).reshape((contour_count,1))

    # for i in range(contour_count):
    #     contour = contours[i]
    #     message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Contour {i + 1}/{contour_count} - Perimeter: {cv2.arcLength(contour, True)}'))
    #     message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Contour {i + 1}/{contour_count} - Area: {cv2.contourArea(contour)}'))

    return nx.Result()