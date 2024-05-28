.. _Tutorial_3:

==============================================
Tutorial 3: Writing a Custom DREAM3D-NX Filter
==============================================

In this tutorial, we will learn how to write a custom filter for DREAM3D-NX. We will use the `Create Python Plugin and/or Filters` filter to generate the skeleton code and then fill in the `default_tags`, `parameters`, `preflight_impl`, and `execute_impl` methods.

###########################################
3.1 Generating the Filter's Python Skeleton
###########################################

The first step in creating a custom filter is to generate the skeleton code. DREAM3D-NX provides a convenient filter called `Create Python Plugin and/or Filters` to help with this.

Open the DREAM3D-NX application, and add the `Create Python Plugin and/or Filters` filter to the pipeline.

3.1.1 Configuring and Executing the Filter
------------------------------------------

- **Use Existing Plugin**: Checking this checkbox will generate the skeleton code for new filters into an existing plugin.  Leaving the checkbox unchecked will generate skeleton code for a new plugin AND generate skeleton code for filters inside that new plugin.

When Creating a New Plugin (**Use Existing Plugin** is unchecked)
*****************************************************************

- **Name of Plugin**: This field specifies the name of your plugin. This name will be used as the directory name for your plugin and as the identifier for your plugin within the generated skeleton code.

- **Human Name of Plugin**: This is the user-friendly name of your plugin. It can be the same as the `Name of Plugin` or a more descriptive name.  This is the name that will be used when your filter appears in the Filter List.

- **Plugin Output Directory**: This specifies the directory where the generated plugin and filter files will be saved. You can select the directory by clicking the `Save` button.

- **Filter Names (comma-separated)**: This field allows you to specify the programmatic names of the filters you want to create. Each filter name should be separated by a comma.  For example, `FirstFilter,SecondFilter`.

When Using an Existing Plugin (**Use Existing Plugin** is checked)
*****************************************************************

- **Existing Plugin Location**: This specifies the directory of the existing plugin to which you want to add new filters. You can select the directory by clicking the `Browse` button.

- **Filter Names (comma-separated)**: This field allows you to specify the programmatic names of the filters you want to create. Each filter name should be separated by a comma.  For example, `FirstFilter,SecondFilter`.

After configuring these inputs, execute the filter; it will generate the necessary skeleton code for your plugin and filters. The `Generated Plugin File(s)` label in the filter displays the paths to the newly generated plugin files.

3.1.2 Example Configuration
---------------------------

Here is an example configuration of the `Create Python Plugin and/or Filters` filter if the goal is to generate a new plugin and filters:

- **Use Existing Plugin**: Unchecked
- **Name of Plugin**: ExamplePlugin
- **Human Name of Plugin**: Example Plugin
- **Plugin Output Directory**: /tmp
- **Filter Names (comma-separated)**: TestFilter1,TestFilter2

The generated plugin and filter files will be located in the specified output directory, and any existing files with the same names will be overwritten.

Here is an example configuration of the `Create Python Plugin and/or Filters` filter if the goal is to generate filters inside an existing plugin:

- **Use Existing Plugin**: Checked
- **Existing Plugin Location**: /Users/bluequartz/Workspace-NX/simplnx/wrapping/python/plugins/DataAnalysisToolkit
- **Filter Names (comma-separated)**: ReadStlFileFilter

The filter files will be located in the specified plugin directory, and any existing files with the same names will be overwritten.

Let's begin editing the newly generated filter's skeleton methods.  Most developers should only edit the `human_name`, `default_tags`, `parameters`, `preflight_impl`, and `execute_impl` methods in the newly created filter; the rest of the methods should usually remain untouched.

##########################
3.2 Editing the Human Name
##########################

The `human_name` method in a DREAM3D-NX filter returns a user-friendly name for the filter. This name is displayed in the DREAM3D-NX interface, making it easier for users to understand the purpose of the filter. Editing the `human_name` method allows developers to provide a clear and descriptive name that enhances the usability of the filter.

.. code:: python

    def human_name(self) -> str:
        """This returns the name of the filter as a user of DREAM3DNX would see it
        :return: The filter's human name
        :rtype: string
        """
        return 'MyCustomFilter (Python)'

We can update the human name so that it is easier to read:

.. code:: python

    def human_name(self) -> str:
        """This returns the name of the filter as a user of DREAM3DNX would see it
        :return: The filter's human name
        :rtype: string
        """
        return 'My Custom Filter (Python)'

The name can also be updated to display something completely different from the programmatic name.  This human-readable name will be displayed in the DREAM3D-NX interface wherever the filter is listed, making it easy for users to identify the filter.

