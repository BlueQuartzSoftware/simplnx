Writing a New Python Filter
===========================

0. Environment Setup
--------------------

You will need to first setup your python environment. We will be using an Anaconda virtual environment for this tutorial.

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.10
    conda activate nxpython
    conda install -c bluequartzsoftware dream3dnx

Or, if you are installing from a local conda-bld directory, it will be something like:

.. code:: shell

    conda install -c file:///some/path/to/a/conda-build/folder dream3dnx


Wait until the environment is fully installed.

- Next you will want to launch **DREAM3D-NX** from the same Anaconda command prompt.

    **MacOS**

        .. code:: shell

            $(conda info --envs | grep '*' | awk '{print $3}')/bin/DREAM3DNX.app/Contents/MacOS/DREAM3DNX
    
    **Linux**

        .. code:: shell

            dream3dnx

    **Windows**

        .. code:: shell

            dream3dnx.exe

1. Generating the Skeleton Code
-------------------------------

**Generate Python Plugin and/or Python Filters** is a filter in *simplnx* that generates the skeleton code for new Python filters.

This filter can generate skeleton code for the new filters in an existing Python plugin, or it can also generate the skeleton code for a new Python plugin.

**Steps:**

#. **Access the Filter:**
    - Add the **Generate Python Plugin and/or Python Filters** filter to your pipeline.

#. **Configure the Filter:**
    **New Python Plugin**
        .. image:: Images/Generate_Python_Plugin.png

        - Turn OFF *Use Existing Plugin*.
        - Input the programmatic name and human name for the new plugin.
        - Select the output directory for the new plugin.
        - Set the desired programmatic names for your new filters (separated by commas).
    
    **Existing Python Plugin**
        .. image:: Images/Generate_Python_Plugin_2.png

        - Turn ON *Use Existing Plugin*.
        - Select the existing plugin location on the file system (the top-level directory of the existing plugin).
        - Set the desired programmatic names for your new filters (separated by commas).

#. **Generate the Code:**
    #. Run the filter.
        - If *Use Existing Plugin* is OFF, then the new plugin directory structure and new filters will be generated at the specified output directory.
        - If *Use Existing Plugin* is ON, then the new filters will be generated inside the existing plugin at the specified location.

2. Understanding the Plugin Structure
-------------------------------------

Python plugins in *simplnx* contain 2 main files (**__init__.py** and **Plugin.py**) and the filter files.

- **__init__.py:** The plugin initialization module that determines which symbols are to be exported when `from PLUGIN_NAME import *` is used.  Also includes a `get_plugin` method that returns an instance of the plugin (used during the process of discovering and loading Python plugins).

- **Plugin.py:** The main plugin module that contains the plugin's unique id, plugin name, plugin description, plugin's vendor, and the list of filters available in the plugin.

- **Filter Files:** The Python modules that contain the parameter setup, validation, and execution logic for each filter.

3. Understanding the Filter Skeleton Structure
----------------------------------------------

The skeleton provides a basic structure with placeholders and conventions that align with *simplnx*'s architecture.

**Components:**

- **Filter Class:** The main class that represents your filter.
  
    .. code-block:: python

        class FirstFilter:
            # Filter class definition
  
- **Parameter Keys:** Use descriptive constants to define keys for your parameters. These keys will be used to access parameter values from the `args` dictionary in `preflight_impl` and `execute_impl`.
  
    .. code-block:: python

        """
        This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name of the value should be ALL_CAPITOL_KEY
        """
        TEST_KEY = 'test'

- **UUID Method:** This method returns the unique identifier for the new filter.  This unique identifier is automatically generated and should typically not be modified.

    **NOTE:** If you are creating a new filter by copying the contents of an existing filter file (NOT generating a new filter), this unique identifier MUST be modified!  For this case, you can simply call `uuidgen` from the command line and then copy the result into this method.

    .. code-block:: python

        def uuid(self) -> nx.Uuid:
            """This returns the UUID of the filter. Each filter has a unique UUID value
            :return: The Filter's Uuid value
            :rtype: string
            """
            return nx.Uuid('caad34b3-54e3-4276-962e-b59cd88b7320')

- **Human Name Method:** This method returns the human-readable name for the filter.  This name is typically used in the DREAM3D-NX GUI.  It is set, by default, to the programmatic name of the filter and should probably be modified to something more human-readable.

    .. code-block:: python

        def human_name(self) -> str:
            """This returns the name of the filter as a user of DREAM3DNX would see it
            :return: The filter's human name
            :rtype: string
            """
            return 'FirstFilter'    # This could be updated to return 'First Filter' or '1st Filter', or any other human-readable name.

- **Class Name Method:** This method returns the programmatic name for the filter.

    .. code-block:: python

        def class_name(self) -> str:
            """The returns the name of the class that implements the filter
            :return: The name of the implementation class
            :rtype: string
            """
            return 'FirstFilter'

