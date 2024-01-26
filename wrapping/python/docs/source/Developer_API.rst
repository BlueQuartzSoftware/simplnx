Filter Parameter Classes
========================

.. _Result:
.. py:class:: Result

   The object that encapsulates any warnings or errors from either preflighting or executing a simplnx.Filter object.
   It can be queried for the list of errors or warnings and thus printed if needed.

   .. code:: python

      result = cxor.ConvertOrientations.execute(data_structure=data_structure,
                                          input_orientation_array_path=array_path,
                                          input_type=0,
                                          output_orientation_array_name='Quaternions',
                                          output_type=2)
      if len(result.errors) != 0:
         print('Errors: {}', result.errors)
         print('Warnings: {}', result.warnings)
      else:
         print("No errors running the ConvertOrientations")

General Parameters 
------------------

.. _ArrayCreationParameter:
.. py:class:: ArrayCreationParameter


   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ArrayCreationParameter(name: str, human_name: str, help_text: str, default_value: DataPath)

   Description
   ~~~~~~~~~~~
   The ``ArrayCreationParameter`` is used to specify the creation of an array within the data structure. 

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter. This is the name used internally by the system to identify the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter. This is the name presented to users and is meant to be more descriptive and user-friendly.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter. This text is intended to guide the user in understanding the purpose of the parameter and how to use it.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default value for the parameter. For the `ArrayCreationParameter`, this is a :ref:`DataPath<DataPath>` object that points to the location within the data structure where the data array will be created.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code:: python
      
      import simplnx as sx
      
      data_path = sx.DataPath(["Small IN100", "Scan Data", "Data"])
      params.insert(sx.ArrayCreationParameter(ExampleFilter2.PARAM5_KEY, 'Array Creation', 'Example array creation help text', data_path))

.. _ArraySelectionParameter:
.. py:class:: ArraySelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ArraySelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[DataType], required_comps: List[List[int]] = ..., location: ArraySelectionParameter.DataLocation = ...)

   Description
   ~~~~~~~~~~~
   The ``ArraySelectionParameter`` is used for selecting an existing array within the data structure based on certain criteria.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default DataPath pointing to the array to be selected.
      - **Type**: `DataPath`

   - ``allowed_types`` : Set[DataType]
      - **Description**: Set of allowed data types for the array.
      - **Type**: Set of `DataType`

   - ``required_comps`` : List[List[int]]
      - **Description**: List of component structures that are required.
      - **Type**: List of lists of integers

   - ``location`` : ArraySelectionParameter.DataLocation
      - **Description**: Location of the array (e.g., on nodes, cells, etc.).
      - **Type**: `ArraySelectionParameter.DataLocation`

   Usage
   ~~~~~~

   .. code:: python

      import simplnx as sx

      data_path = sx.DataPath(["Small IN100", "Scan Data", "Data"])
      params.insert(sx.ArraySelectionParameter(ExampleFilter2.PARAM6_KEY, 'Array Selection', 'Example array selection help text', data_path, sx.get_all_data_types(), [[1]]))

.. _ArrayThresholdsParameter:
.. py:class:: ArrayThresholdsParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ArrayThresholdsParameter(name: str, human_name: str, help_text: str, default_value: ArrayThresholdSet, required_comps: List[List[int]] = ...)

   Description
   ~~~~~~~~~~~
   The ``ArrayThresholdsParameter`` is used to specify thresholds for an array, allowing for filtering based on those thresholds.
   
   This parameter holds a ArrayThresholdSet_ object and is used specifically for the :ref:`simplnx.MultiThresholdObjects() <MultiThresholdObjects>` filter.
   This parameter should not be directly invoked but instead its ArrayThresholdSet_ is invoked and used.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : ArrayThresholdSet
      - **Description**: The default set of thresholds for the array.
      - **Type**: `ArrayThresholdSet`

   - ``required_comps`` : List[List[int]]
      - **Description**: List of component structures that are required.
      - **Type**: List of lists of integers
   

   .. py:class:: ArrayThreshold

      Represents a single threshold, including the comparison type, array path, and the threshold value.

      - ``array_path`` : DataPath
         - **Description**: Path to the data array.
         - **Type**: `DataPath`

      - ``comparison`` : ArrayThreshold.ComparisonType
         - **Description**: Type of comparison to perform (Equal, GreaterThan, LessThan, NotEqual).
         - **Type**: `ArrayThreshold.ComparisonType`

      - ``value`` : float
         - **Description**: The threshold value.
         - **Type**: float

   .. py:class:: ArrayThreshold.ComparisonType

      Defines the types of comparisons that can be used in an `ArrayThreshold`.

      - ``Equal``, ``GreaterThan``, ``LessThan``, ``NotEqual``
         - **Description**: Types of comparison.
         - **Type**: Enum (int)

   .. py:class:: ArrayThresholdSet

      Represents a set of `ArrayThreshold` objects.

      - ``thresholds`` : List[IArrayThreshold]
         - **Description**: List of `ArrayThreshold` objects that make up the set.
         - **Type**: List of `IArrayThreshold`

   Usage
   ~~~~~~
   .. code:: python

      import simplnx as sx

      params.insert(sx.ArrayThresholdsParameter('data_thresholds_key', 'Data Thresholds', 'DataArray thresholds to mask', sx.ArrayThresholdSet()))
 
.. _ArrayThresholdSet:
.. py:class:: ArrayThresholdSet

  This class holds a list of ArrayThreshold_ objects.

  :ivar thresholds: List[ArrayThreshold_] objects

