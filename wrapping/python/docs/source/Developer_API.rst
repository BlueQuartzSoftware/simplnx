SIMPLNX Filter Writing API
==========================

General Parameters 
------------------

.. _ArrayCreationParameter:
.. py:class:: ArrayCreationParameter


   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ArrayCreationParameter(name: str, human_name: str, help_text: str, default_value: DataPath) -> None

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
      - **Description**: The default value for the parameter. For the `ArrayCreationParameter`, this is a :ref:`DataPath <DataPath>` object that points to the location within the data structure where the data array will be created.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code:: python
      
      import simplnx as nx
      
      data_path = nx.DataPath("Small IN100/Scan Data/Data")
      params.insert(nx.ArrayCreationParameter(ExampleFilter2.PARAM5_KEY, 'Array Creation', 'Example array creation help text', data_path))

.. _ArraySelectionParameter:
.. py:class:: ArraySelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ArraySelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[DataType], required_comps: List[List[int]] = ..., location: ArraySelectionParameter.DataLocation = ...) -> None

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
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

      import simplnx as nx

      data_path = nx.DataPath("Small IN100/Scan Data/Data")
      params.insert(nx.ArraySelectionParameter(ExampleFilter2.PARAM6_KEY, 'Array Selection', 'Example array selection help text', data_path, nx.get_all_data_types(), [[1]]))

.. _ArrayThresholdsParameter:
.. py:class:: ArrayThresholdsParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ArrayThresholdsParameter(name: str, human_name: str, help_text: str, default_value: ArrayThresholdSet, required_comps: List[List[int]] = ...) -> None

   Description
   ~~~~~~~~~~~
   The ``ArrayThresholdsParameter`` is used to specify thresholds for an array, allowing for filtering based on those thresholds.
   
   This parameter holds a ArrayThresholdSet_ object and is used specifically for the :ref:`simplnx.MultiThresholdObjects() <MultiThresholdObjectsFilter>` filter.
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

      import simplnx as nx

      params.insert(nx.ArrayThresholdsParameter('data_thresholds_key', 'Data Thresholds', 'DataArray thresholds to mask', nx.ArrayThresholdSet()))
 
.. _ArrayThresholdSet:
.. py:class:: ArrayThresholdSet

  This class holds a list of ArrayThreshold_ objects.

  :ivar thresholds: List[ArrayThreshold_] objects

.. _ArrayThreshold:
.. py:class:: ArrayThresholdSet.ArrayThreshold

  This class holds the values that are used for comparison in the :ref:`simplnx.MultiThresholdObjects() <MultiThresholdObjectsFilter>` filter.

  :ivar array_path: The :ref:`DataPath <DataPath>` to the array to use for this ArrayThreshold
  :ivar comparison: Int. The comparison operator to use. 0=">", 1="<", 2="=", 3="!="
  :ivar value: Numerical Value. The value for the comparison

   The below code will create an ArrayThresholdSet_ that is used to create a "Mask" output array of type boolean that will mark
   each value in its output array as "True" if **both** of the ArrayThreshold Objects evaluate to True. Specifically, the "Confidence Index" and "Image Quality"
   array MUST have the same number of Tuples and the output "Mask" array will also have the same number of tuples.

  .. code:: python

   threshold_1 = nx.ArrayThreshold()
   threshold_1.array_path = nx.DataPath("Small IN100/Scan Data/Confidence Index")
   threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
   threshold_1.value = 0.1

   threshold_2 = nx.ArrayThreshold()
   threshold_2.array_path = nx.DataPath("Small IN100/Scan Data/Image Quality")
   threshold_2.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
   threshold_2.value = 120

   threshold_set = nx.ArrayThresholdSet()
   threshold_set.thresholds = [threshold_1, threshold_2]
   result = nx.MultiThresholdObjects.execute(data_structure=data_structure,
                                       array_thresholds=threshold_set, 
                                       created_data_path="Mask",
                                       created_mask_type=nx.DataType.boolean)

.. _AttributeMatrixSelectionParameter:
.. py:class:: AttributeMatrixSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      AttributeMatrixSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath) -> None

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
      - **Description**: The default DataPath pointing to the Attribute Matrix to be selected.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code:: python

      import simplnx as nx

      params.insert(nx.AttributeMatrixSelectionParameter('cell_attr_matrix_key', "Cell Attribute Matrix", "Example attribute matrix selection help text", nx.DataPath("Image Geometry/Cell Data")))

.. _BoolParameter:
.. py:class:: BoolParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      BoolParameter(name: str, human_name: str, help_text: str, default_value: bool) -> None

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

      import simplnx as nx

      params.insert(nx.BoolParameter('example_bool_key', 'Bool Parameter', 'Example bool help text', False))
   
   Linked Usage
   ~~~~~~~~~~~~~
   The following example sets up a BoolParameter so that it toggles the availability of a DataGroupSelectionParameter.

   .. code-block:: python

      import simplnx as nx

      params.insert_linkable_parameter(nx.BoolParameter('example_bool_key', 'Bool Parameter', 'Example bool help text', True))
      params.insert(nx.DataGroupSelectionParameter('example_data_group_selection_key', 'DataGroupSelectionParameter', 'Example data group selection help text', nx.DataPath([]), set([nx.BaseGroup.GroupType.DataGroup])))

      params.link_parameters('example_bool_key', 'example_data_group_selection_key', True)

.. _CalculatorParameter:
.. py:class:: CalculatorParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      CalculatorParameter(name: str, human_name: str, help_text: str, default_value: CalculatorParameter.ValueType) -> None

   Description
   ~~~~~~~~~~~
   The ``CalculatorParameter`` is used to execute mathematical expressions on data arrays and other data objects.

   This parameter has a single member type "ValueType" that can be constructed with the necessary values.

   .. py:class::    CalculatorParameter.ValueType

   :ivar selected_group: The :ref:`DataGroup<DataGroup>` or :ref:`AttributeMatrix<AttributeMatrix>` that contains the :ref:`DataArray <DataArray>` that will be used in the equations
   :ivar equation: String. The equation that will be evaluated
   :ivar units: nx.CalculatorParameter.AngleUnits.Radians or nx.CalculatorParameter.AngleUnits.Degrees

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
         - selected_group: The :ref:`DataGroup<DataGroup>` or :ref:`AttributeMatrix<AttributeMatrix>` that contains the :ref:`DataArray <DataArray>` that will be used in the equations.
         - equation: String. The equation that will be evaluated.
         - units: nx.CalculatorParameter.AngleUnits.Radians or nx.CalculatorParameter.AngleUnits.Degrees

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      calc_param = nx.CalculatorParameter.ValueType( nx.DataPath("Small IN100/Scan Data"), "Confidence Index * 10", nx.CalculatorParameter.AngleUnits.Radians)
      params.insert(nx.CalculatorParameter(ExampleFilter2.PARAM18_KEY, "CalculatorParameter", "Example help text for calculator parameter", calc_param))