- **Name Method:** This method returns a generic name for the filter.

    .. code-block:: python

        def name(self) -> str:
            """The returns the name of filter
            :return: The name of the filter
            :rtype: string
            """
            return 'FirstFilter'

- **Default Tags Method:** This method returns all the tags that are used to match this filter when searching.  For example if this filter has the tag *Foo*, then any time *Foo* is searched in the Filter List, this filter will match and appear in the search results.  The default tag for Python filters is *python*, but feel free to add more if needed.

    .. code-block:: python

        def default_tags(self) -> List[str]:
            """This returns the default tags for this filter
            :return: The default tags for the filter
            :rtype: list
            """
            return ['python']

- **Clone Method:** This method returns a new instance of the filter.  This method should not be modified.

    .. code-block:: python

        def clone(self):
            """Clones the filter
            :return: A new instance of the filter
            :rtype:  FirstFilter
            """
            return FirstFilter()

- **Parameters Method:** Add *simplnx* filter parameters to this method to configure what inputs are available to users of the filter.

    .. code-block:: python

        def parameters(self) -> nx.Parameters:
            params = nx.Parameters()

            # Add your parameters here

            return params
  
- **Preflight and Execute Methods:** These are crucial methods where your filter's logic will reside.

    .. code-block:: python

        def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
            # Preflight logic

        def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
            # Execution logic

4. Defining Parameters
----------------------
Parameters determine what inputs are available to users; they make your filter configurable and adaptable to different datasets and scenarios.

1. **Define Parameter Keys:**
    - Use descriptive constants to define keys for your parameters. These keys will be used to access parameter values from the `args` dictionary in `preflight_impl` and `execute_impl`.
   
        .. code-block:: python

            OUTPUT_ARRAY_PATH = 'output_array_path_key'
            INIT_VALUE_KEY = 'init_value_key'
            NUM_TUPLES_KEY = 'num_tuples_key'
            NUM_COMPS_KEY = 'num_comps_key'
    
2. **Implement the `parameters` Method:**
    - Create instances of parameter classes provided by *simplnx* and add them to your filter.
   
        .. code-block:: python

            def parameters(self):
                params = nx.Parameters()

                # Create a 'Number of Tuples' input, where the filter's user can input an unsigned 64-bit integer
                params.insert(nx.UInt64Parameter(FirstFilter.NUM_TUPLES_KEY, 'Number of Tuples', 'Number of Tuples', 1))

                # Create a 'Number of Components' input, where the filter's user can input an unsigned 64-bit integer
                params.insert(nx.UInt64Parameter(FirstFilter.NUM_COMPS_KEY, 'Number of Components', 'Number of Components', 1))

                # Create an 'Initialization Value' input, where the filter's user can input the value that will be used to initialize the output array
                params.insert(nx.Float32Parameter(FirstFilter.INIT_VALUE_KEY, 'Initialization Value', 'This value will be used to fill the new array', '0.0'))

                # Create the input that allows the filter's user to pick the path where the output array will be stored in the data structure
                default_output_data_path = nx.DataPath(["Small IN100", "Scan Data", "Output"])
                params.insert(nx.ArrayCreationParameter(FirstFilter.OUTPUT_ARRAY_PATH, 'Array Creation', 'Example array creation help text', default_output_data_path))

                return params
    
    For the full list of parameters and their arguments, please see `Developer_API <Developer_API.html>`__.

    To see examples of how to instantiate each parameter, check out `ExampleFilter1 <https://github.com/BlueQuartzSoftware/simplnx/tree/develop/wrapping/python/plugins/ExamplePlugin/ExampleFilter1.py>`__ and `ExampleFilter2 <https://github.com/BlueQuartzSoftware/simplnx/tree/develop/wrapping/python/plugins/ExamplePlugin/ExampleFilter2.py>`__.

5. Writing the Preflight Implementation
---------------------------------------
The `preflight_impl` method allows you to perform checks, validations, and setup tasks before the filter's main execution.

**Example Preflight Method:**
    This example creates a new 32-bit float output array using the number of tuples, number of components, and output array path provided by the user.  It also validates that the initialization value is not set to 0.

    .. code-block:: python

        def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
            # Retrieve the filter parameter values from the args dictionary using the filter's parameter keys
            num_of_tuples: int = args[FirstFilter.NUM_TUPLES_KEY]
            num_of_components: int = args[FirstFilter.NUM_COMPS_KEY]
            init_value: float = args[FirstFilter.INIT_VALUE_KEY]
            output_array_path: nx.DataPath = args[FirstFilter.OUTPUT_ARRAY_PATH]

            # Return a preflight error if the init value is 0
            if init_value == '0.0':
                return nx.IFilter.PreflightResult(errors=[nx.Error(-123, 'Init Value cannot be 0.')])

            # Create the new output array.  This is done via a CreateArrayAction, which we will create and then append to the output actions.
            # This will create the new output array and add it to the data structure so that it can be used later in the "execute_impl" method.
            output_actions = nx.OutputActions()
            output_actions.append_action(nx.CreateArrayAction(nx.DataType.float32, [num_of_tuples], [num_of_components], output_array_path))

            # Return the output actions
            return nx.IFilter.PreflightResult(output_actions)

