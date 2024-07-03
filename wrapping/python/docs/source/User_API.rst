SIMPLNX Python API
===================

.. _UserAPIDocs:

Error & Warning Reporting
--------------------------

.. _UserAPI:
.. py:module:: nx


.. _Result:
.. py:class:: IFilter.ExecuteResult

   The object that encapsulates any warnings or errors from either preflighting or executing a simplnx.Filter object.
   It can be queried for the list of errors or warnings and thus printed if needed.

   .. code:: python

      result = cxor.ConvertOrientations.execute(data_structure=data_structure,
                                          input_orientation_array_path=array_path,
                                          input_type=0,
                                          output_orientation_array_name='Quaternions',
                                          output_type=2)
      if len(result.warnings) != 0:
         for w in result.warnings:
            print(f'Warning: ({w.code}) {w.message}')

      has_errors = len(result.errors) != 0
      if has_errors:
         for err in result.errors:
            print(f'Error: ({err.code}) {err.message}')
            raise RuntimeError(result)
      else:
         print(f"{filter.name()} :: No errors running the filter")

Creating Geometries
------------------

All the `simplnx` geometries can be created in Python using the following helper methods:

+ create_image_geometry
+ create_rect_grid_geometry
+ create_vertex_geometry
+ create_edge_geometry
+ create_triangle_geometry
+ create_quad_geometry
+ create_tetrahedral_geometry
+ create_hexahedral_geometry

The enumeration `ArrayHandlingType` defines how existing arrays will be handled when creating a new geometry. It includes the following values:

- **Copy**: The existing arrays will be copied into the new geometry.
- **Move**: The existing arrays will be moved into the new geometry.

Creating An Image Geometry
##########################

To create an image geometry, use the `create_image_geometry` method. This method requires the following parameters:

- `data_structure`: The data structure where the geometry will be created.
- `geometry_path`: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- `dimensions`: A list of dimensions for the image.
- `origin`: The origin coordinates of the image.
- `spacing`: The spacing between elements in the image.
- `cell_attr_matrix_name` (optional): The name of the cell attribute matrix. Defaults to "Cell Data".

Example usage:

.. code:: python

   import simplnx as nx

   # Create an image geometry
   result: nx.Result = nx.create_image_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Image Geometry"]), dimensions=[100, 150, 200], origin=[0, 5, -2], spacing=[0.5, 0.5, 0.5], cell_attr_matrix_name='Image Data')
   if result.valid():
      image_geom = data_structure[nx.DataPath(["Image Geometry"])]
      print("Image Geometry Created!")


Creating A Rectilinear Grid Geometry
####################################

To create a rectilinear grid geometry, use the `create_rect_grid_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **x_bounds_path**: The :ref:`DataPath <DataPath>` to the X bounds array.
- **y_bounds_path**: The :ref:`DataPath <DataPath>` to the Y bounds array.
- **z_bounds_path**: The :ref:`DataPath <DataPath>` to the Z bounds array.
- **cell_attr_matrix_name** (optional): The name of the cell attribute matrix. Defaults to "Cell Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create a rect grid geometry
   result = nx.create_rect_grid_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Rect Grid Geometry"]), x_bounds_path=nx.DataPath(["Foo"]), y_bounds_path=nx.DataPath(["Y Bounds"]), z_bounds_path=nx.DataPath(["Z Bounds"]), cell_attr_matrix_name='Cell Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      rect_grid_geom = data_structure[nx.DataPath(["Rect Grid Geometry"])]
      print("Rect Grid Geometry Created!")

Creating A Vertex Geometry
##########################

To create a vertex geometry, use the `create_vertex_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **vertices_path**: The :ref:`DataPath <DataPath>` to the vertices array.
- **vertex_attr_matrix_name** (optional): The name of the vertex attribute matrix. Defaults to "Vertex Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create a vertex geometry
   result = nx.create_vertex_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Vertex Geometry"]), vertices_path=nx.DataPath("Vertices"), vertex_attr_matrix_name='Vertex Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      vertex_geom = data_structure[nx.DataPath("Vertex Geometry")]
      print("Vertex Geometry Created!")