.. _ChoicesParameter:
.. py:class:: ChoicesParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ChoicesParameter(name: str, human_name: str, help_text: str, default_value: int, choices: List[str]) -> None

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

      import simplnx as nx

      params.insert(nx.ChoicesParameter('example_choices_key', 'Choices Parameter', 'Example choices help text', 0, ["foo", "bar", "baz"]))
   
   Linked Usage
   ~~~~~~~~~~~~~
   The following example sets up a ChoicesParameter so that it toggles the availability of a DataPathSelectionParameter based on whether or not the ChoicesParameter is set to the second choice.

   .. code-block:: python

      import simplnx as nx

      params.insert_linkable_parameter(nx.ChoicesParameter('example_choices_key', 'Choices Parameter', 'Example choices help text', 0, ["foo", "bar", "baz"]))
      params.insert(nx.DataPathSelectionParameter('example_data_path_selection_key', 'DataPathSelectionParameter', 'Example data path selection help text', nx.DataPath([])))

      params.link_parameters('example_choices_key', 'example_data_path_selection_key', 1)

.. _DataGroupCreationParameter:
.. py:class:: DataGroupCreationParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataGroupCreationParameter(name: str, human_name: str, help_text: str, default_value: DataPath) -> None

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
      - **Description**: The default DataPath for the data group to be created.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      params.insert(nx.DataGroupCreationParameter('example_data_group_creation_key', 'DataGroupCreationParameter', 'Example data group creation help text', nx.DataPath([])))

.. _DataGroupSelectionParameter:
.. py:class:: DataGroupSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataGroupSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[BaseGroup.GroupType]) -> None

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
      - **Description**: The default DataPath pointing to the data group to be selected.
      - **Type**: `DataPath`

   - ``allowed_types`` : Set[BaseGroup.GroupType]
      - **Description**: Set of allowed group types for the data group.
      - **Type**: Set of `BaseGroup.GroupType`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      params.insert(nx.DataGroupSelectionParameter('example_data_group_selection_key', 'DataGroupSelectionParameter', 'Example data group selection help text', nx.DataPath([]), set([nx.BaseGroup.GroupType.DataGroup])))

.. _DataObjectNameParameter:
.. py:class:: DataObjectNameParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataObjectNameParameter(name: str, human_name: str, help_text: str, default_value: str) -> None

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

      import simplnx as nx

      params.insert(nx.DataObjectNameParameter('data_object_name_key', "DataObjectNameParameter", "Example help text for DataObjectNameParameter", "Data Group"))

.. _DataPathSelectionParameter:
.. py:class:: DataPathSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataPathSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath) -> None

   Description
   ~~~~~~~~~~~
   The ``DataPathSelectionParameter`` is used for selecting a :ref:`DataPath <DataPath>` to a data object within the :ref:`DataStructure<DataStructure>`.

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
      - **Description**: The default DataPath to be selected.
      - **Type**: `DataPath`

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      params.insert(nx.DataPathSelectionParameter('example_data_path_key', 'DataPathSelectionParameter', 'Example data path selection help text', nx.DataPath([])))

.. _DataStoreFormatParameter:
.. py:class:: DataStoreFormatParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataStoreFormatParameter(name: str, human_name: str, help_text: str, default_value: str) -> None

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

      import simplnx as nx

      params.insert(nx.DataStoreFormatParameter('data_store_format_key', 'Data Store Format', 'This value will specify which data format is used by the array\'s data store. An empty string results in in-memory data store.', ""))

.. _DataTypeParameter:
.. py:class:: DataTypeParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DataTypeParameter(name: str, human_name: str, help_text: str, default_value: DataType) -> None

   Description
   ~~~~~~~~~~~
   The ``DataTypeParameter`` is used to specify the type of data for a particular operation or data structure element.

   This parameter holds an enumeration value that represents the numerical type for created arrays. The possible values are:

   .. code:: python

      nx.DataType.int8
      nx.DataType.uint8
      nx.DataType.int16
      nx.DataType.uint16
      nx.DataType.int32
      nx.DataType.uint32
      nx.DataType.int64
      nx.DataType.uint64
      nx.DataType.float32
      nx.DataType.float64
      nx.DataType.boolean

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

      import simplnx as nx

      params.insert(nx.DataTypeParameter('data_type_key', "Data Type", "Example data type help text", nx.DataType.float64))

.. _Dream3dImportParameter:
.. py:class:: Dream3dImportParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      Dream3dImportParameter(name: str, human_name: str, help_text: str, default_value: Dream3dImportParameter.ImportData) -> None

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
         - **data_paths**: List of :ref:`DataPath <DataPath>` objects. Use the python 'None' value to indicate that you want to read **ALL** the data from file.

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      import_data = nx.Dream3dImportParameter.ImportData()
      import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
      import_data.data_paths = None
      params.insert(nx.Dream3dImportParameter('import_file_path_key', "Import File Path", "The HDF5 file path the DataStructure should be imported from.", import_data))

.. _DynamicTableParameter:
.. py:class:: DynamicTableParameter

   Declarations
   ~~~~~~~~~~~~
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

      import simplnx as nx

      default_table = [[10, 20], [30, 40]]
      row_info = nx.DynamicTableInfo.DynamicVectorInfo(0, "Row {}")
      col_info = nx.DynamicTableInfo.DynamicVectorInfo(2, "Col {}")
      dynamic_table_info = nx.DynamicTableInfo(nx.DynamicTableInfo.VectorInfo(row_info), nx.DynamicTableInfo.VectorInfo(col_info))
      params.insert(nx.DynamicTableParameter('dynamic_table', 'DynamicTableParameter', 'DynamicTableParameter Example Help Text', default_table, dynamic_table_info))

.. _EnsembleInfoParameter:
.. py:class:: EnsembleInfoParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      EnsembleInfoParameter(name: str, human_name: str, help_text: str, default_value) -> None

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

      import simplnx as nx

      ensemble_info = []
      ensemble_info.append(["Hexagonal-High 6/mmm","Primary","Phase 1"])
      ensemble_info.append(["Cubic-High m-3m","Primary","Phase 2"])
      params.insert(nx.EnsembleInfoParameter('created_ensemble_info_key', "Created Ensemble Info", "The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase.", ensemble_info))

.. _FileSystemPathParameter:
.. py:class:: FileSystemPathParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      FileSystemPathParameter(name: str, human_name: str, help_text: str, default_value: os.PathLike, extensions_type: Set[str], path_type: FileSystemPathParameter.PathType, accept_all_extensions: bool = ...) -> None

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

      import simplnx as nx

      params.insert(nx.FileSystemPathParameter('input_dir', 'Input Directory', 'Example input directory help text', 'Data', set(), nx.FileSystemPathParameter.PathType.InputDir))
      params.insert(nx.FileSystemPathParameter('input_file', 'Input File', 'Example input file help text', '/opt/local/bin/ninja', set(), nx.FileSystemPathParameter.PathType.InputFile, True))
      params.insert(nx.FileSystemPathParameter('output_dir', 'Output Directory', 'Example output directory help text', 'Output Data', set(), nx.FileSystemPathParameter.PathType.OutputDir))
      params.insert(nx.FileSystemPathParameter('output_file', 'Output File', 'Example output file help text', '', set(), nx.FileSystemPathParameter.PathType.OutputFile))