.. _ArrayThreshold:
.. py:class:: ArrayThresholdSet.ArrayThreshold

  This class holds the values that are used for comparison in the :ref:`simplnx.MultiThresholdObjects() <MultiThresholdObjects>` filter.

  :ivar array_path: The :ref:`DataPath<DataPath>` to the array to use for this ArrayThreshold
  :ivar comparison: Int. The comparison operator to use. 0=">", 1="<", 2="=", 3="!="
  :ivar value: Numerical Value. The value for the comparison

   The below code will create an ArrayThresholdSet_ that is used to create a "Mask" output array of type boolean that will mark
   each value in its output array as "True" if **both** of the ArrayThreshold Objects evaluate to True. Specifically, the "Confidence Index" and "Image Quality"
   array MUST have the same number of Tuples and the output "Mask" array will also have the same number of tuples.

  .. code:: python

   threshold_1 = cx.ArrayThreshold()
   threshold_1.array_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])
   threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
   threshold_1.value = 0.1

   threshold_2 = cx.ArrayThreshold()
   threshold_2.array_path = cx.DataPath(["Small IN100", "Scan Data", "Image Quality"])
   threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
   threshold_2.value = 120

   threshold_set = cx.ArrayThresholdSet()
   threshold_set.thresholds = [threshold_1, threshold_2]
   result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                       array_thresholds=threshold_set, 
                                       created_data_path="Mask",
                                       created_mask_type=cx.DataType.boolean)

.. _AttributeMatrixSelectionParameter:
.. py:class:: AttributeMatrixSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      AttributeMatrixSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath)

   Description
   ~~~~~~~~~~~
   The ``AttributeMatrixSelectionParameter`` is used for selecting an Attribute Matrix within the data structure.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default DataPath pointing to the Attribute Matrix to be selected.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code:: python

      import simplnx as sx

      params.insert(sx.AttributeMatrixSelectionParameter('cell_attr_matrix_key', "Cell Attribute Matrix", "Example attribute matrix selection help text", sx.DataPath(["Image Geometry", "Cell Data"])))

.. _BoolParameter:
.. py:class:: BoolParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      BoolParameter(name: str, human_name: str, help_text: str, default_value: bool)

   Description
   ~~~~~~~~~~~
   The ``BoolParameter`` is used to toggle between two states, true or false.
   
   This parameter can be linked to other parameters so that the other parameters' availability depends on the current state of this parameter.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : bool
      - **Description**: The default boolean value (true or false) for the parameter.
      - **Type**: boolean (bool)

   General Usage
   ~~~~~~~~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.BoolParameter('example_bool_key', 'Bool Parameter', 'Example bool help text', False))
   
   Linked Usage
   ~~~~~~~~~~~~~
   The following example sets up a BoolParameter so that it toggles the availability of a DataGroupSelectionParameter.

   .. code-block:: python

      import simplnx as sx

      params.insert_linkable_parameter(sx.BoolParameter('example_bool_key', 'Bool Parameter', 'Example bool help text', True))
      params.insert(sx.DataGroupSelectionParameter('example_data_group_selection_key', 'DataGroupSelectionParameter', 'Example data group selection help text', sx.DataPath([]), set([sx.BaseGroup.GroupType.DataGroup])))

      params.link_parameters('example_bool_key', 'example_data_group_selection_key', True)

.. _CalculatorParameter:
.. py:class:: CalculatorParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      CalculatorParameter(name: str, human_name: str, help_text: str, default_value: CalculatorParameter.ValueType)

   Description
   ~~~~~~~~~~~
   The ``CalculatorParameter`` is used to execute mathematical expressions on data arrays and other data objects.

   This parameter has a single member type "ValueType" that can be constructed with the necessary values.

   .. py:class::    CalculatorParameter.ValueType

   :ivar selected_group: The :ref:`DataGroup<DataGroup>` or :ref:`AttributeMatrix<AttributeMatrix>` that contains the :ref:`DataArray<DataArray>` that will be used in the equations
   :ivar equation: String. The equation that will be evaluated
   :ivar units: cx.CalculatorParameter.AngleUnits.Radians or cx.CalculatorParameter.AngleUnits.Degrees

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : CalculatorParameter.ValueType
      - **Description**: The default value or expression for the calculator parameter.
      - **Type**: `CalculatorParameter.ValueType`
      - **Internal Data**: The values that are contained within the CalculatorParameter.ValueType.
         - selected_group: The :ref:`DataGroup<DataGroup>` or :ref:`AttributeMatrix<AttributeMatrix>` that contains the :ref:`DataArray<DataArray>` that will be used in the equations.
         - equation: String. The equation that will be evaluated.
         - units: sx.CalculatorParameter.AngleUnits.Radians or sx.CalculatorParameter.AngleUnits.Degrees

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      calc_param = sx.CalculatorParameter.ValueType( sx.DataPath(["Small IN100","Scan Data"]), "Confidence Index * 10", sx.CalculatorParameter.AngleUnits.Radians)
      params.insert(sx.CalculatorParameter(ExampleFilter2.PARAM18_KEY, "CalculatorParameter", "Example help text for calculator parameter", calc_param))

.. _ChoicesParameter:
.. py:class:: ChoicesParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ChoicesParameter(name: str, human_name: str, help_text: str, default_value: int, choices: List[str])

   Description
   ~~~~~~~~~~~
   The ``ChoicesParameter`` is used to provide a string selection from a list of predefined choices.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : int
      - **Description**: The default selected index (from the list of choices) for the parameter.
      - **Type**: integer (int)

   - ``choices`` : List[str]
      - **Description**: The list of available choices for the parameter.
      - **Type**: List of strings (List[str])

   General Usage
   ~~~~~~~~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.ChoicesParameter('example_choices_key', 'Choices Parameter', 'Example choices help text', 0, ["foo", "bar", "baz"]))
   
   Linked Usage
   ~~~~~~~~~~~~~
   The following example sets up a ChoicesParameter so that it toggles the availability of a DataPathSelectionParameter based on whether or not the ChoicesParameter is set to the second choice.

   .. code-block:: python

      import simplnx as sx

      params.insert_linkable_parameter(sx.ChoicesParameter('example_choices_key', 'Choices Parameter', 'Example choices help text', 0, ["foo", "bar", "baz"]))
      params.insert(sx.DataPathSelectionParameter('example_data_path_selection_key', 'DataPathSelectionParameter', 'Example data path selection help text', sx.DataPath([])))

      params.link_parameters('example_choices_key', 'example_data_path_selection_key', 1)

