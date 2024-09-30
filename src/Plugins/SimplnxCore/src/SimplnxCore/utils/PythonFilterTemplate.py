from typing import List
import simplnx as nx

class #PYTHON_FILTER_NAME#:

# -----------------------------------------------------------------------------
# These methods should not be edited
# -----------------------------------------------------------------------------
  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('#PYTHON_FILTER_UUID#')

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return '#PYTHON_FILTER_NAME#'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return '#PYTHON_FILTER_NAME#'

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  #PYTHON_FILTER_NAME#
    """
    return #PYTHON_FILTER_NAME#()

# -----------------------------------------------------------------------------
# These methods CAN (and probably should) be updated. For instance, the 
# human_name() is what users of the filter will see in the DREAM3D-NX GUI. You
# might want to consider putting spaces between workd, using proper capitalization
# and putting "(Python)" at the end of the name (or beginning if you want the 
# filter list to group your filters togther)
# -----------------------------------------------------------------------------
  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return '#PYTHON_FILTER_HUMAN_NAME# (Python)'
 
  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python', '#PYTHON_FILTER_HUMAN_NAME#']
  
  
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  ARRAY_PATH_KEY = 'output_array_path'
  NUM_TUPLES_KEY = 'num_tuples'

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user interface
    and pack them up into a dictionary for use in the preflight and execute methods.
    """
    params = nx.Parameters()

    params.insert(nx.ArrayCreationParameter(#PYTHON_FILTER_NAME#.ARRAY_PATH_KEY, 'Created Array', 'Array storing the data', nx.DataPath()))

    params.insert(nx.UInt64Parameter(#PYTHON_FILTER_NAME#.NUM_TUPLES_KEY, 'Num Tuples', 'The number of tuples the array will have', 0))

    return params

  def parameters_version(self) -> int:
    return 1

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the array sizes are actually known at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example). If in doubt, set the tuple dimensions of an array to [1].
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """

    # Extract the values from the user interface from the 'args' 
    data_array_path: nx.DataPath = args[#PYTHON_FILTER_NAME#.ARRAY_PATH_KEY]
    num_tuples: int = args[#PYTHON_FILTER_NAME#.NUM_TUPLES_KEY]
      
    # Create an OutputActions object to hold any DataStructure modifications that we are going to make
    output_actions = nx.OutputActions()
    
    # Create the Errors and Warnings Lists to commuicate back to the user if anything has gone wrong
    # errors = []
    # warnings = []
    # preflight_values = []

    # Validate that the number of tuples > 0, otherwise return immediately with an error message
    if num_tuples == 0:
      return nx.IFilter.PreflightResult(errors=[nx.Error(-65020, "The number of tuples should be at least 1.")])

    # Append a "CreateArrayAction" 
    output_actions.append_action(nx.CreateArrayAction(nx.DataType.float32, [num_tuples], [1], data_array_path))

    # Send back any messages that will appear in the "Output" widget in the UI. This is optional.
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f"Creating array at: '{data_array_path.to_string('/')}' with {num_tuples} tuples"))

    # Return the output_actions so the changes are reflected in the User Interface.
    return nx.IFilter.PreflightResult(output_actions=output_actions, errors=None, warnings=None, preflight_values=None)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """
    # Extract the values from the user interface from the 'args'
    # This is basically repeated from the preflight because the variables are scoped to the method()
    data_array_path: nx.DataPath = args[#PYTHON_FILTER_NAME#.ARRAY_PATH_KEY]
    num_tuples: int = args[#PYTHON_FILTER_NAME#.NUM_TUPLES_KEY]
    
    # At this point the array has been allocated with the proper number of tuples and components. And we can access
    # the data array through a numpy view.
    data_array_view = data_structure[data_array_path].npview()
    # Now you can go off and use numpy or anything else that can use a numpy view to modify the data
    # or use the data in another calculation. Any operation that works on the numpy view in-place
    # has an immediate effect within the DataStructure

    # -----------------------------------------------------------------------------
    # If you want to send back progress on your filter, you can use the message_handler
    # -----------------------------------------------------------------------------
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Information Message: Num_Tuples = {num_tuples}'))

    # -----------------------------------------------------------------------------
    # If you have a long running process, check the should_cancel to see if the user cancelled the filter
    # -----------------------------------------------------------------------------
    if not should_cancel:
      return nx.Result()


    return nx.Result()