.. _CreateColorMapParameter:
.. py:class:: CreateColorMapParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      CreateColorMapParameter(name: str, human_name: str, help_text: str, default_value: str) -> None

   Description
   ~~~~~~~~~~~
   
   The ``CreateColorMapParameter`` is used to specify parameters for generating color tables, typically used in visualization or data representation.

   This parameter is used specifically for the  :ref:`simplnx.CreateColorMapFilter() <CreateColorMapFilter>` filter.

   These are the color table presets:
   
   - "Rainbow Desaturated"
   - "Cold and Hot"
   - "Black-Body Radiation"
   - "X Ray"
   - "Grayscale"
   - "Black, Blue and White"
   - "Black, Orange and White"
   - "Rainbow Blended White"
   - "Rainbow Blended Grey"
   - "Rainbow Blended Black"
   - "Blue to Yellow"
   - "jet"
   - "rainbow"
   - "Haze"
   - "hsv"

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
      - **Description**: The name of the color table preset to use.
      - **Type**: string (str)
   
   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      params.insert(nx.CreateColorMapParameter('color_table_preset_key', "Select Color Preset...", "Select a preset color name.", "Cool to Warm"))

.. _GeneratedFileListParameter:
.. py:class:: GeneratedFileListParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      GeneratedFileListParameter(name: str, human_name: str, help_text: str, default_value: GeneratedFileListParameter.ValueType) -> None

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

      import simplnx as nx

      def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
         params = nx.Parameters()
         generated_file_list_value = nx.GeneratedFileListParameter.ValueType()
         generated_file_list_value.input_path = "DREAM3DNXData/Data/Porosity_Image"
         generated_file_list_value.ordering = nx.GeneratedFileListParameter.Ordering.LowToHigh
         generated_file_list_value.file_prefix = "slice-"
         generated_file_list_value.file_suffix = ""
         generated_file_list_value.file_extension = ".tif"
         generated_file_list_value.start_index = 11
         generated_file_list_value.end_index = 174
         generated_file_list_value.increment_index = 1
         generated_file_list_value.padding_digits = 2
         params.insert(nx.GeneratedFileListParameter(FilterClassName.INPUT_FILE_LIST_KEY, "Input File List", "The list of files to be read", generated_file_list_value))

      def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:

         file_list: nx.GeneratedFileListParameter.ValueType = [FilterClassName.INPUT_FILE_LIST_KEY].generate()
         for file in file_list:
            print(f'{file}')

.. _GeometrySelectionParameter:
.. py:class:: GeometrySelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      GeometrySelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[IGeometry.Type]) -> None

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

   - ``default_value`` : :ref:`DataPath <DataPath>`
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

      import simplnx as nx

      params.insert(nx.GeometrySelectionParameter('example_geometry_selection_parameter', 'GeometrySelectionParameter', 'Example geometry selection help text', nx.DataPath([]), set([nx.IGeometry.Type.Image, nx.IGeometry.Type.RectGrid])))

.. _ReadCSVFileParameter:
.. py:class:: ReadCSVFileParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      ReadCSVFileParameter(name: str, human_name: str, help_text: str, default_value: ReadCSVDataParameter) -> None

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
         - **Description**: The data types of the columns in the CSV file. Indicates the kind of native numerical values (int, float... ) that will be used in the created  :ref:`DataArray <DataArray>`.
         - **Type**: List of :ref:`nx.DataType<DataTypeParameter>`

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

      import simplnx as nx
      
      # Example File has 7 columns to import
      read_csv_data = nx.ReadCSVDataParameter()
      read_csv_data.input_file_path = "/tmp/test_csv_data.csv"
      read_csv_data.start_import_row = 2
      read_csv_data.delimiters = [',']
      read_csv_data.custom_headers = []
      read_csv_data.column_data_types = [nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.int32]
      read_csv_data.skipped_array_mask = [False,False,False,False,False,False,False]
      read_csv_data.tuple_dims = [37989]
      read_csv_data.headers_line = 1
      read_csv_data.header_mode = nx.ReadCSVDataParameter.HeaderMode.Line
      params.insert(nx.ReadCSVFileParameter('csv_importer_data_key', "CSV Importer Data", "Holds all relevant csv file data collected from the custom interface", read_csv_data))

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

      ReadHDF5DatasetParameter(name: str, human_name: str, help_text: str, default_value: ReadHDF5DatasetParameter.ValueType) -> None

   Description
   ~~~~~~~~~~~
   This parameter is used for the :ref:`simplnx.ReadHDF5DatasetFilter<ReadHDF5DatasetFilter>` and holds the information to import specific data sets from within the HDF5 file into DREAM3D/simplnx

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
         - **Description**: The :ref:`DataPath <DataPath>` object to a parent group to create the :ref:`DataArrays <DataArray>` into. If left blank, the :ref:`DataArray <DataArray>` will be created at the top level of the :ref:`DataStructure<DataStructure>`.

   Usage
   ~~~~~~

   .. code-block:: python

      import simplnx as nx

      dataset1 = nx.ReadHDF5DatasetParameter.DatasetImportInfo()
      dataset1.dataset_path = "/DataStructure/DataContainer/Cell Data/Confidence Index"
      dataset1.tuple_dims = "117,201,189"
      dataset1.component_dims = "1"

      dataset2 = nx.ReadHDF5DatasetParameter.DatasetImportInfo()
      dataset2.dataset_path = "/DataStructure/DataContainer/Cell Data/EulerAngles"
      dataset2.tuple_dims = "117,201,189"
      dataset2.component_dims = "3"

      import_hdf5_param = nx.ReadHDF5DatasetParameter.ValueType()
      import_hdf5_param.input_file = "SmallIN100_Final.dream3d"
      import_hdf5_param.datasets = [dataset1, dataset2]

      params.insert(nx.ReadHDF5DatasetParameter('hdf5_file_key', "Select HDF5 File", "The HDF5 file data to import", import_hdf5_param))

.. _MultiArraySelectionParameter:
.. py:class:: MultiArraySelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      MultiArraySelectionParameter(name: str, human_name: str, help_text: str, default_value: List[DataPath], allowed_types: Set[IArray.ArrayType], allowed_data_types: Set[DataType], required_comps: List[List[int]] = ...) -> None

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

      import simplnx as nx

      params.insert(nx.MultiArraySelectionParameter(ExampleFilter2.PARAM12_KEY, 'MultiArraySelectionParameter', 'Example multiarray selection help text', [], set([nx.IArray.ArrayType.Any]), nx.get_all_data_types(), [[1]]))


.. _MultiPathSelectionParameter:
.. py:class:: MultiPathSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      MultiPathSelectionParameter(name: str, human_name: str, help_text: str, default_value: List[DataPath]) -> None

   Description
   ~~~~~~~~~~~
   This parameter represents a list of :ref:`DataPath <DataPath>` objects. The end point of each :ref:`DataPath <DataPath>` object can be any object in the  :ref:`DataStructure<DataStructure>`

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

      import simplnx as nx
      
      params.insert(nx.MultiPathSelectionParameter('objects_to_copy_key', "Objects to copy", "A list of DataPaths to the DataObjects to be copied", [nx.DataPath("Small IN100/Scan Data/Confidence Index"), nx.DataPath("Small IN100/Scan Data/Euler Angles")]))

.. _NeighborListSelectionParameter:
.. py:class:: NeighborListSelectionParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      NeighborListSelectionParameter(name: str, human_name: str, help_text: str, default_value: DataPath, allowed_types: Set[DataType]) -> None

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

      import simplnx as nx
      
      params.insert(nx.NeighborListSelectionParameter('neighbor_list_key', "Neighbor List", "List of the contiguous neighboring Features for a given Feature", nx.DataPath([]), set([nx.DataType.int32])))

.. _NumericTypeParameter:
.. py:class:: NumericTypeParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      NumericTypeParameter(name: str, human_name: str, help_text: str, default_value: NumericType) -> None

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

      import simplnx as nx

      params.insert(nx.NumericTypeParameter('numeric_type_key', 'Numeric Type', 'Example numeric type help text', nx.NumericType.int32))