Creating An Edge Geometry
#########################

To create an edge geometry, use the `create_edge_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **vertices_path**: The :ref:`DataPath <DataPath>` to the vertices array.
- **edge_list_path**: The :ref:`DataPath <DataPath>` to the edge list array.
- **vertex_attr_matrix_name** (optional): The name of the vertex attribute matrix. Defaults to "Vertex Data".
- **edge_attr_matrix_name** (optional): The name of the edge attribute matrix. Defaults to "Edge Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create an edge geometry
   result = nx.create_edge_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Edge Geometry"]), vertices_path=nx.DataPath("Vertices"), edge_list_path=nx.DataPath("Edge List"), vertex_attr_matrix_name='Vertex Data', edge_attr_matrix_name='Edge Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      edge_geom = data_structure[nx.DataPath("Edge Geometry")]
      print("Edge Geometry Created!")

Creating A Triangle Geometry
############################

To create a triangle geometry, use the `create_triangle_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **vertices_path**: The :ref:`DataPath <DataPath>` to the vertices array.
- **triangle_list_path**: The :ref:`DataPath <DataPath>` to the triangle list array.
- **vertex_attr_matrix_name** (optional): The name of the vertex attribute matrix. Defaults to "Vertex Data".
- **face_attr_matrix_name** (optional): The name of the face attribute matrix. Defaults to "Face Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create a triangle geometry
   result = nx.create_triangle_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Triangle Geometry"]), vertices_path=nx.DataPath("Vertices"), triangle_list_path=nx.DataPath("Triangle List"), vertex_attr_matrix_name='Vertex Data', face_attr_matrix_name='Face Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      triangle_geom = data_structure[nx.DataPath("Triangle Geometry")]
      print("Triangle Geometry Created!")

Creating A Quadrilateral Geometry
#################################

To create a quadrilateral geometry, use the `create_quad_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **vertices_path**: The :ref:`DataPath <DataPath>` to the vertices array.
- **quad_list_path**: The :ref:`DataPath <DataPath>` to the quadrilateral list array.
- **vertex_attr_matrix_name** (optional): The name of the vertex attribute matrix. Defaults to "Vertex Data".
- **face_attr_matrix_name** (optional): The name of the face attribute matrix. Defaults to "Quad Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create a quad geometry
   result = nx.create_quad_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Quad Geometry"]), vertices_path=nx.DataPath("Vertices"), quad_list_path=nx.DataPath("Quad List"), vertex_attr_matrix_name='Vertex Data', face_attr_matrix_name='Quad Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      quad_geom = data_structure[nx.DataPath("Quad Geometry")]
      print("Quad Geometry Created!")

Creating A Tetrahedral Geometry
###############################

To create a tetrahedral geometry, use the `create_tetrahedral_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **vertices_path**: The :ref:`DataPath <DataPath>` to the vertices array.
- **tetrahedral_list_path**: The :ref:`DataPath <DataPath>` to the tetrahedral list array.
- **vertex_attr_matrix_name** (optional): The name of the vertex attribute matrix. Defaults to "Vertex Data".
- **cell_attr_matrix_name** (optional): The name of the cell attribute matrix. Defaults to "Cell Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create a tetrahedral geometry
   result = nx.create_tetrahedral_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Tetrahedral Geometry"]), vertices_path=nx.DataPath("Vertices"), tetrahedral_list_path=nx.DataPath("Tetrahedral List"), vertex_attr_matrix_name='Vertex Data', cell_attr_matrix_name='Cell Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      tetrahedral_geom = data_structure[nx.DataPath("Tetrahedral Geometry")]
      print("Tetrahedral Geometry Created!")

Creating A Hexahedral Geometry
##############################

To create a hexahedral geometry, use the `create_hexahedral_geometry` method. This method requires the following parameters:

- **data_structure**: The data structure where the geometry will be created.
- **geometry_path**: The :ref:`DataPath <DataPath>` where the geometry will be stored.
- **vertices_path**: The :ref:`DataPath <DataPath>` to the vertices array.
- **hexahedral_list_path**: The :ref:`DataPath <DataPath>` to the hexahedral list array.
- **vertex_attr_matrix_name** (optional): The name of the vertex attribute matrix. Defaults to "Vertex Data".
- **cell_attr_matrix_name** (optional): The name of the cell attribute matrix. Defaults to "Cell Data".
- **array_handling** (optional): Specifies how to handle existing arrays. Defaults to ``ArrayHandlingType.Copy``.