.. _DataGroupCreationParameter:
.. py:class:: DataGroupCreationParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataGroupCreationParameter(name: str, human_name: str, help_text: str, default_value: DataPath)

   Description
   ~~~~~~~~~~~
   The ``DataGroupCreationParameter`` is used to specify the creation of a data group within the data structure.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default DataPath for the data group to be created.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.DataGroupCreationParameter('example_data_group_creation_key', 'DataGroupCreationParameter', 'Example data group creation help text', sx.DataPath([])))

.. _DataGroupSelectionParameter:
.. py:class:: DataGroupSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataGroupSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[BaseGroup.GroupType])

   Description
   ~~~~~~~~~~~
   The ``DataGroupSelectionParameter`` is used for selecting an existing data group within the data structure based on a set of allowed group types.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default DataPath pointing to the data group to be selected.
      - **Type**: `DataPath`

   - ``allowed_types`` : Set[BaseGroup.GroupType]
      - **Description**: Set of allowed group types for the data group.
      - **Type**: Set of `BaseGroup.GroupType`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.DataGroupSelectionParameter('example_data_group_selection_key', 'DataGroupSelectionParameter', 'Example data group selection help text', sx.DataPath([]), set([sx.BaseGroup.GroupType.DataGroup])))

.. _DataObjectNameParameter:
.. py:class:: DataObjectNameParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataObjectNameParameter(name: str, human_name: str, help_text: str, default_value: str)

   Description
   ~~~~~~~~~~~
   The ``DataObjectNameParameter`` is used to specify the name of a data object within the data structure.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : str
      - **Description**: The default name for the data object.
      - **Type**: string (str)

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.DataObjectNameParameter('data_object_name_key', "DataObjectNameParameter", "Example help text for DataObjectNameParameter", "Data Group"))

.. _DataPathSelectionParameter:
.. py:class:: DataPathSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataPathSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath)

   Description
   ~~~~~~~~~~~
   The ``DataPathSelectionParameter`` is used for selecting a :ref:`DataPath<DataPath>` to a data object within the :ref:`DataStructure<DataStructure>`.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default DataPath to be selected.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.DataPathSelectionParameter('example_data_path_key', 'DataPathSelectionParameter', 'Example data path selection help text', sx.DataPath([])))

.. _DataStoreFormatParameter:
.. py:class:: DataStoreFormatParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataStoreFormatParameter(name: str, human_name: str, help_text: str, default_value: str)

   Description
   ~~~~~~~~~~~
   The ``DataStoreFormatParameter`` is used to specify the format of a :ref:`DataStore<DataStore>` within the data structure.
   
   Depending on the version of simplnx being used, there can be both in-core and out-of-core  :ref:`DataStore<DataStore>` objects available.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : str
      - **Description**: The default format for the data store.
      - **Type**: string (str)

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.DataStoreFormatParameter('data_store_format_key', 'Data Store Format', 'This value will specify which data format is used by the array\'s data store. An empty string results in in-memory data store.', ""))

.. _DataTypeParameter:
.. py:class:: DataTypeParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataTypeParameter(name: str, human_name: str, help_text: str, default_value: DataType)

   Description
   ~~~~~~~~~~~
   The ``DataTypeParameter`` is used to specify the type of data for a particular operation or data structure element.

   This parameter holds an enumeration value that represents the numerical type for created arrays. The possible values are:

   .. code:: python

      cx.DataType.int8
      cx.DataType.uint8
      cx.DataType.int16
      cx.DataType.uint16
      cx.DataType.int32
      cx.DataType.uint32
      cx.DataType.int64
      cx.DataType.uint64
      cx.DataType.float32
      cx.DataType.float64
      cx.DataType.boolean

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : DataType
      - **Description**: The default data type.
      - **Type**: `DataType`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.DataTypeParameter('data_type_key', "Data Type", "Example data type help text", sx.DataType.float64))

.. _Dream3dImportParameter:
.. py:class:: Dream3dImportParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      Dream3dImportParameter(name: str, human_name: str, help_text: str, default_value: Dream3dImportParameter.ImportData)

   Description
   ~~~~~~~~~~~
   The ``Dream3dImportParameter`` holds the information necessary to import a .dream3d file through the **ImportData** object.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : Dream3dImportParameter.ImportData
      - **Description**: The default import data setting for DREAM3D.
      - **Type**: `Dream3dImportParameter.ImportData`
      - **Internal Data**:
         - **file_path**: Path to the .dream3d file on the file system
         - **data_paths**: List of :ref:`DataPath<DataPath>` objects. Use the python 'None' value to indicate that you want to read **ALL** the data from file.

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      import_data = sx.Dream3dImportParameter.ImportData()
      import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
      import_data.data_paths = None
      params.insert(sx.Dream3dImportParameter('import_file_path_key', "Import File Path", "The HDF5 file path the DataStructure should be imported from.", import_data))