.. _StringParameter:
.. py:class:: StringParameter

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      StringParameter(name: str, human_name: str, help_text: str, default_value: str) -> None

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

      import simplnx as nx

      params.insert(nx.StringParameter('string_key', 'StringParameter', 'Example string help text', 'Example String'))

Numerical Parameters
--------------------

Declarations
~~~~~~~~~~~~

   .. code-block:: python

      Int8Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      UInt8Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      Int16Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      UInt16Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      Int32Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      UInt32Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      Int64Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      UInt64Parameter(name: str, human_name: str, help_text: str, default_value: int) -> None

      Float32Parameter(name: str, human_name: str, help_text: str, default_value: float) -> None

      Float64Parameter(name: str, human_name: str, help_text: str, default_value: float) -> None

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

      import simplnx as nx

      params.insert(nx.Float32Parameter('float32_key', 'Float32Parameter', 'The 1st parameter', 0.1234))
      params.insert(nx.Int32Parameter('int32_key', 'Int32Parameter', 'The 2nd parameter', 0))

Numerical Vector Parameters
---------------------------

Declarations
~~~~~~~~~~~~

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

      import simplnx as nx

      params.insert(nx.VectorInt32Parameter('3d_dimensions_key', '3D Dimensions', 'Example int32 vector help text', [-19, -100, 456], ["X", "Y", "Z"]))
      params.insert(nx.VectorFloat64Parameter('quaternion_key', 'Quaternion', 'Example float64 vector help text', [0, 84.98, 234.12, 985.98], ["U", "V", "W", "X"]))

Preflight Actions
-----------------

Preflight actions perform modifications to the DataStructure before executing the filter.  Some examples include:

+ Creating a data array
+ Creating a geometry
+ Deleting a data object
+ Moving a data object

The following is an example that shows how to create an action and append it to the OutputActions object that gets returned by preflight.  In this example, we are deleting an array from the DataStructure.

.. code-block:: python

   import simplnx as nx

   output_actions = nx.OutputActions()
   output_actions.append_action(nx.DeleteDataAction(nx.DataPath('Path/To/Data'), nx.DeleteDataAction.DeleteType.JustObject))

Preflight actions can also be deferred, which means that they are applied after the filter is finished executing, instead of before execution.  An example of when using a deferred action might be useful is if the filter developer needs a data object to exist during filter execution, but does not want that data object to exist afterwards.  In that case, the filter developer could write the following code:

.. code-block:: python

   import simplnx as nx

   output_actions = nx.OutputActions()
   output_actions.append_deferred_action(nx.DeleteDataAction(nx.DataPath('Path/To/Data'), nx.DeleteDataAction.DeleteType.JustObject))

There are two main types of Preflight Actions:

1. Creation Actions
2. Modification Actions

Creation Actions
~~~~~~~~~~~~~~~~

.. _CreateImageGeometryAction:
.. py:class:: CreateImageGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateImageGeometryAction(path: nx.DataPath, dims: list[int], origin: list[float], spacing: list[float], cell_attribute_matrix_name: str) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateImageGeometryAction`` is used to create a structured grid or image geometry, specifying dimensions, origin, and spacing, with an attribute matrix that can store cell data arrays.

   Inputs
   ~~~~~~

   - ``path``
      - **Description**: The path where the image geometry will be created.
      - **Type**: nx.DataPath
   - ``dims``
      - **Description**: Dimensions of the image geometry, given as [width, height, depth].
      - **Type**: list[int]
   - ``origin``
      - **Description**: Origin of the image geometry coordinate system, typically [x, y, z].
      - **Type**: list[float]
   - ``spacing``
      - **Description**: Spacing between elements in the image geometry, for each dimension.
      - **Type**: list[float]
   - ``cell_attribute_matrix_name``
      - **Description**: Name for the cell attribute matrix associated with the image geometry.
      - **Type**: str

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Create image geometry with specified dimensions, origin, and spacing at /Image Geometry
      geom_path = nx.DataPath('Image Geometry')
      dims = [256, 256, 100]
      origin = [0.0, 0.0, 0.0]
      spacing = [0.75, 0.75, 1.0]
      cell_matrix_name = 'Cell Data'

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateImageGeometryAction(geom_path, dims, origin, spacing, cell_matrix_name))

.. _CreateRectGridGeometryAction:
.. py:class:: CreateRectGridGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateRectGridGeometryAction(path: DataPath, x_bounds_dim: int, y_bounds_dim: int, z_bounds_dim: int, cell_attribute_matrix_name: str, x_bounds_name: str, y_bounds_name: str, z_bounds_name: str) -> None
      CreateRectGridGeometryAction(path: DataPath, input_x_bounds_path: DataPath, input_y_bounds_path: DataPath, input_z_bounds_path: DataPath, cell_attribute_matrix_name: str, array_type: IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateRectGridGeometryAction`` is intended to create rectilinear grid geometries either by specifying dimensions and boundary array names or by utilizing existing boundary arrays.

   Inputs
   ~~~~~~

   First Constructor:

   - ``path``
      - **Description**: The path where the rectilinear grid geometry will be created.
      - **Type**: DataPath
   - ``x_bounds_dim``
      - **Description**: The number of divisions along the X dimension.
      - **Type**: int
   - ``y_bounds_dim``
      - **Description**: The number of divisions along the Y dimension.
      - **Type**: int
   - ``z_bounds_dim``
      - **Description**: The number of divisions along the Z dimension.
      - **Type**: int
   - ``cell_attribute_matrix_name``
      - **Description**: Name for the newly created cell attribute matrix.
      - **Type**: str
   - ``x_bounds_name``
      - **Description**: Name for the X boundary array.
      - **Type**: str
   - ``y_bounds_name``
      - **Description**: Name for the Y boundary array.
      - **Type**: str
   - ``z_bounds_name``
      - **Description**: Name for the Z boundary array.
      - **Type**: str

   Second Constructor:

   - ``path``
      - **Description**: The path where the rectilinear grid geometry will be created.
      - **Type**: DataPath
   - ``input_x_bounds_path``
      - **Description**: The path to the input X boundary array.
      - **Type**: DataPath
   - ``input_y_bounds_path``
      - **Description**: The path to the input Y boundary array.
      - **Type**: DataPath
   - ``input_z_bounds_path``
      - **Description**: The path to the input Z boundary array.
      - **Type**: DataPath
   - ``cell_attribute_matrix_name``
      - **Description**: The name for the newly created cell attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input arrays should be handled when creating new arrays in the rectilinear grid geometry. Possible values are Copy, Move, Reference, or Create.
      - **Type**: IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example using the first constructor to create a rectangular grid geometry
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateRectGridGeometryAction(nx.DataPath('Rect Grid Geometry'), 10, 20, 30, 'Cell Matrix', 'X Bounds', 'Y Bounds', 'Z Bounds'))

      # Example using the second constructor to create a rectangular grid geometry using existing arrays
      x_bounds_path = DataPath(['Other Rect Grid Geometry', 'X Bounds'])
      y_bounds_path = DataPath(['Other Rect Grid Geometry', 'Y Bounds'])
      z_bounds_path = DataPath(['Other Rect Grid Geometry', 'Z Bounds'])
      output_actions.append_action(nx.CreateRectGridGeometryAction(nx.DataPath('Rect Grid Geometry'), x_bounds_path, y_bounds_path, z_bounds_path, 'Cell Matrix', nx.IDataCreationAction.ArrayHandlingType.Copy))