Example usage:

.. code-block:: python

   import simplnx as nx

   # Create a hexahedral geometry
   result = nx.create_hexahedral_geometry(data_structure=data_structure, geometry_path=nx.DataPath(["Hexahedral Geometry"]), vertices_path=nx.DataPath("Vertices"), hexahedral_list_path=nx.DataPath("Hexahedral List"), vertex_attr_matrix_name='Vertex Data', cell_attr_matrix_name='Cell Data', array_handling=nx.ArrayHandlingType.Copy)
   if result.valid():
      hexahedral_geom = data_structure[nx.DataPath("Hexahedral Geometry")]
      print("Hexahedral Geometry Created!")

General Parameters 
------------------

.. _ArrayCreationParameter:
.. py:class:: ArrayCreationParameter


   This parameter holds a :ref:`DataPath <DataPath>` value that points to the location within the DataStructure of where
   the DataArray will be created.

  .. code:: python

    data_path = nx.DataPath("Small IN100/Scan Data/Data")

.. _ArraySelectionParameter:
.. py:class:: ArraySelectionParameter

   This parameter holds a :ref:`DataPath <DataPath>` value that points to the location within the DataStructure of where
   the DataArray will be read.

  .. code:: python

    data_path = nx.DataPath("Small IN100/Scan Data/Data")

.. _ArrayThresholdsParameter:
.. py:class:: ArrayThresholdsParameter

   This parameter holds a ArrayThresholdSet_ object and is used specifically for the :ref:`simplnx.MultiThresholdObjectsFilter() <MultiThresholdObjectsFilter>` filter.
   This parameter should not be directly invoked but instead it's ArrayThresholdSet_ is invoked and used.
 
.. _ArrayThresholdSet:
.. py:class:: ArrayThresholdSet

  This class holds a list of ArrayThreshold_ objects.

  :ivar thresholds: List[ArrayThreshold_] objects

.. _ArrayThreshold:
.. py:class:: ArrayThresholdSet.ArrayThreshold

  This class holds the values that are used for comparison in the :ref:`simplnx.MultiThresholdObjectsFilter() <MultiThresholdObjectsFilter>` filter.

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
   threshold_set.union_op = nx.IArrayThreshold.UnionOperator.And
   result = nx.MultiThresholdObjectsFilter.execute(data_structure=data_structure,
                                       array_thresholds=threshold_set, 
                                       created_data_path="Mask",
                                       created_mask_type=nx.DataType.boolean)

.. _AttributeMatrixSelectionParameter:
.. py:class:: AttributeMatrixSelectionParameter

   This parameter holds a :ref:`DataPath <DataPath>` value that points to the location within the DataStructure of a selected AttributeMatrix.

  .. code:: python

    data_path = nx.DataPath("Small IN100/Scan Data")

.. _BoolParameter:
.. py:class:: BoolParameter

   This parameter holds a True/False value and is represented in the UI with a check box

   .. code:: python

    enable_some_feature = True

.. _CalculatorParameter:
.. py:class:: CalculatorParameter

   This parameter has a single member type "ValueType" that can be constructed with the necessary values.

   .. py:class::    CalculatorParameter.ValueType

   :ivar selected_group: The :ref:`DataGroup<DataGroup>` or :ref:`AttributeMatrix<AttributeMatrix>` that contains the :ref:`DataArray <DataArray>` that will be used in the equations
   :ivar equation: String. The equation that will be evaluated
   :ivar units: nx.CalculatorParameter.AngleUnits.Radians or nx.CalculatorParameter.AngleUnits.Degrees