.. _DynamicTableParameter:
.. py:class:: DynamicTableParameter

   Declarations
   ~~~~~~~~~~~
   .. code-block:: python

      DynamicTableParameter(name: str, human_name: str, help_text: str, default_value: List[List[float]], table_info: DynamicTableInfo)

      DynamicTableParameter(name: str, human_name: str, help_text: str, table_info: DynamicTableInfo)

   Description
   ~~~~~~~~~~~
   The ``DynamicTableParameter`` is used to specify parameters for dynamic tables which can be modified by the user during runtime. It involves detailed configuration of rows and columns using the `DynamicTableInfo` class.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : List[List[float]]
      - **Description**: The default value for the dynamic table, typically a list of lists representing the table rows and columns.
      - **Type**: List of lists of floats (List[List[float]])

   - ``table_info`` : DynamicTableInfo
      - **Description**: Configuration information for the dynamic table, including row and column details.
      - **Type**: `DynamicTableInfo`

   .. py:class:: DynamicTableInfo

      Used to provide detailed configuration for the dynamic table's rows and columns. It includes the following nested classes:

      .. py:class:: DynamicVectorInfo

         - Used to specify dynamic rows or columns where the size can be adjusted.
         - **Methods**:
            - ``__init__(self, min_size: int, default_size: int, header_template: str)``: Initialize with minimum size, default size, and a header template.
            - ``__init__(self, min_size: int, header_template: str)``: Initialize with minimum size and a header template.

      .. py:class:: StaticVectorInfo

         - Used to specify static rows or columns with a fixed size or predefined headers.
         - **Methods**:
            - ``__init__(self, size: int)``: Initialize with a fixed size.
            - ``__init__(self, headers: List[str])``: Initialize with predefined headers.

      .. py:class:: VectorInfo

         - Used as a wrapper to specify information about either static or dynamic rows/columns.
         - **Methods**:
            - ``__init__(self, vector_info: DynamicTableInfo.StaticVectorInfo)``: Initialize with static vector information.
            - ``__init__(self, vector_info: DynamicTableInfo.DynamicVectorInfo)``: Initialize with dynamic vector information.

      - **Methods**:
         - ``__init__(self)``: Initialize without specific row/column information.
         - ``__init__(self, rows_info: DynamicTableInfo.VectorInfo, cols_info: DynamicTableInfo.VectorInfo)``: Initialize with specific information for rows and columns.
         - ``set_cols_info(self, info: DynamicTableInfo.VectorInfo)``: Set information for columns.
         - ``set_rows_info(self, info: DynamicTableInfo.VectorInfo)``: Set information for rows.

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      default_table = [[10, 20], [30, 40]]
      row_info = sx.DynamicTableInfo.DynamicVectorInfo(0, "Row {}")
      col_info = sx.DynamicTableInfo.DynamicVectorInfo(2, "Col {}")
      dynamic_table_info = sx.DynamicTableInfo(sx.DynamicTableInfo.VectorInfo(row_info), sx.DynamicTableInfo.VectorInfo(col_info))
      params.insert(sx.DynamicTableParameter('dynamic_table', 'DynamicTableParameter', 'DynamicTableParameter Example Help Text', default_table, dynamic_table_info))

.. _EnsembleInfoParameter:
.. py:class:: EnsembleInfoParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      EnsembleInfoParameter(name: str, human_name: str, help_text: str, default_value)

   Description
   ~~~~~~~~~~~
   The ``EnsembleInfoParameter`` is used to represent a list of 3 value lists. Each list holds 3 values, Crystal Structure, Phase Type, Phase Name.
   
   Each row represents a specific phase.

   The valid values for the **Crystal Structures** are:

   - "Hexagonal-High 6/mmm"
   - "Cubic-High m-3m"
   - "Hexagonal-Low 6/m"
   - "Cubic-Low m-3 (Tetrahedral)"
   - "Triclinic -1"         
   - "Monoclinic 2/m" 
   - "Orthorhombic mmm"
   - "Tetragonal-Low 4/m"
   - "Tetragonal-High 4/mmm"
   - "Trigonal-Low -3", 
   - "Trigonal-High -3m"

   The valid **Phase Types** are:

   - "Primary"
   - "Precipitate"
   - "Transformation"
   - "Matrix"
   - "Boundary"

   The user can define their own phase names.

   This is used in combination with the :ref:`OrientationAnalysis.CreateEnsembleInfoFilter() <CreateEnsembleInfoFilter>` filter.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value``
      - **Description**: The default value for the ensemble information.
      - **Type**: Varies (type is context-dependent)

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      ensemble_info = []
      ensemble_info.append(["Hexagonal-High 6/mmm","Primary","Phase 1"])
      ensemble_info.append(["Cubic-High m-3m","Primary","Phase 2"])
      params.insert(sx.EnsembleInfoParameter('created_ensemble_info_key', "Created Ensemble Info", "The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase.", ensemble_info))

.. _FileSystemPathParameter:
.. py:class:: FileSystemPathParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      FileSystemPathParameter(name: str, human_name: str, help_text: str, default_value: os.PathLike, extensions_type: Set[str], path_type: FileSystemPathParameter.PathType, accept_all_extensions: bool = ...)

   Description
   ~~~~~~~~~~~
   The ``FileSystemPathParameter`` is used to specify a file system path, allowing the user to select directories or files for input or output operations.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : os.PathLike
      - **Description**: The default path.
      - **Type**: `os.PathLike`

   - ``extensions_type`` : Set[str]
      - **Description**: Set of allowed file extensions.
      - **Type**: Set of strings (Set[str])

   - ``path_type`` : FileSystemPathParameter.PathType
      - **Description**: The type of path (InputDir, InputFile, OutputDir, OutputFile).
      - **Type**: `FileSystemPathParameter.PathType`

   - ``accept_all_extensions`` : bool
      - **Description**: Flag indicating whether all file extensions are acceptable.
      - **Type**: boolean (bool)

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.FileSystemPathParameter('input_dir', 'Input Directory', 'Example input directory help text', 'Data', set(), sx.FileSystemPathParameter.PathType.InputDir))
      params.insert(sx.FileSystemPathParameter('input_file', 'Input File', 'Example input file help text', '/opt/local/bin/ninja', set(), sx.FileSystemPathParameter.PathType.InputFile, True))
      params.insert(sx.FileSystemPathParameter('output_dir', 'Output Directory', 'Example output directory help text', 'Output Data', set(), sx.FileSystemPathParameter.PathType.OutputDir))
      params.insert(sx.FileSystemPathParameter('output_file', 'Output File', 'Example output file help text', '', set(), sx.FileSystemPathParameter.PathType.OutputFile))

