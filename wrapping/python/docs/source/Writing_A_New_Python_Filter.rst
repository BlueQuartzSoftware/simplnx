Writing a New Python Filter
===========================

1. Generating the Skeleton Code
-----------------------------------------------------------------
**Generate Python Plugin and/or Python Filters** is a powerful filter in *simplnx* that kickstarts the development of new Python filters by generating the skeleton code for the new filters.

This filter can generate skeleton code for the new filters in an existing Python plugin, or it can also generate the skeleton code for a new Python plugin.

**Steps:**

#. **Access the Filter:**
    - Add the **Generate Python Plugin and/or Python Filters** filter to your pipeline.

#. **Configure the Filter:**
    #. **New Python Plugin**
        .. image:: Images/Generate_Python_Plugin.png
        - Turn OFF *Use Existing Plugin*.
        - Input the programmatic name and human name for the new plugin.
        - Select the output directory for the new plugin.
        - Set the desired programmatic names for your new filters (separated by commas).
    
    #. **Existing Python Plugin**
        .. image:: Images/Generate_Python_Plugin_2.png
        - Turn ON *Use Existing Plugin*.
        - Select the existing plugin location on the file system (the top-level directory of the existing plugin).
        - Set the desired programmatic names for your new filters (separated by commas).

#. **Generate the Code:**
    #. Run the filter.
        - If *Use Existing Plugin* is OFF, then the new plugin directory structure and new filters will be generated.
        - If *Use Existing Plugin* is ON, then the new filters will be generated inside the existing plugin.

2. Understanding the Filter Skeleton Structure
---------------------------------------
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

- **UUID Method:** This method returns the unique identifier for the new filter.  This unique identifier was automatically generated and typically does not need to be modified.

    .. code-block:: python

        def uuid(self) -> nx.Uuid:
            """This returns the UUID of the filter. Each filter has a unique UUID value
            :return: The Filter's Uuid value
            :rtype: string
            """
            return nx.Uuid('caad34b3-54e3-4276-962e-b59cd88b7320')

- **Human Name Method:** This method returns the human-readable name for the filter.  This value is typically used in the DREAM3D-NX GUI.  It is set, by default, to the programmatic name of the filter and should probably be modified to something more human-readable.

    .. code-block:: python

        def human_name(self) -> str:
            """This returns the name of the filter as a user of DREAM3DNX would see it
            :return: The filter's human name
            :rtype: string
            """
            return 'FirstFilter'    # This could be updated to return 'First Filter' or any other human-readable name.

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

- **Default Tags Method:** This method returns all the tags that are used to match with this filter when searching.  For example, if this filter has the tag *Foo* then any time *Foo* is searched in the Filter List, this filter will match and appear in the search results.  The default tag for Python filters is *python*, but feel free to add more if needed.

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

        def parameters(self) -> sx.Parameters:
            params = sx.Parameters()

            # Add your parameters here

            return params
  
- **Preflight and Execute Methods:** These are crucial methods where your filter's logic will reside.

    .. code-block:: python

        def preflight_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.PreflightResult:
            # Preflight logic

        def execute_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.ExecuteResult:
            # Execution logic

3. Defining Parameters
----------------------
Parameters determine what inputs are available to users; they make your filter configurable and adaptable to different datasets and scenarios.

1. **Define Parameter Keys:**
    - Use descriptive constants to define keys for your parameters. These keys will be used to access parameter values from the `args` dictionary in `preflight_impl` and `execute_impl`.
   
        .. code-block:: python

            MY_PARAMETER_KEY = 'my_parameter'
            INPUT_ARRAY_PATH = 'input_array_path'
            OUTPUT_ARRAY_PATH = 'output_array_path'
    
2. **Implement the `parameters` Method:**
    - Create instances of parameter classes provided by *simplnx* and add them to your filter.
   
        .. code-block:: python

            def parameters(self):
                params = sx.Parameters()

                params.insert(sx.Float32Parameter(FirstFilter.MY_PARAMETER_KEY, 'My Parameter', 'Description of my parameter', 1.0))

                default_input_data_path = sx.DataPath(["Small IN100", "Scan Data", "Data"])
                params.insert(sx.ArraySelectionParameter(FirstFilter.INPUT_ARRAY_PATH, 'Array Selection', 'Example array selection help text', default_input_data_path, sx.get_all_data_types(), [[1]]))

                default_output_data_path = sx.DataPath(["Small IN100", "Scan Data", "Data"])
                params.insert(sx.ArrayCreationParameter(FirstFilter.OUTPUT_ARRAY_PATH, 'Array Creation', 'Example array creation help text', default_output_data_path))

                return params
    
    For the full list of parameters and their arguments, please see `Developer_API <Developer_API.html>`__.

4. Writing the Preflight Implementation
-------------------------------------------------------
The `preflight_impl` method allows you to perform checks, validations, and setup tasks before the filter's main execution.

**Key Aspects:**