.. code:: python

   selected_group = nx.DataPath("Small IN100/Scan Data")
   infix_equation = "Confidence Index * 10"
   calc_param = nx.CalculatorParameter.ValueType( selected_group, infix_equation, nx.CalculatorParameter.AngleUnits.Radians)
   result = nx.ArrayCalculatorFilter.execute(data_structure = data_structure,
                                             calculated_array=nx.DataPath("Small IN100/Scan Data/Calulated CI"),
                                             calculator_parameter = calc_param, 
                                             scalar_type=nx.NumericType.float32)



.. _ChoicesParameter:
.. py:class:: ChoicesParameter

   This parameter holds a single value from a list of choices in the form of an integer. The filter documentation
   should have the valid values to chose from. It is represented in the UI through a ComboBox drop down menu.
   It can be initialized with an integer type.

.. code:: python

    a_combo_box_value = 2

.. _DataGroupCreationParameter:
.. py:class:: DataGroupCreationParameter

   This parameter holds a :ref:`DataPath <DataPath>` value that points to the location within the DataStructure of a :ref:`DataGroup<DataGroup>` that will be created
   by the filter.

  .. code:: python

    data_path = nx.DataPath("Small IN100/Scan Data")

.. _DataGroupSelectionParameter:
.. py:class:: DataGroupSelectionParameter

   This parameter holds a :ref:`DataPath <DataPath>` value that points to the location within the DataStructure of a :ref:`DataGroup<DataGroup>` that will be used in the filter.

  .. code:: python

    data_path = nx.DataPath("Small IN100/Scan Data")

.. _DataObjectNameParameter:
.. py:class:: DataObjectNameParameter

   This parameter holds a **string** value. It typically is the name of a **DataObject** within the **DataStructure**. 

  .. code:: python

    data_path = "Small IN100"

.. _DataPathSelectionParameter:
.. py:class:: DataPathSelectionParameter

   This parameter holds a :ref:`DataPath <DataPath>` object that represents an object within the :ref:`DataStructure<DataStructure>`.

  .. code:: python

    data_path = nx.DataPath("Small IN100/Scan Data/Confidence Index")

.. _DataStoreFormatParameter:
.. py:class:: DataStoreFormatParameter

   This parameter holds a **string** value that represents the kind of  :ref:`DataStore<DataStore>` that will be used
   to store the data. Depending on the version of simplnx being used, there can be
   both in-core and out-of-core  :ref:`DataStore<DataStore>` objects available.


.. _DataTypeParameter:
.. py:class:: DataTypeParameter

   This parameter holds an enumeration value that represents the numerical type for created arrays. The possible values are.

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

.. _Dream3dImportParameter:
.. py:class:: Dream3dImportParameter

   This class holds the information necessary to import a .dream3d file through the ImportData object.

   :ivar ValueType: ImportData

   .. py:class:: Dream3dImportParameter.ValueType
   
      The ImportData object has 2 member variables that can be set.

   :ivar file_path: Path to the .dream3d file on the file system
   :ivar data_paths: List of :ref:`DataPath <DataPath>` objects. Use the python 'None' value to indicate that you want to read **ALL** the data from file.

.. code:: python

   import_data = nx.Dream3dImportParameter.ImportData()
   import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
   import_data.data_paths = None
   result = nx.ReadDREAM3DFilter.execute(data_structure=data_structure, import_file_data=import_data)

.. _DynamicTableParameter:
.. py:class:: DynamicTableParameter

    This paramter holds values from a 2D table of values. This parameter can be initialized from a 
    python "list of lists". For a 2D table the values are rastered with the columns moving the fastest.
    For example in the code below we are creating a 2D DynamicTable info where the first row is "1,2,3"
    and the second row is "4,5,6"
  
   .. code:: python

    dynamic_table_value = [[1,2,3][4,5,6]]


.. _EnsembleInfoParameter:
.. py:class:: EnsembleInfoParameter

   This parameter is represented as a list of 3 value lists. Each List holds 3 values, Crystal Structure, Phase Type, Phase Name.
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

  .. code:: python

    ensemble_info_parameter = []
    ensemble_info_parameter.append(["Hexagonal-High 6/mmm","Primary","Phase 1"])
    ensemble_info_parameter.append(["Cubic-High m-3m","Primary","Phase 2"])
    result = cxor.CreateEnsembleInfoFilter.execute(data_structure=data_structure,
                             cell_ensemble_attribute_matrix_name=nx.DataPath(["Phase Data"]), 
                             crystal_structures_array_name="CrystalStructures", 
                             phase_names_array_name="Phase Names", 
                             phase_types_array_name="Primary", 
                             ensemble=ensemble_info_parameter
                             )