############################
3.3 Editing the Default Tags
############################

The `default_tags` method allows the filter developer to define a set of tags for the filter. These tags can be used to categorize and search for your filter within the DREAM3D-NX interface.

.. code:: python

    def default_tags(self) -> List[str]:
        """This returns the default tags for this filter
        :return: The default tags for the filter
        :rtype: list
        """
        return ['python', 'MyCustomFilter']

In this example, the `default_tags` method returns a list of tags associated with the filter. These tags can be keywords that describe the filter's functionality, category, or any other relevant information.

We can update the tags to include a few more terms:

.. code:: python

    def default_tags(self) -> List[str]:
        """This returns the default tags for this filter
        :return: The default tags for the filter
        :rtype: list
        """
        return ['python', 'MyCustomFilter', 'test', 'example']

Now when the user searches for `test` or `example` in the DREAM3D-NX interface, this filter will be matched and listed.

##############################
3.4 Defining Filter Parameters
##############################

Next, we need to define the parameters that our filter will accept. These parameters will be used by the filter during its execution. Parameters are essential as they allow users to input values that will be utilized in the filter's logic.

The `parameters` method is where we define the parameters for our filter. Each parameter is given a key, which should be in snake_case, and a descriptive name in ALL_CAPS. This method returns an `nx.Parameters` object that collects these parameters.

.. code:: python

    ARRAY_PATH_KEY = 'output_array_path'
    NUM_TUPLES_KEY = 'num_tuples'

    def parameters(self) -> nx.Parameters:
        """This function defines the parameters that are needed by the filter. Parameters collect the values from the user interface
        and pack them up into a dictionary for use in the preflight and execute methods.
        """
        params = nx.Parameters()

        params.insert(nx.ArrayCreationParameter(MyCustomFilter.ARRAY_PATH_KEY, 'Created Array', 'Array storing the data', nx.DataPath()))

        params.insert(nx.UInt64Parameter(MyCustomFilter.NUM_TUPLES_KEY, 'Num Tuples', 'The number of tuples the array will have', 0))

        return params

In this example, we define two parameters:

1. **Created Array**: This parameter, using the parameter key 'ARRAY_PATH_KEY', is defined using `nx.ArrayCreationParameter`. It specifies the nx.DataPath to the array that will be created to store the data.
2. **Num Tuples**: This parameter, using the parameter key 'NUM_TUPLES_KEY', is defined using `nx.UInt64Parameter`. It provides an unsigned 64-bit integer input that specifies the number of tuples that the newly created array will have.

These parameters will be available when the filter is used, allowing users to provide input values that the filter will utilize during its execution.

#####################################
3.5 Implementing the Preflight Method
#####################################

The `preflight_impl` method is used to perform any necessary checks and setup before the filter is executed. This includes validating input parameters, preparing actions that will modify the data structure, and communicating with the user interface.

.. code:: python

    def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
        """This method preflights the filter and ensures that all inputs are sanity-checked. It validates array sizes if known at preflight time and sets the tuple dimensions of an array when in doubt.
        :returns: Preflight result containing actions, errors, warnings, and preflight values.
        :rtype: nx.IFilter.PreflightResult
        """

        # Write your preflight code here.

Typical operations in the `preflight_impl` method include extracting and validating input parameters, creating data structure actions, adding preflight values, sending messages to the console/interface, and then returning a `PreflightResult` object which contains the actions, errors, warnings, and preflight values generated by the method.

3.5.1 Extracting Input Parameters
---------------------------------

In the above `preflight_impl` method, you can extract the inputs that the user chose for the filter from the args dictionary. This is essential for accessing the necessary data for validation and processing.

.. code:: python

    data_array_path: nx.DataPath = args[MyCustomFilter.ARRAY_PATH_KEY]
    num_tuples: int = args[MyCustomFilter.NUM_TUPLES_KEY]

In this example, `MyCustomFilter.ARRAY_PATH_KEY` and `MyCustomFilter.NUM_TUPLES_KEY` are used to retrieve the path to the data array and the number of tuples, respectively.

3.5.2 Validating Input Parameters
---------------------------------

It is important to validate the inputs that the user chose to ensure they meet the necessary criteria before proceeding. For example, you can check if the number of tuples is greater than zero.

.. code:: python

    if num_tuples <= 0:
        return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-65020, "The number of tuples should be a positive integer.")])

In this example, the filter will return an error message if the number of tuples is a non-positive integer, preventing further execution.

3.5.3 Creating Data Structure Actions
-------------------------------------------------------