.. _CreateVertexGeometryAction:
.. py:class:: CreateVertexGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateVertexGeometryAction(geometry_path: DataPath, num_vertices: int, vertex_attribute_matrix_name: str, shared_vertex_list_name: str) -> None
      CreateVertexGeometryAction(geometry_path: DataPath, input_vertices_array_path: DataPath, vertex_attribute_matrix_name: str, array_type: IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateVertexGeometryAction`` is used to create a vertex geometry within the data structure, either by specifying the number of vertices and a vertex data array name, or by utilizing an existing vertex data array.

   Inputs
   ~~~~~~

   First Constructor:

   - ``geometry_path``
      - **Description**: The path where the vertex geometry will be created.
      - **Type**: nx.DataPath
   - ``num_vertices``
      - **Description**: Number of vertices to create.
      - **Type**: int
   - ``vertex_attribute_matrix_name``
      - **Description**: Name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``shared_vertex_list_name``
      - **Description**: Name for the newly created shared vertex list.
      - **Type**: str

   Second Constructor:

   - ``geometry_path``
      - **Description**: The path where the vertex geometry will be created.
      - **Type**: nx.DataPath
   - ``input_vertices_array_path``
      - **Description**: The path to the input vertex array.
      - **Type**: nx.DataPath
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input array should be handled when creating new arrays in the vertex geometry. Possible values are Copy, Move, Reference, or Create.
      - **Type**: IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example using the first constructor to create a vertex geometry
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateVertexGeometryAction(nx.DataPath('Vertex Geometry'), 1000, 'Vertex Matrix', 'Shared Vertex List'))

      # Example using the second constructor to create a vertex geometry using an existing array
      vertices_path = nx.DataPath('Other Vertex Geometry/Vertices')
      output_actions.append_action(nx.CreateVertexGeometryAction(nx.DataPath('Vertex Geometry'), vertices_path, 'Vertex Matrix', nx.IDataCreationAction.ArrayHandlingType.Reference))

.. _CreateEdgeGeometryAction:
.. py:class:: CreateEdgeGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateEdgeGeometryAction(geometry_path: nx.DataPath, num_edges: int, num_vertices: int, vertex_attribute_matrix_name: str, edge_attribute_matrix_name: str, shared_vertices_name: str, shared_edges_name: str) -> None
      CreateEdgeGeometryAction(geometry_path: nx.DataPath, input_vertices_array_path: nx.DataPath, input_edges_array_path: nx.DataPath, vertex_attribute_matrix_name: str, edge_attribute_matrix_name: str, array_type: nx.IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateEdgeGeometryAction`` is used to create an edge geometry in a data structure. There are two ways to create the geometry:
   
   1. Specifying the number of edges and vertices along with attribute matrix and data array names (the vertex and edge arrays will be created for you).
   
   2. Using existing arrays for vertices and edges and including an array handling type (Copy, Move, Reference, or Create) that determines how these existing arrays are handled by the action.

   Inputs
   ~~~~~~

   First Constructor:

   - ``geometry_path``
      - **Description**: The path where the edge geometry will be created.
      - **Type**: nx.DataPath
   - ``num_edges``
      - **Description**: The number of edges to be created.
      - **Type**: int
   - ``num_vertices``
      - **Description**: The number of vertices to be created.
      - **Type**: int
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``edge_attribute_matrix_name``
      - **Description**: The name for the newly created edge attribute matrix.
      - **Type**: str
   - ``shared_vertices_name``
      - **Description**: The name for the newly created vertices array.
      - **Type**: str
   - ``shared_edges_name``
      - **Description**: The name for the newly created edges array.
      - **Type**: str

   Second Constructor:

   - ``geometry_path``
      - **Description**: The path where the edge geometry will be created.
      - **Type**: nx.DataPath
   - ``input_vertices_array_path``
      - **Description**: The path to the input array of vertices.
      - **Type**: nx.DataPath
   - ``input_edges_array_path``
      - **Description**: The path to the input array of edges.
      - **Type**: nx.DataPath
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``edge_attribute_matrix_name``
      - **Description**: The name for the newly created edge attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input arrays should be handled when creating new arrays in the edge geometry.  Possible values are Copy, Move, Reference, or Create.
      - **Type**: nx.IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Example using the first constructor
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateEdgeGeometryAction(nx.DataPath('Edge Geometry'), 100, 200, 'Vertex Matrix', 'Edge Matrix', 'Vertices', 'Edges'))

      # Example using the second constructor
      vertices_path = nx.DataPath('Other Edge Geometry/Vertices')
      edges_path = nx.DataPath('Other Edge Geometry/Edges')
      output_actions.append_action(nx.CreateEdgeGeometryAction(nx.DataPath('Edge Geometry'), vertices_path, edges_path, 'Vertex Matrix', 'Edge Matrix', nx.IDataCreationAction.ArrayHandlingType.Copy))

.. _CreateTriangleGeometryAction:
.. py:class:: CreateTriangleGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateTriangleGeometryAction(geometry_path: DataPath, num_faces: int, num_vertices: int, vertex_attribute_matrix_name: str, face_attribute_matrix_name: str, shared_vertices_name: str, shared_faces_name: str) -> None
      CreateTriangleGeometryAction(geometry_path: DataPath, input_vertices_array_path: DataPath, input_faces_array_path: DataPath, vertex_attribute_matrix_name: str, face_attribute_matrix_name: str, array_type: IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateTriangleGeometryAction`` is designed to create a triangle geometry within the data structure, either by specifying the counts of faces and vertices along with their data names, or by directly using existing vertex and face arrays.

   Inputs
   ~~~~~~

   First Constructor:

   - ``geometry_path``
      - **Description**: The path where the triangular geometry will be created.
      - **Type**: nx.DataPath
   - ``num_faces``
      - **Description**: Number of triangular faces to create.
      - **Type**: int
   - ``num_vertices``
      - **Description**: Number of vertices to create.
      - **Type**: int
   - ``vertex_attribute_matrix_name``
      - **Description**: Name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``face_attribute_matrix_name``
      - **Description**: Name for the newly created face attribute matrix.
      - **Type**: str
   - ``shared_vertices_name``
      - **Description**: Name for the newly created shared vertices array.
      - **Type**: str
   - ``shared_faces_name``
      - **Description**: Name for the newly created shared faces array.
      - **Type**: str

   Second Constructor:

   - ``geometry_path``
      - **Description**: The path where the triangular geometry will be created.
      - **Type**: nx.DataPath
   - ``input_vertices_array_path``
      - **Description**: The path to the input vertex array.
      - **Type**: nx.DataPath
   - ``input_faces_array_path``
      - **Description**: The path to the input face array.
      - **Type**: nx.DataPath
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``face_attribute_matrix_name``
      - **Description**: The name for the newly created face attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input arrays should be handled when creating new arrays in the triangular geometry. Possible values are Copy, Move, Reference, or Create.
      - **Type**: IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example using the first constructor to create a triangle geometry
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateTriangleGeometryAction(nx.DataPath('Triangle Geometry'), 150, 300, 'Vertex Matrix', 'Face Matrix', 'Shared Vertices', 'Shared Faces'))

      # Example using the second constructor to create a triangle geometry using existing arrays
      vertices_path = nx.DataPath('Other Triangle Geometry/Vertices')
      faces_path = nx.DataPath('Other Triangle Geometry/Faces')
      output_actions.append_action(nx.CreateTriangleGeometryAction(nx.DataPath('Triangle Geometry'), vertices_path, faces_path, 'Vertex Matrix', 'Face Matrix', nx.IDataCreationAction.ArrayHandlingType.Move))