.. _FileSystemPathParameter:
.. py:class:: FileSystemPathParameter

   This parameter represents a file or folder on the local filesystem (or a network mounted filesystem) 
   and can be instantiated using a "PathLike" python class or python string.

.. code:: python

    a_file_system_path = "/The/Path/To/The/File/Or/Directory"

.. _CreateColorMapParameter:
.. py:class:: CreateColorMapParameter (Updated v1.2.6)
   
   This parameter is used specifically for the  :ref:`simplnx.CreateColorMapFilter() <CreateColorMapFilter>` filter. This parameter 
   represents a **string** value that corresponds to an RGB Preset Name and can be instantiated using a simple python string type.

   Default RGB Preset Names are as follows:

   * "Rainbow Desaturated"
   * "Cold and Hot"
   * "Black-Body Radiation"
   * "X Ray"
   * "Grayscale"
   * "Black, Blue and White"
   * "Black, Orange and White"
   * "Rainbow Blended White"
   * "Rainbow Blended Grey"
   * "Rainbow Blended Black"
   * "Blue to Yellow"
   * "jet"
   * "rainbow"
   * "Haze"
   * "hsv"

   .. code:: python

      result = nx.CreateColorMapFilter.execute(data_structure=data_structure,
                                              rgb_array_path="CI Color", 
                                              input_data_array_path=nx.DataPath("Small IN100/Scan Data/Confidence Index"), 
                                              selected_preset="hsv")      

.. _GeneratedFileListParameter:
.. py:class:: GeneratedFileListParameter

   This parameter describes the necessary pieces of information to construct a list
   of files that is then handed off to the filter. In order to instantiate this 
   parameter the programmer should use the  GeneratedFileListParameter.ValueType data member
   of the GeneratedFileListParameter.

  :ivar ValueType: data member that holds values to generate a file list

  .. py:class:: GeneratedFileListParameter.ValueType

  :ivar input_path: The file system path to the directory that contains the input files
  :ivar ordering: This describes how to generate the files. One of nx.GeneratedFileListParameter.Ordering.LowToHigh or nx.GeneratedFileListParameter.Ordering.HighToLow
  :ivar file_prefix: The string part of the file name that appears **before** the index digits
  :ivar file_suffix: The string part of the file anem that appears **after** the index digits
  :ivar file_extension: The file extension of the input files includeing the "." character.
  :ivar start_index: The starting index value
  :ivar end_index: The ending index value (inclusive)
  :ivar increment_index: The value of how much to increment the index value when generating the file list
  :ivar padding_digits: The maximum number of digits to pad values out to.


  For example if you have a stack of images in tif format numbered from 11 to 174
  where there are only 2 digits for slice indices \< 100, and 3 digits after 100 the
  breakdown of the file name is as follows

   +------------------------+--------------------------+--------+-----------+
   | Prefix                 | index and padding digits | suffix | extension |
   +========================+==========================+========+===========+
   | slice-                 | 100                      | _Data  | .tif      |
   +------------------------+--------------------------+--------+-----------+

  The python code to implement this scheme is as follows

  .. code:: python

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

    result = cxitk.ITKImportImageStack.execute(data_structure=data_structure, 
                                      cell_data_name="Cell Data", 
                                      image_data_array_path="Image Data", 
                                      output_image_geometry_path=nx.DataPath(["Image Stack"]), 
                                      image_transform_choice=0,
                                      input_file_list_info=generated_file_list_value,
                                      origin=[0., 0., 0.], 
                                      spacing=[1., 1.,1.])
    if len(result.errors) != 0:
        print('Errors: {}', result.errors)
        print('Warnings: {}', result.warnings)
    else:
        print("No errors running the filter")


.. _GeometrySelectionParameter:
.. py:class:: GeometrySelectionParameter

   This parameter represents the :ref:`DataPath <DataPath>` to a valid :ref:`simplnx.Geometry() <Geometry Descriptions>`