.. _GenerateColorTableParameter:
.. py:class:: GenerateColorTableParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      GenerateColorTableParameter(name: str, human_name: str, help_text: str, default_value: Json)

   Description
   ~~~~~~~~~~~
   **NOTE: THIS API IS GOING TO CHANGE IN A FUTURE UPDATE**
   
   The ``GenerateColorTableParameter`` is used to specify parameters for generating color tables, typically used in visualization or data representation.

   This parameter is used specifically for the  :ref:`simplnx.GenerateColorTableFilter() <GenerateColorTableFilter>` filter.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : Json
      - **Description**: The default color table in JSON format.
      - **Type**: `Json`
   
   .. attribute:: simplnx.Json
      
      This value encapsulates a string that represents well formed JSON. It can be constructed on-the-fly as follows:

      .. code:: python

         color_control_points = cx.Json('{"RGBPoints": [0,0,0,0,0.4,0.901960784314,0,0,0.8,0.901960784314,0.901960784314,0,1,1,1,1]}')

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      color_control_points = sx.Json('{"RGBPoints": [0,0,0,0,0.4,0.901960784314,0,0,0.8,0.901960784314,0.901960784314,0,1,1,1,1]}')
      params.insert(sx.GenerateColorTableParameter('color_table_preset_key', "Select Preset...", "Select a preset color scheme to apply to the created array", color_control_points))

.. _GeneratedFileListParameter:
.. py:class:: GeneratedFileListParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      GeneratedFileListParameter(name: str, human_name: str, help_text: str, default_value: GeneratedFileListParameter.ValueType)

   Description
   ~~~~~~~~~~~
   The ``GeneratedFileListParameter`` is used to specify parameters for generating a list of file paths, typically used in batch processing or automated file generation.

   In order to instantiate this parameter, the programmer should use the ``GeneratedFileListParameter.ValueType`` data member.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : GeneratedFileListParameter.ValueType
      - **Description**: The default configuration for generating the file list.
      - **Type**: `GeneratedFileListParameter.ValueType`
   
   .. py:class:: GeneratedFileListParameter.Ordering

      Enumeration that defines the ordering of the generated file list.

      - ``HighToLow``, ``LowToHigh``
         - **Description**: Determines the ordering of the files in the generated list.
         - **Type**: Enum (int)

   .. py:class:: GeneratedFileListParameter.ValueType

      Represents the configuration for the generated file list.

      - **Fields**:
         - ``start_index`` : int
            - **Description**: The start index for the file list generation.
         - ``end_index`` : int
            - **Description**: The end index for the file list generation (inclusive).
         - ``file_extension`` : str
            - **Description**: The file extension of the input files including the "." character.
         - ``file_prefix`` : str
            - **Description**: The string part of the file name that appears **before** the index digits.
         - ``file_suffix`` : str
            - **Description**: The string part of the file anem that appears **after** the index digits.
         - ``increment_index`` : int
            - **Description**: The value that determines how much to increment the index value when generating the file list.
         - ``input_path`` : str
            - **Description**: The file system path to the directory that contains the input files
         - ``ordering`` : GeneratedFileListParameter.Ordering
            - **Description**: The ordering of the generated files (HighToLow or LowToHigh).
         - ``padding_digits`` : int
            - **Description**: The number of digits used for padding the file index.

      - **Methods**:
         - ``generate()``: Generates the list of file names.
         - ``generate_and_validate(arg0: bool)``: Generates the list of file names and validates them based on the provided argument.

   Usage
   ~~~~~~

   If you have a stack of images in tif format numbered from 11 to 174 where there are only 2 digits for slice indices \< 100 and 3 digits after 100, the breakdown of the file name is as follows:

      +------------------------+--------------------------+--------+-----------+
      | Prefix                 | index and padding digits | suffix | extension |
      +========================+==========================+========+===========+
      | slice-                 | 100                      | _Data  | .tif      |
      +------------------------+--------------------------+--------+-----------+

   The python code to implement this scheme is as follows:

   .. code-block:: python

      import simplnx as sx

      generated_file_list_value = sx.GeneratedFileListParameter.ValueType()
      generated_file_list_value.input_path = "DREAM3DNXData/Data/Porosity_Image"
      generated_file_list_value.ordering = sx.GeneratedFileListParameter.Ordering.LowToHigh
      generated_file_list_value.file_prefix = "slice-"
      generated_file_list_value.file_suffix = ""
      generated_file_list_value.file_extension = ".tif"
      generated_file_list_value.start_index = 11
      generated_file_list_value.end_index = 174
      generated_file_list_value.increment_index = 1
      generated_file_list_value.padding_digits = 2
      params.insert(sx.GeneratedFileListParameter('input_file_list_key', "Input File List", "The list of files to be read", generated_file_list_value))