.. _CreateQuadGeometryAction:
.. py:class:: CreateQuadGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateQuadGeometryAction(geometry_path: DataPath, num_faces: int, num_vertices: int, vertex_attribute_matrix_name: str, face_attribute_matrix_name: str, shared_vertices_name: str, shared_faces_name: str) -> None
      CreateQuadGeometryAction(geometry_path: DataPath, input_vertices_array_path: DataPath, input_faces_array_path: DataPath, vertex_attribute_matrix_name: str, face_attribute_matrix_name: str, array_type: IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateQuadGeometryAction`` is designed to create a quad geometry within the data structure, either by specifying the counts of faces and vertices along with associated attribute matrix and data names, or by directly using existing vertex and face arrays.

   Inputs
   ~~~~~~

   First Constructor:

   - ``geometry_path``
      - **Description**: The path where the quad geometry will be created.
      - **Type**: DataPath
   - ``num_faces``
      - **Description**: Number of quad faces to create.
      - **Type**: int
   - ``num_vertices``
      - **Description**: Number of vertices to create.
      - **Type**: int
   - ``vertex_attribute_matrix_name``
      - **Description**: Name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``face_attribute_matrix_name``
      - **Description**: Name for the newly created face attribute matrix.
      - **Type**: str
   - ``shared_vertices_name``
      - **Description**: Name for the newly created shared vertices array.
      - **Type**: str
   - ``shared_faces_name``
      - **Description**: Name for the newly created shared faces array.
      - **Type**: str

   Second Constructor:

   - ``geometry_path``
      - **Description**: The path where the quad geometry will be created.
      - **Type**: DataPath
   - ``input_vertices_array_path``
      - **Description**: The path to the input vertex array.
      - **Type**: DataPath
   - ``input_faces_array_path``
      - **Description**: The path to the input face array.
      - **Type**: DataPath
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``face_attribute_matrix_name``
      - **Description**: The name for the newly created face attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input arrays should be handled when creating new arrays in the quad geometry. Possible values are Copy, Move, Reference, or Create.
      - **Type**: IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example using the first constructor to create a quad geometry
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateQuadGeometryAction(nx.DataPath('Quad Geometry'), 50, 100, 'Vertex Matrix', 'Face Matrix', 'Shared Vertices', 'Shared Faces'))

      # Example using the second constructor to create a quad geometry using existing arrays
      vertices_path = DataPath(['Other Quad Geometry', 'Vertices'])
      faces_path = DataPath(['Other Quad Geometry', 'Faces'])
      output_actions.append_action(nx.CreateQuadGeometryAction(nx.DataPath('Quad Geometry'), vertices_path, faces_path, 'Vertex Matrix', 'Face Matrix', nx.IDataCreationAction.ArrayHandlingType.Copy))

.. _CreateHexahedralGeometryAction:
.. py:class:: CreateHexahedralGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateHexahedralGeometryAction(geometry_path: nx.DataPath, num_cells: int, num_vertices: int, vertex_data_name: str, cell_data_name: str, shared_vertices_name: str, shared_cells_name: str) -> None
      CreateHexahedralGeometryAction(geometry_path: nx.DataPath, input_vertices_array_path: nx.DataPath, input_cell_array_path: nx.DataPath, vertex_attribute_matrix_name: str, cell_attribute_matrix_name: str, array_type: nx.IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateHexahedralGeometryAction`` is designed to create a hexahedral geometry within the data structure, either by specifying the counts of cells and vertices, or by directly using existing vertex and cell arrays.

   Inputs
   ~~~~~~

   First Constructor:

   - ``geometry_path``
      - **Description**: The path where the hexahedral geometry will be created.
      - **Type**: nx.DataPath
   - ``num_cells``
      - **Description**: Number of hexahedral cells to create.
      - **Type**: int
   - ``num_vertices``
      - **Description**: Number of vertices to create.
      - **Type**: int
   - ``vertex_data_name``
      - **Description**: Name for the newly created vertex data array.
      - **Type**: str
   - ``cell_data_name``
      - **Description**: Name for the newly created cell data array.
      - **Type**: str
   - ``shared_vertices_name``
      - **Description**: Name for the newly created vertex array.
      - **Type**: str
   - ``shared_cells_name``
      - **Description**: Name for the newly created cells array.
      - **Type**: str

   Second Constructor:

   - ``geometry_path``
      - **Description**: The path where the hexahedral geometry will be created.
      - **Type**: nx.DataPath
   - ``input_vertices_array_path``
      - **Description**: The path to the input vertex array.
      - **Type**: nx.DataPath
   - ``input_cell_array_path``
      - **Description**: The path to the input cell array.
      - **Type**: nx.DataPath
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``cell_attribute_matrix_name``
      - **Description**: The name for the newly created cell attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input arrays should be handled when creating new arrays in the hexahedral geometry.  Possible values are Copy, Move, Reference, or Create.
      - **Type**: nx.IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Example using the first constructor to create a hexahedral geometry
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateHexahedralGeometryAction(nx.DataPath('Hexahedral Geometry'), 100, 200, 'Vertex Data', 'Cell Data', 'Shared Vertices', 'Shared Hexahedrals'))

      # Example using the second constructor to create a hexahedral geometry using existing arrays
      vertices_path = nx.DataPath('Other Hexahedral Geometry/Vertices')
      cells_path = nx.DataPath('Other Hexahedral Geometry/Hexahedrals')
      output_actions.append_action(nx.CreateHexahedralGeometryAction(nx.DataPath('Hexahedral Geometry'), vertices_path, cells_path, 'Vertex Matrix', 'Cell Matrix', nx.IDataCreationAction.ArrayHandlingType.Copy))