.. _ReadCSVDataParameter:
.. py:class:: ReadCSVDataParameter

   This parameter is used for the :ref:`simplnx.ReadCSVFileFilter() <ReadCSVFileFilter>` and holds
   the information to import a file formatted as table data where each 
   column of data is a single array. 
   
   + The file can be comma, space, tab or semicolon separated.
   + The file optionally can have a line of headers. The user can specify what line number the header is located.
   + The import can start at a user specified line number but will continue to the end of the file.

   The primary python object that will hold the information to pass to the filter is the ReadCSVDataParameter class described below.

   :ivar ValueType: ReadCSVDataParameter

   .. py:class:: ReadCSVDataParameter

      The ReadCSVDataParameter class holds all the necessary information to import a CSV formatted file into DREAM3D-NX. There are
      a number of member variables that need to be set correctly before the filter will execute
      correctly.

   :ivar input_file_path: "PathLike".  The path to the input file on the file system.
   :ivar start_import_row: Int.  What line number does the data start on. ONE (1) Based numbering scheme.
   :ivar delimiters: List[string]. List of delimiters that will be used to separate the lines of the file into columns.
   :ivar consecutive_delimiters: Bool. Should consecutive delimiters be counted as a single delimiter.
   :ivar custom_headers: List[string]. If the file does not have headers, this is a list of string values, 1 per column of data, that will also become the names of the created  :ref:`DataArray <DataArray>`.
   :ivar data_types: List[:ref:`nx.DataType<DataTypeParameter>`]. The DataType, one per column, that indicates the kind of native numerical values (int, float... ) that will be used in the created  :ref:`DataArray <DataArray>`.
   :ivar skipped_array_mask: List[bool]. Booleans, one per column, that indicate whether or not to skip importing each created :ref:`DataArray <DataArray>`.
   :ivar tuple_dims: List[int]. The tuple dimensions for the created  :ref:`DataArrays <DataArray>`.
   :ivar headers_line: Int. The line number of the file that has the headers listed on a single line. ONE (1) based indexing.
   :ivar header_mode: 'nx.ReadCSVDataParameter.HeaderMode.'. Can be one of 'nx.ReadCSVDataParameter.HeaderMode.Line' or 'nx.ReadCSVDataParameter.HeaderMode.Custom'.


.. code:: python

   data_structure = nx.DataStructure()
   
   # Example File has 7 columns to import
   read_csv_data = nx.ReadCSVDataParameter()
   read_csv_data.input_file_path = "/tmp/test_csv_data.csv"
   read_csv_data.start_import_row = 2
   read_csv_data.delimiters = [',']
   read_csv_data.custom_headers = []
   read_csv_data.column_data_types = [nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.int32 ]
   read_csv_data.skipped_array_mask = [False,False,False,False,False,False,False ]
   read_csv_data.tuple_dims = [37989]
   read_csv_data.headers_line = 1
   read_csv_data.header_mode = nx.ReadCSVDataParameter.HeaderMode.Line

   # This will store the imported arrays into a newly generated DataGroup
   result = nx.ReadCSVFileFilter.execute(data_structure=data_structure,
                                         # This will store the imported arrays into a newly generated DataGroup
                                         created_data_group=nx.DataPath(["Imported Data"]),
                                         # We are not using this parameter but it still needs a value
                                         selected_data_group=nx.DataPath(),
                                         # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                         use_existing_group=False,
                                         # The ReadCSVData object with all member variables set.
                                         read_csv_data=read_csv_data # The ReadCSVData object with all member variables set.
                                         )