.. _GeometrySelectionParameter:
.. py:class:: GeometrySelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      GeometrySelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[IGeometry.Type])

   Description
   ~~~~~~~~~~~
   The ``GeometrySelectionParameter`` is used to specify a valid :ref:`simplnx.Geometry() <Geometry Descriptions>` selection within the data structure, constrained by allowed geometry types.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : :ref:`DataPath<DataPath>`
      - **Description**: The default path to the geometry data.
      - **Type**: `DataPath`

   - ``allowed_types`` : Set[IGeometry.Type]
      - **Description**: The set of allowed geometry types for the selection.
      - **Type**: Set of `IGeometry.Type`
   
   .. attribute:: IGeometry.Type

      Defines the allowed types of geometry data that can be chosen.

      - ``Edge``: Represents edge geometry.
      - ``Hexahedral``: Represents hexahedral geometry.
      - ``Image``: Represents image geometry.
      - ``Quad``: Represents quad geometry.
      - ``RectGrid``: Represents rectangular grid geometry.
      - ``Tetrahedral``: Represents tetrahedral geometry.
      - ``Triangle``: Represents triangle geometry.
      - ``Vertex``: Represents vertex geometry.

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.GeometrySelectionParameter('example_geometry_selection_parameter', 'GeometrySelectionParameter', 'Example geometry selection help text', sx.DataPath([]), set([sx.IGeometry.Type.Image, sx.IGeometry.Type.RectGrid])))

.. _ReadCSVFileParameter:
.. py:class:: ReadCSVFileParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ReadCSVFileParameter(self, name: str, human_name: str, help_text: str, default_value: ReadCSVDataParameter)

   Description
   ~~~~~~~~~~~
   The ``ReadCSVFileParameter`` is used to specify parameters for reading data from a CSV (Comma-Separated Values) file using a `ReadCSVDataParameter` instance for detailed configuration.

   + The file can be comma, space, tab or semicolon separated.
   + The file optionally can have a line of headers. The user can specify what line number the header is located.
   + The import can start at a user specified line number and will continue importing lines equal to the total number of tuples that the user specified.

   The primary python object that will hold the parameter information is the `ReadCSVDataParameter` class described below.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The unique, programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : ReadCSVDataParameter
      - **Description**: The default configuration for reading the CSV file.
      - **Type**: `ReadCSVDataParameter`
   
   .. py:class:: ReadCSVDataParameter

      The ReadCSVDataParameter class holds all the necessary information to import a CSV formatted file into DREAM3D-NX. There are
      a number of member variables that need to be set correctly before passing it into the filter parameter as the default value.

      - ``column_data_types`` : List[DataType]
         - **Description**: The data types of the columns in the CSV file. Indicates the kind of native numerical values (int, float... ) that will be used in the created  :ref:`DataArray<DataArray>`.
         - **Type**: List of :ref:`cx.DataType<DataTypeParameter>`

      - ``consecutive_delimiters`` : bool
         - **Description**: Flag indicating whether consecutive delimiters should be treated as a single delimiter.
         - **Type**: boolean (bool)

      - ``custom_headers`` : List[str]
         - **Description**: Custom headers to use if the header mode is set to Custom.
         - **Type**: List of strings (List[str])

      - ``delimiters`` : List[str]
         - **Description**: The delimiters used in the CSV file.
         - **Type**: List of strings (List[str])

      - ``header_mode`` : ReadCSVDataParameter.HeaderMode
         - **Description**: The mode used for parsing headers in the CSV file (Custom or Line).
         - **Type**: `ReadCSVDataParameter.HeaderMode`

      - ``headers_line`` : int
         - **Description**: The line number where headers are located, used if the header mode is set to Line. One-based indexing.
         - **Type**: integer (int)

      - ``input_file_path`` : str
         - **Description**: The file path to the input CSV file.
         - **Type**: string (str)

      - ``skipped_array_mask`` : List[bool]
         - **Description**: Booleans, one per column, that indicate whether or not to skip importing each created :ref:`DataArray <DataArray>`.
         - **Type**: List of booleans (List[bool])

      - ``start_import_row`` : int
         - **Description**: The row number from which to start importing data.  One-based indexing.
         - **Type**: integer (int)

      - ``tuple_dims`` : List[int]
         - **Description**: The tuple dimensions for the created  :ref:`DataArrays <DataArray>`.
         - **Type**: List of integers (List[int])

   ``ReadCSVDataParameter.HeaderMode`` Enum
   ----------------------------------------
   Defines the modes for parsing headers in the CSV file.

      - ``Custom``, ``Line``
         - **Description**: Determines how headers are parsed (Custom: use custom headers, Line: use headers from a specific line).
         - **Type**: Enum (int)

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx
      
      # Example File has 7 columns to import
      read_csv_data = sx.ReadCSVDataParameter()
      read_csv_data.input_file_path = "/tmp/test_csv_data.csv"
      read_csv_data.start_import_row = 2
      read_csv_data.delimiters = [',']
      read_csv_data.custom_headers = []
      read_csv_data.column_data_types = [sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.float32,sx.DataType.int32]
      read_csv_data.skipped_array_mask = [False,False,False,False,False,False,False]
      read_csv_data.tuple_dims = [37989]
      read_csv_data.headers_line = 1
      read_csv_data.header_mode = sx.ReadCSVDataParameter.HeaderMode.Line
      params.insert(sx.ReadCSVFileParameter('csv_importer_data_key', "CSV Importer Data", "Holds all relevant csv file data collected from the custom interface", read_csv_data))