.. _CreateTetrahedralGeometryAction:
.. py:class:: CreateTetrahedralGeometryAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateTetrahedralGeometryAction(geometry_path: DataPath, num_cells: int, num_vertices: int, vertex_data_name: str, cell_data_name: str, shared_vertices_name: str, shared_cells_name: str) -> None
      CreateTetrahedralGeometryAction(geometry_path: DataPath, input_vertices_array_path: DataPath, input_cell_array_path: DataPath, vertex_attribute_matrix_name: str, cell_attribute_matrix_name: str, array_type: IDataCreationAction.ArrayHandlingType) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateTetrahedralGeometryAction`` creates a tetrahedral geometry within the data structure, either by specifying the counts of cells and vertices along with cell and vertex data names, or by directly using existing vertex and cell arrays.

   Inputs
   ~~~~~~

   First Constructor:

   - ``geometry_path``
      - **Description**: The path where the tetrahedral geometry will be created.
      - **Type**: DataPath
   - ``num_cells``
      - **Description**: Number of tetrahedral cells to create.
      - **Type**: int
   - ``num_vertices``
      - **Description**: Number of vertices to create.
      - **Type**: int
   - ``vertex_data_name``
      - **Description**: Name for the newly created vertex data array.
      - **Type**: str
   - ``cell_data_name``
      - **Description**: Name for the newly created cell data array.
      - **Type**: str
   - ``shared_vertices_name``
      - **Description**: Name for the newly created shared vertices array.
      - **Type**: str
   - ``shared_cells_name``
      - **Description**: Name for the newly created shared cells array.
      - **Type**: str

   Second Constructor:

   - ``geometry_path``
      - **Description**: The path where the tetrahedral geometry will be created.
      - **Type**: DataPath
   - ``input_vertices_array_path``
      - **Description**: The path to the input vertex array.
      - **Type**: DataPath
   - ``input_cell_array_path``
      - **Description**: The path to the input cell array.
      - **Type**: DataPath
   - ``vertex_attribute_matrix_name``
      - **Description**: The name for the newly created vertex attribute matrix.
      - **Type**: str
   - ``cell_attribute_matrix_name``
      - **Description**: The name for the newly created cell attribute matrix.
      - **Type**: str
   - ``array_type``
      - **Description**: Specifies how the input arrays should be handled when creating new arrays in the tetrahedral geometry. Possible values are Copy, Move, Reference, or Create.
      - **Type**: IDataCreationAction.ArrayHandlingType

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example using the first constructor to create a tetrahedral geometry
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateTetrahedralGeometryAction(nx.DataPath('Tetrahedral Geometry'), 500, 1000, 'Vertex Data', 'Cell Data', 'Shared Vertices', 'Shared Tetrahedrals'))

      # Example using the second constructor to create a tetrahedral geometry using existing arrays
      vertices_path = DataPath(['Other Tetrahedral Geometry', 'Vertices'])
      cells_path = DataPath(['Other Tetrahedral Geometry', 'Tetrahedrals'])
      output_actions.append_action(nx.CreateTetrahedralGeometryAction(nx.DataPath('Tetrahedral Geometry'), vertices_path, cells_path, 'Vertex Matrix', 'Cell Matrix', nx.IDataCreationAction.ArrayHandlingType.Copy))

.. _CreateArrayAction:
.. py:class:: CreateArrayAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateArrayAction(type: nx.DataType, t_dims: list[int], c_dims: list[int], path: nx.DataPath, data_format: str = ...) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateArrayAction`` is used to create a new array with specific data type, tuple dimensions, and component dimensions at a specified path within the data structure.

   Inputs
   ~~~~~~

   - ``type``
      - **Description**: The data type of the array to be created.
      - **Type**: nx.DataType

   - ``t_dims``
      - **Description**: The tuple dimensions of the array.
      - **Type**: list[int]

   - ``c_dims``
      - **Description**: The component dimensions of the array.
      - **Type**: list[int]

   - ``path``
      - **Description**: The DataPath where the new array will be created.
      - **Type**: nx.DataPath

   - ``data_format``
      - **Description**: Format for the data, either an empty string for an in-memory array or "Zarr" for an out-of-core array.
      - **Type**: str
      - **Default**: Empty string

   Usage
   ~~~~~

   In-memory example:

   .. code-block:: python
      
      import simplnx as nx

      # Create an array with specified dimensions and data type at /Data Array
      dtype = nx.DataType.float32
      t_dims = [100,100]
      c_dims = [3]
      path = nx.DataPath('Data Array')

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateArrayAction(dtype, t_dims, c_dims, path))    # In-memory
   
   Out-of-core:

   .. code-block:: python

      output_actions.append_action(nx.CreateArrayAction(dtype, t_dims, c_dims, path, 'Zarr'))    # Out-of-core

.. _CreateAttributeMatrixAction:
.. py:class:: CreateAttributeMatrixAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateAttributeMatrixAction(path: nx.DataPath, shape: list[int]) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateAttributeMatrixAction`` is used to create an :ref:`AttributeMatrix<AttributeMatrix>` at a specified path with a given shape.

   Inputs
   ~~~~~~

   - ``path``
      - **Description**: The DataPath where the attribute matrix will be created.
      - **Type**: nx.DataPath

   - ``shape``
      - **Description**: The shape of the attribute matrix, defined as a list of integers representing the tuple dimensions.
      - **Type**: list[int]

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Create an attribute matrix at /Image Geometry/Cell Attribute Matrix
      path = nx.DataPath('Image Geometry/Cell Attribute Matrix')
      shape = [100, 200]

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateAttributeMatrixAction(path, shape))

.. _CreateDataGroupAction:
.. py:class:: CreateDataGroupAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateDataGroupAction(path: nx.DataPath) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateDataGroupAction`` is used to create a new data group within the data structure at a specified path.

   Inputs
   ~~~~~~

   - ``path``
      - **Description**: The DataPath where the new data group will be created.
      - **Type**: nx.DataPath

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Create a data group at /Data Group
      path = nx.DataPath('Data Group')

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateDataGroupAction(path))

.. _CreateNeighborListAction:
.. py:class:: CreateNeighborListAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateNeighborListAction(type: nx.DataType, tuple_count: int, path: nx.DataPath) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateNeighborListAction`` is used to create a neighbor list data structure at a specific path by specifying the data type and number of tuples.

   Inputs
   ~~~~~~

   - ``type``
      - **Description**: Data type of the elements in the neighbor list.
      - **Type**: nx.DataType
   - ``tuple_count``
      - **Description**: Number of tuples or entries in the neighbor list.
      - **Type**: int
   - ``path``
      - **Description**: The path where the neighbor list will be stored.
      - **Type**: nx.DataPath

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Create a neighbor list for integer data type with 100 tuples at /Data/Neighbors
      data_type = nx.DataType.int32
      tuple_count = 100
      path = nx.DataPath('Data/Neighbors')

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateNeighborListAction(data_type, tuple_count, path))

.. _CreateStringArrayAction:
.. py:class:: CreateStringArrayAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      CreateStringArrayAction(t_dims: list[int], path: DataPath, initialize_value: str = ...) -> None

   Description
   ~~~~~~~~~~~

   The ``CreateStringArrayAction`` is designed to create a string array at a specified path within the data structure, with the option to initialize all elements with a given value.

   Inputs
   ~~~~~~

   - ``t_dims``
      - **Description**: The tuple dimensions of the string array to be created.
      - **Type**: list[int]
   - ``path``
      - **Description**: The path in the data structure where the string array will be stored.
      - **Type**: DataPath
   - ``initialize_value`` (optional)
      - **Description**: The initial value to set for all elements of the string array.
      - **Type**: str

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example of creating a 2D string array initialized with "foo"
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CreateStringArrayAction([10, 20], nx.DataPath('String Array'), 'foo'))

.. _ImportH5ObjectPathsAction:
.. py:class:: ImportH5ObjectPathsAction

   Declaration
   ~~~~~~~~~~~

   .. code-block:: python

      ImportH5ObjectPathsAction(import_file: os.PathLike, paths: list[DataPath] | None) -> None

   Description
   ~~~~~~~~~~~

   The ``ImportH5ObjectPathsAction`` is used to import specific objects from an HDF5 file into the data structure. This action allows for selective data import based on provided paths.

   Inputs
   ~~~~~~

   - ``import_file``
      - **Description**: The file path of the HDF5 file from which to import data.
      - **Type**: os.PathLike
   - ``paths``
      - **Description**: A list of paths specifying which objects to import from the HDF5 file.  If `None`, all objects will be imported.  If list is empty, nothing will be imported.
      - **Type**: list[nx.DataPath] | None

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx
      import os

      # Example of importing specific object paths from an H5 file
      output_actions = nx.OutputActions()
      h5_file_path = os.path.join('path', 'to', 'file.h5')
      output_actions.append_action(nx.ImportH5ObjectPathsAction(h5_file_path, ['Data/Object1', 'Data/Object2']))

      # Example of importing all objects from an H5 file
      output_actions = nx.OutputActions()
      h5_file_path = os.path.join('path', 'to', 'file.h5')
      output_actions.append_action(nx.ImportH5ObjectPathsAction(h5_file_path, None))

      # Example of importing nothing from an H5 file
      output_actions = nx.OutputActions()
      h5_file_path = os.path.join('path', 'to', 'file.h5')
      output_actions.append_action(nx.ImportH5ObjectPathsAction(h5_file_path, []))