.. _H5EbsdReaderParameter:
.. py:class:: ReadH5EbsdFileParameter
   
   This parameter is used for the :ref:`orientationAnalysis.ReadH5EbsdFilter() <ReadH5EbsdFilter>` and holds the information to import the EBSD data from the file.

   The primary python object that will hold the information to pass to the filter is the ReadH5EbsdFileParameter class described below.

   :ivar ValueType: ReadH5EbsdFileParameter

   .. py:class:: ReadH5EbsdFileParameter

      The ReadH5EbsdFileParameter class holds all the necessary information to import EBSD data stored in the H5Ebsd file.

   :ivar euler_representation: Int.  0=Radians, 1=Degrees
   :ivar start_slice: Int. The starting slice of EBSD data to import
   :ivar end_slice: Int.  The ending slice (inclusive) of EBSD data to import
   :ivar selected_array_names: List[string]. The names of the EBSD data to import. These may differ slightly between the various OEMs.
   :ivar input_file_path: PathLike. The path to the .h5ebsd file to read.
   :ivar use_recommended_transform: Bool. Apply the stored sample and crystal reference frame transformations.

   .. code:: python

      data_structure = nx.DataStructure()
      # Create the ReadH5EbsdFileParameter and assign values to it.
      h5ebsdParameter = cxor.ReadH5EbsdFileParameter.ValueType()
      h5ebsdParameter.euler_representation=0
      h5ebsdParameter.end_slice=117
      h5ebsdParameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
      h5ebsdParameter.input_file_path="Data/Output/Reconstruction/Small_IN100.h5ebsd"
      h5ebsdParameter.start_slice=1
      h5ebsdParameter.use_recommended_transform=True

      # Execute Filter with Parameters
      result = cxor.ReadH5EbsdFilter.execute(
         data_structure=data_structure,
         cell_attribute_matrix_name="Cell Data",
         cell_ensemble_attribute_matrix_name="Cell Ensemble Data",
         data_container_name=nx.DataPath("DataContainer"),
         read_h5_ebsd_parameter=h5ebsdParameter
      )


.. _ReadHDF5DatasetParameter:
.. py:class:: ReadHDF5DatasetParameter

   This parameter is used for the :ref:`simplnx.ReadHDF5DatasetFilter<ReadHDF5DatasetFilter>` and holds the information
   to import specific data sets from within the HDF5 file into DREAM3D/simplnx

   .. py:class:: ReadHDF5DatasetParameter.ValueType

      This holds the main parameter values which consist of the following data members

      :ivar input_file: A "PathLike" value to the HDF5 file on the file system
      :ivar datasets: list[ReadHDF5DatasetParameter.DatasetImportInfo, ....]
      :ivar parent: Optional: The :ref:`DataPath <DataPath>` object to a parente group to create the :ref:`DataArray <DataArray>` into. If left blank the :ref:`DataArray <DataArray>` will be created at the top level of the :ref:`DataStructure<DataStructure>`

   .. py:class:: ReadHDF5DatasetParameter.DatasetImportInfo

      The DatasetImportInfo class has 3 data members that hold information on a specific data set
      inside the HDF5 file that the programmer wants to import.

   :ivar dataset_path: string. The internal HDF5 path to the data set expressed as a path like string "/foo/bar/dataset"
   :ivar tuple_dims: string. A comma separated list of the tuple dimensions from **SLOWEST** to **FASTEST** dimensions ("117,201,189")
   :ivar component_dims: string. A comma separated list of the component dimensions from **SLOWEST** to **FASTEST** dimensions ("1")

   .. code:: python

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
      # import_hdf5_param.parent = nx.DataPath(["Imported Data"])
      result = nx.ReadHDF5DatasetFilter.execute(data_structure=data_structure,
                                          import_hdf5_object=import_hdf5_param
                                          )


.. _MultiArraySelectionParameter:
.. py:class:: MultiArraySelectionParameter

   This parameter represents a list of :ref:`DataPath <DataPath>` objects where each :ref:`DataPath <DataPath>` object
   points to a  :ref:`DataArray <DataArray>`

   .. code:: python

    path_list = [nx.DataPath(["Group 1", "Array"]), nx.DataPath(["Group 1", "Array 2"])]

.. _MultiPathSelectionParameter:
.. py:class:: MultiPathSelectionParameter

   This parameter represents a list of :ref:`DataPath <DataPath>` objects. The end point of each :ref:`DataPath <DataPath>`
   object can be any object in the  :ref:`DataStructure<DataStructure>`

   .. code:: python

    path_list = [nx.DataPath(["Group 1", "Array"]), nx.DataPath(["Group 1", "Array 2"])]   