.. _ReadH5EbsdFileParameter:
.. py:class:: ReadH5EbsdFileParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class ReadH5EbsdFileParameter(simplnx.IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: ReadH5EbsdFileParameter.ValueType) -> None: ...

   Description
   ~~~~~~~~~~~
   This parameter is used for the :ref:`orientationAnalysis.ReadH5EbsdFilter() <ReadH5EbsdFilter>` and holds the information to import the EBSD data from the H5EBSD file.

   The primary python object that will hold the default information to pass to this parameter is the ReadH5EbsdFileParameter.ValueType class described below.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : ReadH5EbsdFileParameter.ValueType
      - **Description**: The default configuration for reading the EBSD data.
      - **Type**: `ReadH5EbsdFileParameter.ValueType`

   ``ReadH5EbsdFileParameter.ValueType`` Class
   -------------------------------------------
   Represents the configuration for reading EBSD data from the H5EBSD file.

   - **Fields**:
      - ``end_slice`` : int
         - **Description**: The end slice (inclusive) for the EBSD data import.
      - ``euler_representation`` : int
         - **Description**: The representation of Euler angles in the EBSD data. 0 = Radians, 1 = Degrees.
      - ``input_file_path`` : str
         - **Description**: The file path to the input .h5ebsd file containing EBSD data.
      - ``selected_array_names`` : List[str]
         - **Description**: The names of the EBSD data to import. These may differ slightly between the various OEMs.
      - ``start_slice`` : int
         - **Description**: The start slice for the EBSD data import.
      - ``use_recommended_transform`` : bool
         - **Description**: Apply the stored sample and crystal reference frame transformations.

   Usage
   ~~~~~~

   .. code-block:: python

      import orientationanalysis as oa

      read_h5ebsd_data = oa.ReadH5EbsdFileParameter.ValueType()
      read_h5ebsd_data.euler_representation=0
      read_h5ebsd_data.end_slice=117
      read_h5ebsd_data.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
      read_h5ebsd_data.input_file_path="Data/Output/Reconstruction/Small_IN100.h5ebsd"
      read_h5ebsd_data.start_slice=1
      read_h5ebsd_data.use_recommended_transform=True
      params.insert(oa.ReadH5EbsdFileParameter('import_h5ebsd_file_key', "Import H5Ebsd File", "Object that holds all relevant information to import data from the file.", read_h5ebsd_data))

.. _ReadHDF5DatasetParameter:
.. py:class:: ReadHDF5DatasetParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class ReadHDF5DatasetParameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: ReadHDF5DatasetParameter.ValueType) -> None: ...

   Description
   ~~~~~~~~~~~
   This parameter is used for the :ref:`simplnx.ReadHDF5Dataset<ReadHDF5Dataset>` and holds the information to import specific data sets from within the HDF5 file into DREAM3D/simplnx

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : ReadHDF5DatasetParameter.ValueType
      - **Description**: The default configuration for reading the data sets from the HDF5 file.
      - **Type**: `ReadHDF5DatasetParameter.ValueType`

   ``ReadHDF5DatasetParameter.DatasetImportInfo`` Class
   ----------------------------------------------------
   Represents the configuration for importing a single data set from the HDF5 file.

   - **Fields**:
      - ``component_dims`` : str
         - **Description**: The dimensions of the components in the data set.
      - ``dataset_path`` : str
         - **Description**: The path to the data set within the HDF5 file.
      - ``tuple_dims`` : str
         - **Description**: The dimensions of the tuples in the data set.

   ``ReadHDF5DatasetParameter.ValueType`` Class
   --------------------------------------------
   Represents the configuration for importing data sets from the HDF5 file.

   - **Fields**:
      - ``datasets`` : List[ReadHDF5DatasetParameter.DatasetImportInfo]
         - **Description**: The list of data sets to be imported.
      - ``input_file`` : str
         - **Description**: The file path to the input HDF5 file.
      - ``parent`` : Optional[DataPath]
         - **Description**: The :ref:`DataPath<DataPath>` object to a parent group to create the :ref:`DataArray<DataArray>`s into. If left blank, the :ref:`DataArray<DataArray>` will be created at the top level of the :ref:`DataStructure<DataStructure>`.

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      dataset1 = sx.ReadHDF5DatasetParameter.DatasetImportInfo()
      dataset1.dataset_path = "/DataStructure/DataContainer/CellData/Confidence Index"
      dataset1.tuple_dims = "117,201,189"
      dataset1.component_dims = "1"

      dataset2 = sx.ReadHDF5DatasetParameter.DatasetImportInfo()
      dataset2.dataset_path = "/DataStructure/DataContainer/CellData/EulerAngles"
      dataset2.tuple_dims = "117,201,189"
      dataset2.component_dims = "3"

      import_hdf5_param = sx.ReadHDF5DatasetParameter.ValueType()
      import_hdf5_param.input_file = "SmallIN100_Final.dream3d"
      import_hdf5_param.datasets = [dataset1, dataset2]

      params.insert(sx.ReadHDF5DatasetParameter('hdf5_file_key', "Select HDF5 File", "The HDF5 file data to import", import_hdf5_param))

.. _MultiArraySelectionParameter:
.. py:class:: MultiArraySelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class MultiArraySelectionParameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[DataPath], allowed_types: Set[IArray.ArrayType], allowed_data_types: Set[DataType], required_comps: List[List[int]] = ...) -> None: ...

   Description
   ~~~~~~~~~~~
   This parameter is used to specify a selection of multiple arrays within the data structure, constrained by component dimensions as well as allowed array and data types.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : List[DataPath]
      - **Description**: The default paths to the selected arrays.
      - **Type**: List of `DataPath`

   - ``allowed_types`` : Set[IArray.ArrayType]
      - **Description**: The set of allowed array types for the selected arrays.
      - **Type**: Set of `IArray.ArrayType`

   - ``allowed_data_types`` : Set[DataType]
      - **Description**: The set of allowed data types for the selected arrays.
      - **Type**: Set of `DataType`

   - ``required_comps`` : List[List[int]]
      - **Description**: List of required component dimensions for the selected arrays.
      - **Type**: List of lists of integers

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.MultiArraySelectionParameter(ExampleFilter2.PARAM12_KEY, 'MultiArraySelectionParameter', 'Example multiarray selection help text', [], set([sx.IArray.ArrayType.Any]), sx.get_all_data_types(), [[1]]))