- **Parameter Retrieval and Validation:**
  - Extract and validate the parameters to ensure they meet your filter's requirements.
  
    .. code-block:: python

       my_param = args[FirstFilter.MY_PARAMETER_KEY]
       if my_param < 0:
           raise ValueError('My Parameter must be positive')
        
        input_arr_path: sx.DataPath = args[FirstFilter.INPUT_ARRAY_PATH]
        output_arr_path: sx.DataPath = args[FirstFilter.OUTPUT_ARRAY_PATH]

- **Access Data Arrays/Objects From The Data Structure:**
    - Use DataPaths to get a reference to data arrays and other data objects from the data structure.

        .. code-block:: python

            input_array: sx.IDataArray = data_structure[input_arr_path]
            output_array: sx.IDataArray = data_structure[output_arr_path]
    
- **Output Actions Setup:**
  - If your filter generates new data arrays, define their structure and add the creation actions to the `output_actions` object.
  
    .. code-block:: python

       output_actions = sx.OutputActions()
       output_actions.append_action(sx.CreateArrayAction(data_type, dimensions, component_dimensions, output_path))

- **Example Preflight Method:**

    .. code-block:: python

        def preflight_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.PreflightResult:
            # Retrieve the filter parameter values from the args dictionary using the filter's parameter keys
            input_pts_arr_path: sx.DataPath = args[InterpolateGridDataFilter.INPUT_POINTS_ARRAY_PATH]
            input_data_arr_path: sx.DataPath = args[InterpolateGridDataFilter.INPUT_DATA_ARRAY_PATH]
            target_pts_arr_path: sx.DataPath = args[InterpolateGridDataFilter.TARGET_POINTS_ARRAY_PATH]
            interpolation_method: int = args[InterpolateGridDataFilter.INTERPOLATION_METHOD]
            use_fill_value: bool = args[InterpolateGridDataFilter.USE_FILL_VALUE]
            fill_value: float = args[InterpolateGridDataFilter.FILL_VALUE]
            interpolated_data_arr_path: sx.DataPath = args[InterpolateGridDataFilter.INTERPOLATED_DATA_ARRAY_PATH]

            # Return a preflight warning if the interpolation method is set to Linear and Use Fill Value is also turned on
            if interpolation_method == InterpolateGridDataFilter.InterpolationMethod.LINEAR and use_fill_value == True:
                return sx.IFilter.PreflightResult(sx.OutputActions(), [sx.Warning(200, f'The Fill Value ({fill_value}) will have no effect with the currently selected interpolation method ("Nearest").')])

            # Access data arrays from the data structure using DataPaths
            input_points_array: sx.IDataArray = data_structure[input_pts_arr_path]
            input_data_array: sx.IDataArray = data_structure[input_data_arr_path]
            target_points_array: sx.IDataArray = data_structure[target_pts_arr_path]

            # Return a preflight error if the "Input Pts" and "Input Data" arrays do not have the same number of total tuples
            input_pts_array_size = math.prod(input_points_array.tdims)
            input_data_array_size = math.prod(input_data_array.tdims)
            input_points_array_str = 'x'.join(str(num) for num in input_points_array.tdims)
            input_data_array_str = 'x'.join(str(num) for num in input_data_array.tdims)
            if input_pts_array_size != input_data_array_size:
            return sx.IFilter.PreflightResult(sx.OutputActions(), [sx.Error(-1000, f"Array '{str(input_pts_arr_path)}' has tuple dimensions {input_points_array_str} ({input_pts_array_size} tuples) and array '{str(input_data_arr_path)}' has tuple dimensions {input_data_array_str} ({input_data_array_size} tuples).  The total number of tuples for these two arrays should be the same ({input_pts_array_size} != {input_data_array_size}).")])
        
            # Create the new Interpolated Data array.  This is done via a CreateArrayAction, which will create the array and add it to the data structure.
            output_actions = sx.OutputActions()
            output_actions.append_action(sx.CreateArrayAction(input_data_array.data_type, target_points_array.tdims, input_data_array.cdims, interpolated_data_arr_path))

            # Return the output actions
            return sx.IFilter.PreflightResult(output_actions)

5. Writing the Execute Implementation
---------------------------------------------------
In `execute_impl`, you'll implement the core functionality of your filter.

**Key Aspects:**

- **Accessing Data Arrays:**
  - Use the paths from the parameters to access the necessary data arrays from the `data_structure`.
  
    .. code-block:: python

       input_array = data_structure[args[MyCustomFilter.INPUT_ARRAY_PATH]]
       output_array = data_structure[args[MyCustomFilter.OUTPUT_ARRAY_PATH]]
    
- **Implementing the Filter Algorithm:**
  - Apply your filter's core logic to the input data and store the results in the output arrays.
  
    .. code-block:: python

       # Example: Simple processing logic
       output_array[:] = input_array * my_param
    
- **Storing the Results:**
  - Ensure that the results of your processing are correctly stored in the output arrays or any other specified data structure.
  
    .. code-block:: python

       data_structure[args[MyCustomFilter.OUTPUT_ARRAY_PATH]] = output_array

Conclusion
----------
By following this guide, you can create a custom Python filter for *simplnx* that is configurable, follows best practices, and integrates smoothly into data processing pipelines. Remember to thoroughly test your filter with different parameter configurations and datasets to ensure its robustness and correctness.