Modification Actions
~~~~~~~~~~~~~~~~~~~~

.. _CopyArrayInstanceAction:
.. py:class:: CopyArrayInstanceAction

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      CopyArrayInstanceAction(selected_data_path: nx.DataPath, created_data_path: nx.DataPath) -> None

   Description
   ~~~~~~~~~~~
   The ``CopyArrayInstanceAction`` is used to create a copy of an array from a selected data path to a newly specified data path.

   The copied array will be created out-of-core if the source array at the selected data path is also out-of-core.

   This action will also check that the copied array has the appropriate tuple dimensions if copied into an Attribute Matrix.

   Inputs
   ~~~~~~
   - ``selected_data_path``
      - **Description**: The DataPath to the array instance that will be copied.
      - **Type**: nx.DataPath

   - ``created_data_path``
      - **Description**: The DataPath to the newly copied array instance.
      - **Type**: nx.DataPath

   Usage
   ~~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Copy the array at /Original/Path to /New/Path
      selected_data_path = nx.DataPath('Original/Path')
      created_data_path = nx.DataPath('New/Path')

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CopyArrayInstanceAction(selected_data_path, created_data_path))


.. _CopyDataObjectAction:
.. py:class:: CopyDataObjectAction

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      CopyDataObjectAction(path: nx.DataPath, new_path: nx.DataPath, all_created_paths: list[nx.DataPath]) -> None

   Description
   ~~~~~~~~~~~
   The ``CopyDataObjectAction`` is used to copy a data object from one path to another within a data structure.

   If copying a Data Group, Attribute Matrix, or Geometry, this action will also recursively copy all children.

   *Note*: For copying data arrays, we recommend using :ref:`CopyArrayInstanceAction <CopyArrayInstanceAction>`.  This action offers enhanced error-checking and superior out-of-core handling tailored specifically for data arrays.

   Inputs
   ~~~~~~
   - ``path``
      - **Description**: The DataPath to the data object that will be copied.
      - **Type**: nx.DataPath

   - ``new_path``
      - **Description**: The DataPath where the copied data object will be placed.
      - **Type**: nx.DataPath

   - ``all_created_paths``
      - **Description**: A list that keeps track of all DataPaths where new data objects are created during the operation.
      - **Type**: list[nx.DataPath]

   Usage
   ~~~~~

   .. code-block:: python
      
      import simplnx as nx

      # Copy the data object from /Original/Path to /New/Path
      path = nx.DataPath('Original/Path')
      new_path = nx.DataPath('New/Path')
      all_created_paths = []

      output_actions = nx.OutputActions()
      output_actions.append_action(nx.CopyDataObjectAction(path, new_path, []))

.. _DeleteDataAction:
.. py:class:: DeleteDataAction

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      DeleteDataAction(path: DataPath, type: DeleteDataAction.DeleteType) -> None

   Description
   ~~~~~~~~~~~
   The ``DeleteDataAction`` is used to delete data within a data structure based on a specified path and deletion type.

   Inputs
   ~~~~~~
   - ``path``
      - **Description**: The path to the data that needs to be deleted.
      - **Type**: nx.DataPath
   - ``type``
      - **Description**: The type of deletion to perform
      - **Type**: DeleteDataAction.DeleteType

   Nested Class: DeleteType
   ~~~~~~~~~~~~~~~~~~~~~~~~
   The ``DeleteType`` is used to determine the type of deletion to perform.  Currently, only the ``JustObject`` type is available, which deletes the specified object and all children.

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example of deleting data using the DeleteDataAction
      output_actions = nx.OutputActions()
      output_actions.append_action(nx.DeleteDataAction(nx.DataPath('Path/To/Data'), nx.DeleteDataAction.DeleteType.JustObject))

.. _MoveDataAction:
.. py:class:: MoveDataAction

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      MoveDataAction(path: DataPath, new_parent_path: DataPath) -> None

   Description
   ~~~~~~~~~~~
   The ``MoveDataAction`` facilitates the relocation of data within the data structure, allowing data at a specified path to be moved to a new parent location.

   Inputs
   ~~~~~~
   - ``path``
      - **Description**: The path to the data that needs to be moved.
      - **Type**: nx.DataPath
   - ``new_parent_path``
      - **Description**: The new parent path where the data should be relocated.
      - **Type**: nx.DataPath

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example of moving data within the data structure
      output_actions = nx.OutputActions()
      data_path = nx.DataPath('Current/Location/Data')
      new_parent = nx.DataPath('New/Location')
      output_actions.append_action(nx.MoveDataAction(data_path, new_parent))

.. _RenameDataAction:
.. py:class:: RenameDataAction

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      RenameDataAction(path: DataPath, new_name: str) -> None

   Description
   ~~~~~~~~~~~
   The ``RenameDataAction`` is used to rename an existing data object within the data structure.

   Inputs
   ~~~~~~
   - ``path``
      - **Description**: The path to the data object that needs to be renamed.
      - **Type**: nx.DataPath
   - ``new_name``
      - **Description**: The new name to assign to the data object.
      - **Type**: str

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example of renaming a data object
      output_actions = nx.OutputActions()
      data_path = nx.DataPath('Current/Name')
      new_data_name = 'New Name'
      output_actions.append_action(nx.RenameDataAction(data_path, new_data_name))

.. _UpdateImageGeomAction:
.. py:class:: UpdateImageGeomAction

   Declaration
   ~~~~~~~~~~~
   .. code-block:: python

      UpdateImageGeomAction(origin, spacing, path: DataPath) -> None

   Description
   ~~~~~~~~~~~
   The ``UpdateImageGeomAction`` modifies the geometry parameters of an image stored in the data structure. It updates the origin and spacing of the image to better align with spatial or resolution requirements.

   Inputs
   ~~~~~~
   - ``origin``
      - **Description**: The new origin coordinates of the image. This should be a tuple of floats indicating the starting point of the image in its coordinate space.
      - **Type**: tuple[float, float, float]
   - ``spacing``
      - **Description**: The new spacing between the pixels (or voxels) of the image. This should be a tuple of floats representing the distance between elements in each dimension.
      - **Type**: tuple[float, float, float]
   - ``path``
      - **Description**: The path to the image geometry that needs updating.
      - **Type**: nx.DataPath

   Usage
   ~~~~~

   .. code-block:: python

      import simplnx as nx

      # Example of updating image geometry
      output_actions = nx.OutputActions()
      image_path = nx.DataPath('Image Geometry')
      new_origin = (0.0, 0.0, 0.0)
      new_spacing = (1.0, 1.0, 1.0)
      output_actions.append_action(nx.UpdateImageGeomAction(new_origin, new_spacing, image_path))