**Key Aspects:**

- **Parameter Retrieval and Validation:**
    - Extract and validate the parameters to ensure they meet your filter's requirements.
  
        .. code-block:: python

            init_value: float = args[FirstFilter.INIT_VALUE_KEY]
            if init_value == '0.0':
                return nx.IFilter.PreflightResult(errors=[nx.Error(-123, 'Init Value cannot be 0.')])
    
- **Output Actions Setup:**
    - If your filter creates new data arrays, create and add the CreateArrayActions to the `output_actions` object.
  
        .. code-block:: python

            output_actions = nx.OutputActions()
            output_actions.append_action(nx.CreateArrayAction(nx.DataType.float32, [num_of_tuples], [num_of_components], output_array_path))

6. Writing the Execute Implementation
-------------------------------------
In `execute_impl`, you'll implement the core functionality of your filter.

**Example Execute Method:**
    This example sets the initialization value provided by the user into every index of the newly created output array.

    .. code-block:: python

        def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
            # Retrieve the needed filter parameter values from the args dictionary using the filter's parameter keys
            init_value: float = args[FirstFilter.INIT_VALUE_KEY]
            output_array_path: nx.DataPath = args[FirstFilter.OUTPUT_ARRAY_PATH]

            # Get a reference to the output data array from the data structure
            output_data_array: nx.IDataArray = data_structure[output_array_path]

            # Get a numpy view of the output data array
            data = data_array.npview()

            # Set the init value into every index of the array
            data[:] = init_value

            return nx.Result()

**Key Aspects:**

- **Parameter Retrieval:**
    - Extract the necessary parameters from the args dictionary.
  
        .. code-block:: python

            # Retrieve the needed filter parameter values from the args dictionary using the filter's parameter keys
            init_value: float = args[FirstFilter.INIT_VALUE_KEY]
            output_array_path: nx.DataPath = args[FirstFilter.OUTPUT_ARRAY_PATH]

- **Access Data Arrays/Objects From The Data Structure:**
    - Use DataPaths to get a reference to data arrays and other data objects from the data structure.

        .. code-block:: python

             # Get a reference to the output data array from the data structure
            output_data_array: nx.IDataArray = data_structure[output_array_path]
    
- **Manipulating Data Arrays With Numpy:**
    - Get a numpy view into data arrays and then set values into the arrays using numpy.
  
        .. code-block:: python

            # Get a numpy view of the output data array
            data = data_array.npview()

            # Set the init value into every index of the array
            data[:] = init_value

7. Loading The New Plugin
-------------------------
After you have generated your Python plugin/filters, close the current instance of DREAM3D-NX.  Now, from the same Anaconda command prompt as before:

- Export the `PYTHONPATH` environment variable and have that point to a folder that you will be saving your new plugin into or a folder that holds your currently in-development plugin.  Set the `SIMPLNX_PYTHON_PLUGINS` environment variable to the name of your plugin.  Multiple plugin names should be separated by commas.

    **MacOS/Linux**

        .. code:: shell

            export PYTHONPATH=/path/to/plugin/parent/directory
            export SIMPLNX_PYTHON_PLUGINS=[NAME_OF_PLUGIN]

    **Windows**

        .. code:: shell

            set PYTHONPATH=/path/to/plugin/parent/directory
            set SIMPLNX_PYTHON_PLUGINS=[NAME_OF_PLUGIN]

- Re-launch **DREAM3D-NX** again.

    **MacOS**

        .. code:: shell

            $(conda info --envs | grep '*' | awk '{print $3}')/bin/DREAM3DNX.app/Contents/MacOS/DREAM3DNX
    
    **Linux**

        .. code:: shell

            dream3dnx

    **Windows**

        .. code:: shell

            dream3dnx.exe

You should now see your new filters loaded in the DREAM3D-NX user interface.  Search for your filter's name in the Filter List to verify.

Conclusion
----------
By following this guide, you can create a custom Python filter for *simplnx* that is configurable, follows best practices, and integrates smoothly into data processing pipelines. Remember to thoroughly test your filter with different parameter configurations and datasets to ensure its robustness and correctness.

For more Python filter examples, check out the `ExamplePlugin <https://github.com/BlueQuartzSoftware/simplnx/tree/develop/wrapping/python/plugins/ExamplePlugin>`_.