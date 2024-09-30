
from typing import List

import simplnx as nx

import numpy as np

class NPSortArray:
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  INPUT_ARRAY_KEY = 'input_array_path'

  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('ae37de34-4ead-4202-9a49-6c222cbd88a8')

  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return 'Sort Data Array (Numpy)'

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'NPSortArray'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'NPSortArray'

  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python']

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  NPSortArray
    """
    return NPSortArray()

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = nx.Parameters()

    params.insert(nx.ArraySelectionParameter(NPSortArray.INPUT_ARRAY_KEY, 'Input Data Array', 'The input array to sort.', nx.DataPath(), nx.get_all_data_types()))

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

#    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Preflight: {value}'))
    return nx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """
    # Extract the DataPath from the arguments
    input_data_arr_path: nx.DataPath = args[NPSortArray.INPUT_ARRAY_KEY]
    # Use the DataPath to get a numpy view of the DataArray at that DataPath
    input_array_view = data_structure[input_data_arr_path].npview()

    # Treat the view like a single dimensional array
    # The entire array is sorted from first element to last element. 
    input_array_view = input_array_view.reshape(-1)

    # Sort the actual Data Array in place
    input_array_view.sort()
        
    return nx.Result()