.. _MultiPathSelectionParameter:
.. py:class:: MultiPathSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class MultiPathSelectionParameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[DataPath]) -> None: ...

   Description
   ~~~~~~~~~~~
   This parameter represents a list of :ref:`DataPath<DataPath>` objects. The end point of each :ref:`DataPath<DataPath>` object can be any object in the  :ref:`DataStructure<DataStructure>`

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : List[DataPath]
      - **Description**: The default paths selected by the parameter.
      - **Type**: List of `DataPath`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx
      
      params.insert(sx.MultiPathSelectionParameter('objects_to_copy_key', "Objects to copy", "A list of DataPaths to the DataObjects to be copied", [sx.DataPath(["Small IN100", "Scan Data", "Confidence Index"]), sx.DataPath(["Small IN100", "Scan Data", "Euler Angles"])]))

.. _NeighborListSelectionParameter:
.. py:class:: NeighborListSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class NeighborListSelectionParameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[DataType]) -> None: ...

   Description
   ~~~~~~~~~~~
   The ``NeighborListSelectionParameter`` is used to specify a selection of a neighbor list array within the data structure, constrained by allowed data types.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : DataPath
      - **Description**: The default path to the neighbor list array.
      - **Type**: `DataPath`

   - ``allowed_types`` : Set[DataType]
      - **Description**: The set of allowed data types for the neighbor list.
      - **Type**: Set of `DataType`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx
      
      params.insert(sx.NeighborListSelectionParameter('neighbor_list_key', "Neighbor List", "List of the contiguous neighboring Features for a given Feature", sx.DataPath([]), set([sx.DataType.int32])))

.. _NumericTypeParameter:
.. py:class:: NumericTypeParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class NumericTypeParameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: NumericType) -> None: ...

   Description
   ~~~~~~~~~~~
   This parameter represents a choice from a list of known numeric types. The programmer should use the predefined types instead of a plain integer value.

    - NumericType.int8 = 0
    - NumericType.uint8= 1
    - NumericType.int16= 2
    - NumericType.uint16= 3
    - NumericType.int32= 4
    - NumericType.uint32= 5
    - NumericType.int64= 6
    - NumericType.uint64= 7
    - NumericType.float32= 8
    - NumericType.float64= 9

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : NumericType
      - **Description**: The default numeric type.
      - **Type**: `NumericType`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.NumericTypeParameter('numeric_type_key', 'Numeric Type', 'Example numeric type help text', sx.NumericType.int32))

.. _StringParameter:
.. py:class:: StringParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      class StringParameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: str) -> None: ...

   Description
   ~~~~~~~~~~~
   The ``StringParameter`` is used to specify a string input.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : str
      - **Description**: The default string value for the parameter.
      - **Type**: string (str)

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.StringParameter('string_key', 'StringParameter', 'Example string help text', 'Example String'))

Numerical Parameters
--------------------

   Declarations
   ~~~~~~~~~~~
   .. code-block:: python

      class Int8Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class UInt8Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...

      class Int16Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class UInt16Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class Int32Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class UInt32Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class Int64Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class UInt64Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: int) -> None: ...
      
      class Float32Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: float) -> None: ...
      
      class Float64Parameter(IParameter):
         def __init__(self, name: str, human_name: str, help_text: str, default_value: float) -> None: ...

   Description
   ~~~~~~~~~~~

   This group of parameters wrap a specific native C++ numeric type and can be used to add integer/float inputs to a filter.  Their default values can be instantiated using standard python integers or decimal values.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : int
      - **Description**: The default value for the parameter, expected to be within the range of the chosen float or integer type.
      - **Type**: Integer or Float

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.Float32Parameter('float32_key', 'Float32Parameter', 'The 1st parameter', 0.1234))
      params.insert(sx.Int32Parameter('int32_key', 'Int32Parameter', 'The 2nd parameter', 0))

Numerical Vector Parameters
--------------------

   Declarations
   ~~~~~~~~~~~
   .. code-block:: python

      class VectorInt8Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorUInt8Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorInt16Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorUInt16Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...
      
      class VectorInt32Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorUInt32Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorInt64Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorUInt64Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[int], names: List[str]) -> None: ...

      class VectorFloat32Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[float]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[float], names: List[str]) -> None: ...

      class VectorFloat64Parameter(IParameter):
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[float]) -> None: ...
         @overload
         def __init__(self, name: str, human_name: str, help_text: str, default_value: List[float], names: List[str]) -> None: ...

   Description
   ~~~~~~~~~~~

   This group of parameters can be used to gather more than a single scalar value from the user. For example, an Origin for an Image Geometry or the dimensions of a DataArray. It is represented as a list of numerical values.

   Inputs
   ~~~~~~
   - ``name`` : str
      - **Description**: The programmatic name of the parameter.
      - **Type**: string (str)

   - ``human_name`` : str
      - **Description**: The human-readable name of the parameter.
      - **Type**: string (str)

   - ``help_text`` : str
      - **Description**: A brief description or help text for the parameter.
      - **Type**: string (str)

   - ``default_value`` : int
      - **Description**: The default value for the parameter, a list of integers or floats that are all expected to be within the range of the chosen float or integer type.
      - **Type**: List of Integers or Floats

   - ``names`` : int
      - **Description**: The list of names that describe each value in the vector.
      - **Type**: List of strings

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as sx

      params.insert(sx.VectorInt32Parameter('3d_dimensions_key', '3D Dimensions', 'Example int32 vector help text', [-19, -100, 456], ["X", "Y", "Z"]))
      params.insert(sx.VectorFloat64Parameter('quaternion_key', 'Quaternion', 'Example float64 vector help text', [0, 84.98, 234.12, 985.98], ["U", "V", "W", "X"]))