.. _NeighborListSelectionParameter:
.. py:class:: NeighborListSelectionParameter

   This parameter represents a :ref:`DataPath <DataPath>` object that has an end point of a 'nx.NeighborList' object

.. _NumericTypeParameter:
.. py:class:: NumericTypeParameter

   This parameter represents a choice from a list of known numeric types. The programmer
   should use the predefined types instead of a plain integer value.

    - nx.NumericType.int8 = 0
    - nx.NumericType.uint8= 1
    - nx.NumericType.int16= 2
    - nx.NumericType.uint16= 3
    - nx.NumericType.int32= 4
    - nx.NumericType.uint32= 5
    - nx.NumericType.int64= 6
    - nx.NumericType.uint64= 7
    - nx.NumericType.float32= 8
    - nx.NumericType.float64= 9

  .. code:: python

    array_type = nx.NumericType.float32

.. _StringParameter:
.. py:class:: StringParameter

   This parameter represents a **string** value and can be instantiated using a simple python string type. 

Numerical Parameters
--------------------

This group of parameters wrap a specific native C++ numeric type. They can be instantiated
using standard python integers or decimal values. For example.

   .. code:: python

      some_varible = 10
      other_variable = 22.342


.. _Int8Parameter:
.. py:class:: Int8Parameter

   Represents a signed 8 bit integer value

.. _UInt8Parameter:
.. py:class:: UInt8Parameter

   Represents a unsigned 8 bit integer value

.. _Int16Parameter:
.. py:class:: Int16Parameter

   Represents a signed 16 bit integer value

.. _UInt16Parameter:
.. py:class:: UInt16Parameter

   Represents a unsigned 16 bit integer value

.. _Int32Parameter:
.. py:class:: Int32Parameter

   Represents a signed 32 bit integer value

.. _UInt32Parameter:
.. py:class:: UInt32Parameter

   Represents a unsigned 32 bit integer value

.. _Int64Parameter:
.. py:class:: Int64Parameter

   Represents a signed 64 bit integer value

.. _UInt64Parameter:
.. py:class:: UInt64Parameter

   Represents a unsigned 64 bit integer value

.. _Float32Parameter:
.. py:class:: Float32Parameter

   Represents a 32 bit floating point value

.. _Float64Parameter:
.. py:class:: Float64Parameter

   Represents a 64 bit floating point value


Numerical Vector Parameters
---------------------------

This group represents a parameter that is being used to gather more than a single
scalar value from the user. For example, an Origin for an Image Geometry or the 
dimensions of a DataArray. It is represented as a list of numerical values. For example
if a parameter is a 4x1 Float32 value then it would be initialized by:

.. code:: python

   origin = [10.0, 20.0, 33.3, 0.2342]

.. _VectorInt8Parameter:
.. py:class:: VectorInt8Parameter

   Represents a vector of signed 8 bit integer values

.. _VectorUInt8Parameter:
.. py:class:: VectorUInt8Parameter

   Represents a vector of unsigned 8 bit integer values

.. _VectorInt16Parameter:
.. py:class:: VectorInt16Parameter

   Represents a vector of signed 16 bit integer values

.. _VectorUInt16Parameter:
.. py:class:: VectorUInt16Parameter

   Represents a vector of unsigned 16 bit integer values

.. _VectorInt32Parameter:
.. py:class:: VectorInt32Parameter

   Represents a vector of signed 32 bit integer values

.. _VectorUInt32Parameter:
.. py:class:: VectorUInt32Parameter

   Represents a vector of unsigned 32 bit integer values

.. _VectorInt64Parameter:
.. py:class:: VectorInt64Parameter

   Represents a vector of signed 64 bit integer values

.. _VectorUInt64Parameter:
.. py:class:: VectorUInt64Parameter

   Represents a vector of unsigned 64 bit integer values

.. _VectorFloat32Parameter:
.. py:class:: VectorFloat32Parameter

   Represents a vector of 32 bit floating point values

.. _VectorFloat64Parameter:
.. py:class:: VectorFloat64Parameter

   Represents a vector of 64 bit floating point values