You can create actions that will modify the data structure during the preflight phase, such as creating a new array. These actions are appended to the `OutputActions` object.

.. code:: python

    output_actions = nx.OutputActions()
    output_actions.append_action(nx.CreateArrayAction(nx.DataType.float32, [num_tuples], [1], data_array_path))

Here, an action is created to add a new array with the specified DataType, tuple dimensions, and component dimensions at the given nx.DataPath.  The `OutputActions` object will hold any actions that are created by the filter, such as actions to create or modify arrays.


3.5.4 Sending Messages to the User Interface
--------------------------------------------

Optionally, you can send messages to the user interface or console to provide feedback or information about the preflight process.

.. code:: python

    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f"Creating array at: '{data_array_path.to_string('/')}' with {num_tuples} tuples"))

This example sends an informational message indicating the creation of the array with its location and number of tuples.

3.5.5 Adding Preflight Values
-----------------------------

Preflight values are name-value pairs where both the name and the value are strings. These pairs are used to provide descriptive information about the filter's actions or input values.

The name is the title of the information, and the value is the information that the developer wishes to convey.

.. code:: python

    preflight_updated_values: List[nx.IFilter.PreflightValue] = []
    preflight_value = nx.IFilter.PreflightValue()
    preflight_value.name = "Total Array Dimensions"
    preflight_value.value = f"{num_tuples}x{num_components}"
    preflight_updated_values.append(preflight_value)

In this example, we are displaying the name `Created Array Tuple Dimensions` along with the total array dimensions (the number of tuples by the number of components).

3.5.6 Returning the Preflight Result
------------------------------------

Finally, you return the preflight result, which includes the actions to be taken, any errors or warnings, and any preflight values.

.. code:: python

    return nx.IFilter.PreflightResult(output_actions=output_actions, errors=None, warnings=None, preflight_values=preflight_updated_values)

This ensures that any actions created by this filter are reflected in the DataStructure, any errors or warnings are communicated back to the user, and any preflight values are displayed properly in the user interface.

#########################################
3.6 Implementing the Execute Method
#########################################

The `execute_impl` method is used to run the actual filter algorithm and report results. This method performs the main computation and modifications to the data structure, and provides feedback to the user interface.

.. code:: python

    def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
        """ This method actually executes the filter algorithm and reports results.
        :returns: Execution result containing the status of the filter execution.
        :rtype: nx.IFilter.ExecuteResult
        """

        # Write your execute code here

Typical operations in the `execute_impl` method include extracting input parameters, accessing data structure objects (like geometries, groups, arrays, etc), reporting progress to the console/interface, checking whether or not the user pressed cancel during long-running algorithms, and returning an `ExecutionResult` object which contains any errors or warnings generated by the method.

3.6.1 Extracting Input Parameters
---------------------------------

In the `execute_impl` method, you start by extracting input parameters provided by the user interface. This step is similar to the preflight step and ensures you have the necessary input data for processing.

.. code:: python

    data_array_path: nx.DataPath = args[MyCustomFilter.ARRAY_PATH_KEY]
    num_tuples: int = args[MyCustomFilter.NUM_TUPLES_KEY]

These lines retrieve the path to the data array and the number of tuples specified by the user.

3.6.2 Accessing the Data Array
------------------------------

Next, you access the allocated data array from the data structure and then get a numpy view of the array. This allows you to perform efficient computations and modifications directly on the data using numpy.

.. code:: python

    data_array_view = data_structure[data_array_path].npview()

3.6.3 Reporting Progress
------------------------

During execution, you can report progress or provide informational messages to the user interface using the message handler. This keeps users informed about the filter's progress.

.. code:: python

    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Initializing Data Array {str(data_array_path)}...'))

This example sends an informational message telling the user that the data array is being initialized.

3.6.4 Handling Long Running Processes
-------------------------------------

For long-running processes, it's important to check periodically if the user has requested to cancel the filter. This allows for graceful termination, if needed.

.. code:: python

    if not should_cancel:
        return nx.Result()

3.6.5 Returning the Execution Result
------------------------------------

Finally, you return the execution result, which indicates the status of the filter execution.

.. code:: python

    return nx.Result()

Returning the result signifies the completion of the filter's execution; this result can contain errors or warnings that the filter generated during execution.

#########################################
3.7 Full Example: MyCustomFilter.py
#########################################

Let's say that the `Create Python Plugin and/or Filters` filter was used to generate a new DREAM3D-NX Python filter called `MyCustomFilter`.  The contents of the filter will look like the following:

.. code:: python

    from typing import List
    import simplnx as nx

    class MyCustomFilter:
        def uuid(self) -> nx.Uuid:
            """This returns the UUID of the filter. Each filter has a unique UUID value
            :return: The Filter's Uuid value
            :rtype: string
            """
            return nx.Uuid('932d477a-5b4c-4d91-9e40-2b3dca3adbfe')

        def class_name(self) -> str:
            """The returns the name of the class that implements the filter
            :return: The name of the implementation class
            :rtype: string
            """
            return 'MyCustomFilter'

        def name(self) -> str:
            """The returns the name of filter
            :return: The name of the filter
            :rtype: string
            """
            return 'MyCustomFilter'

        def clone(self):
            """Clones the filter
            :return: A new instance of the filter
            :rtype:  MyCustomFilter
            """
            return MyCustomFilter()

        def human_name(self) -> str:
            """This returns the name of the filter as a user of DREAM3DNX would see it
            :return: The filter's human name
            :rtype: string
            """
            return 'My Custom Filter (Python)'
        
        def default_tags(self) -> List[str]:
            """This returns the default tags for this filter
            :return: The default tags for the filter
            :rtype: list
            """
            return ['python', 'MyCustomFilter', 'test', 'example']
        
        
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

            params.insert(nx.ArrayCreationParameter(MyCustomFilter.ARRAY_PATH_KEY, 'Created Array', 'Array storing the data', nx.DataPath()))

            params.insert(nx.UInt64Parameter(MyCustomFilter.NUM_TUPLES_KEY, 'Num Tuples', 'The number of tuples the array will have', 0))

            return params

        def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
            """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
            sizes can be checked if the array sizes are actually known at preflight time. Some filters will not be able to report output
            array sizes during preflight (segmentation filters for example). If in doubt, set the tuple dimensions of an array to [1].
            :returns:
            :rtype: nx.IFilter.PreflightResult
            """

            # Extract the values from the user interface from the 'args' 
            data_array_path: nx.DataPath = args[MyCustomFilter.ARRAY_PATH_KEY]
            num_tuples: int = args[MyCustomFilter.NUM_TUPLES_KEY]
            
            # Create an OutputActions object to hold any DataStructure modifications that we are going to make
            output_actions = nx.OutputActions()

            # Validate that the number of tuples > 0, otherwise return immediately with an error message
            if num_tuples <= 0:
                return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-65020, "The number of tuples should be a positive integer.")])

            # Append a "CreateArrayAction" 
            output_actions.append_action(nx.CreateArrayAction(nx.DataType.float32, [num_tuples], [1], data_array_path))

            # Send back any messages that will appear in the "Output" widget in the UI. This is optional.
            message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f"Creating array at: '{data_array_path.to_string('/')}' with {num_tuples} tuples"))

            # Add a preflight value
            preflight_updated_values: List[nx.IFilter.PreflightValue] = []
            preflight_value = nx.IFilter.PreflightValue()
            preflight_value.name = "Total Array Dimensions"
            preflight_value.value = f"{num_tuples}x{num_components}"
            preflight_updated_values.append(preflight_value)

            # Return the output_actions so the changes are reflected in the User Interface.
            return nx.IFilter.PreflightResult(output_actions=output_actions, errors=None, warnings=None, preflight_values=preflight_updated_values)

        def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
            """ This method actually executes the filter algorithm and reports results.
            :returns:
            :rtype: nx.IFilter.ExecuteResult
            """
            # Extract the values from the user interface from the 'args'
            # This is basically repeated from the preflight because the variables are scoped to the method()
            data_array_path: nx.DataPath = args[MyCustomFilter.ARRAY_PATH_KEY]
            num_tuples: int = args[MyCustomFilter.NUM_TUPLES_KEY]
            
            # At this point the array has been allocated with the proper number of tuples and components. And we can access
            # the data array through a numpy view.
            data_array_view = data_structure[data_array_path].npview()

            # Now you can go off and use numpy or anything else that can use a numpy view to modify the data
            # or use the data in another calculation. Any operation that works on the numpy view in-place
            # has an immediate effect within the DataStructure

            # ---------------------------------------------------------------------------------
            # If you want to send back progress on your filter, you can use the message_handler
            # ---------------------------------------------------------------------------------
            message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Initializing Data Array {str(data_array_path)}...'))

            # ---------------------------------------------------------------------------------------------------
            # If you have a long running process, check the should_cancel to see if the user cancelled the filter
            # ---------------------------------------------------------------------------------------------------
            if not should_cancel:
                return nx.Result()

            return nx.Result()