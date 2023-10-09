Filter Parameter Classes
========================

.. _Result:
.. py:class:: Result

   The object that encapsulates any warnings or errors from either preflighting or executing a complex.Filter object.
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

   This parameter holds a :ref:`DataPath<DataPath>` value that points to the location within the DataStructure of where
   the DataArray will be created.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Data"])   

.. _ArraySelectionParameter:
.. py:class:: ArraySelectionParameter

   This parameter holds a :ref:`DataPath<DataPath>` value that points to the location within the DataStructure of where
   the DataArray will be read.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Data"])   

.. _ArrayThresholdsParameter:
.. py:class:: ArrayThresholdsParameter

   This parameter holds a ArrayThresholdSet_ object and is used specifically for the :ref:`complex.MultiThresholdObjects() <MultiThresholdObjects>` filter.
   This parameter should not be directly invoked but instead it's ArrayThresholdSet_ is invoked and used.

 
.. _ArrayThresholdSet:
.. py:class:: ArrayThresholdSet

  This class holds a list of ArrayThreshold_ objects.

  :ivar thresholds: List[ArrayThreshold_] objects

.. _ArrayThreshold:
.. py:class:: ArrayThresholdSet.ArrayThreshold

  This class holds the values that are used for comparison in the :ref:`complex.MultiThresholdObjects() <MultiThresholdObjects>` filter.

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

   This parameter holds a :ref:`DataPath<DataPath>` value that points to the location within the DataStructure of a selected AttributeMatrix.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data"])   

.. _BoolParameter:
.. py:class:: BoolParameter

   This parameter holds a True/False value and is represented in the UI with a check box

   .. code:: python

    enable_some_feature = True

.. _CalculatorParameter:
.. py:class:: CalculatorParameter

   This parameter has a single member type "ValueType" that can be constructed with the necessary values.

   .. py:class::    CalculatorParameter.ValueType

   :ivar selected_group: The :ref:`DataGroup<DataGroup>` or :ref:`AttributeMatrix<AttributeMatrix>` that contains the :ref:`DataArray<DataArray>` that will be used in the equations
   :ivar equation: String. The equation that will be evaluated
   :ivar units: cx.CalculatorParameter.AngleUnits.Radians or cx.CalculatorParameter.AngleUnits.Degrees

.. code:: python

   selected_group = cx.DataPath(["Small IN100","Scan Data"])
   infix_equation = "Confidence Index * 10"
   calc_param = cx.CalculatorParameter.ValueType( selected_group, infix_equation, cx.CalculatorParameter.AngleUnits.Radians)
   result = cx.ArrayCalculatorFilter.execute(data_structure = data_structure,
                                             calculated_array=cx.DataPath(["Small IN100","Scan Data","Calulated CI"]), 
                                           infix_equation = calc_param, 
                                           scalar_type=cx.NumericType.float32)



.. _ChoicesParameter:
.. py:class:: ChoicesParameter

   This parameter holds a single value from a list of choices in the form of an integer. The filter documentation
   should have the valid values to chose from. It is represented in the UI through a ComboBox drop down menu.
   It can be initialized with an integer type.

.. code:: python

    a_combo_box_value = 2

.. _DataGroupCreationParameter:
.. py:class:: DataGroupCreationParameter

   This parameter holds a :ref:`DataPath<DataPath>` value that points to the location within the DataStructure of a :ref:`DataGroup<DataGroup>` that will be created
   by the filter.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data"])

.. _DataGroupSelectionParameter:
.. py:class:: DataGroupSelectionParameter

   This parameter holds a :ref:`DataPath<DataPath>` value that points to the location within the DataStructure of a :ref:`DataGroup<DataGroup>` that will be used in the filter.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data"])

.. _DataObjectNameParameter:
.. py:class:: DataObjectNameParameter

   This parameter holds a **string** value. It typically is the name of a **DataObject** within the **DataStructure**. 

  .. code:: python

    data_path = "Small IN100"

.. _DataPathSelectionParameter:
.. py:class:: DataPathSelectionParameter

   This parameter holds a :ref:`DataPath<DataPath>` object that represents an object within the :ref:`DataStructure<DataStructure>`.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])

.. _DataStoreFormatParameter:
.. py:class:: DataStoreFormatParameter

   This parameter holds a **string** value that represents the kind of  :ref:`DataStore<DataStore>` that will be used
   to store the data. Depending on the version of complex being used, there can be
   both in-core and out-of-core  :ref:`DataStore<DataStore>` objects available.


.. _DataTypeParameter:
.. py:class:: DataTypeParameter

   This parameter holds an enumeration value that represents the numerical type for created arrays. The possible values are.

   .. code:: python

    cx.DataType.boolean
    cx.DataType.uint8
    cx.DataType.int8
    cx.DataType.uint16
    cx.DataType.int16
    cx.DataType.uint32
    cx.DataType.int32
    cx.DataType.uint64
    cx.DataType.int64
    cx.DataType.float32
    cx.DataType.float64

.. _Dream3dImportParameter:
.. py:class:: Dream3dImportParameter

   This class holds the information necessary to import a .dream3d file through the ImportData object.

   :ivar ValueType: ImportData

   .. py:class:: Dream3dImportParameter.ValueType
   
      The ImportData object has 2 member variables that can be set.

   :ivar file_path: Path to the .dream3d file on the file system
   :ivar data_paths: List of :ref:`DataPath<DataPath>` objects. Use the python 'None' value to indicate that you want to read **ALL** the data from file.

.. code:: python

   import_data = cx.Dream3dImportParameter.ImportData()
   import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
   import_data.data_paths = None
   result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure, import_file_data=import_data)

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
                             cell_ensemble_attribute_matrix_name=cx.DataPath(["Phase Data"]), 
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

.. _GenerateColorTableParameter:
.. py:class:: GenerateColorTableParameter

   **NOTE: THIS API IS GOING TO CHANGE IN A FUTURE UPDATE**
   
   This parameter is used specifically for the  :ref:`complex.GenerateColorTableFilter() <GenerateColorTableFilter>` filter. The parameter has 
   a single member variable 'default_value' that is of type 'complex.Json'. 

   .. py:class:: complex.Json
   
   This class encapsulates a string that represents well formed JSON. It can be constructed on-the-fly as follows:

   .. code:: python

      color_control_points = cx.Json('{"RGBPoints": [0,0,0,0,0.4,0.901960784314,0,0,0.8,0.901960784314,0.901960784314,0,1,1,1,1]}')
      result = cx.GenerateColorTableFilter.execute(data_structure=data_structure,
                                              rgb_array_path="CI Color", 
                                              selected_data_array_path=cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"]), 
                                              selected_preset=color_control_points)      

.. _GeneratedFileListParameter:
.. py:class:: GeneratedFileListParameter

   This parameter describes the necessary pieces of information to construct a list
   of files that is then handed off to the filter. In order to instantiate this 
   parameter the programmer should use the  GeneratedFileListParameter.ValueType data member
   of the GeneratedFileListParameter.

  :ivar ValueType: data member that holds values to generate a file list

  .. py:class:: GeneratedFileListParameter.ValueType

  :ivar input_path: The file system path to the directory that contains the input files
  :ivar ordering: This describes how to generate the files. One of cx.GeneratedFileListParameter.Ordering.LowToHigh or cx.GeneratedFileListParameter.Ordering.HighToLow
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

    generated_file_list_value = cx.GeneratedFileListParameter.ValueType()
    generated_file_list_value.input_path = "/Users/mjackson/DREAM3DNXData/Data/Porosity_Image"
    generated_file_list_value.ordering = cx.GeneratedFileListParameter.Ordering.LowToHigh

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
                                      image_geometry_path=cx.DataPath(["Image Stack"]), 
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

   This parameter represents the :ref:`DataPath<DataPath>` to a valid :ref:`complex.Geometry() <Geometry Descriptions>`

.. _ImportTextDataParameter:
.. py:class:: ImportTextDataParameter

   This parameter is used for the :ref:`complex.ImportTextDataFilter() <ImportTextDataFilter>` and holds
   the information to import a file formatted as table data where each 
   column of data is a single array. 
   
   + The file can be comma, space, tab or semicolon separated.
   + The file optionally can have a line of headers. The user can specify what line the headers are on
   + The import can start at a user specified line number but will continue to the end of the file.

   The primary python object that will hold the information to pass to the filter is the TextImporterData class described below.

   :ivar ValueType: TextImporterData

   .. py:class:: ImportTextDataParameter.TextImporterData

      The TextImporterData class holds all the necessary information to import a CSV formatted file into DREAM3D-NX. There are
      a number of member variables that need to be set correctly before the filter will execute
      correctly.

   :ivar input_file_path: "PathLike".  The path to the input file on the file system.
   :ivar start_import_row: Int.  What line number does the data start on. 1 Based numbering scheme.
   :ivar comma_as_delimiter: Bool. Are the values comma separated.
   :ivar semicolon_as_delimiter: Bool. Are the values semicolon separated.
   :ivar space_as_delimiter: Bool. Are the values space separated.
   :ivar tab_as_delimiter: Bool. Are the values tab separated.
   :ivar consecutive_delimiters: Bool. Should consecutive delimiters be counted as a single delimiter.
   :ivar custom_headers: List[string]. If the file does not have headers, this is a list of string values, 1 per column of data, that will also become the names of the created  :ref:`DataArray<DataArray>`.
   :ivar data_types: List[cx.DataType]. The DataType, one per column, that indicates the kind of native numerical values (int, float... ) that will be used in the created  :ref:`DataArray<DataArray>`.
   :ivar skipped_array_mask: List[bool]. Booleans, one per column, that indicate whether or not to skip importing each created :ref:`DataArray <DataArray>`.
   :ivar tuple_dims: List[int]. The tuple dimensions for the created  :ref:`DataArrays <DataArray>`.
   :ivar headers_line: Int. The line number of the headers.
   :ivar header_mode: 'cx.TextImporterData.HeaderMode.'. Can be one of 'cx.TextImporterData.HeaderMode.Line' or 'cx.TextImporterData.HeaderMode.Custom'.


.. code:: python

   data_structure = cx.DataStructure()

   text_importer_data = cx.TextImporterData()
   text_importer_data.input_file_path = "/tmp/test_csv_data.csv"
   text_importer_data.start_import_row = 2

   text_importer_data.comma_as_delimiter = True
   text_importer_data.semicolon_as_delimiter = False
   text_importer_data.space_as_delimiter = False
   text_importer_data.tab_as_delimiter = False
   text_importer_data.consecutive_delimiters = False

   text_importer_data.custom_headers = []
   text_importer_data.data_types = [cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.int32 ]
   text_importer_data.skipped_array_mask = [False,False,False,False,False,False,False ]
   text_importer_data.tuple_dims = [37989]

   text_importer_data.headers_line = 1
   text_importer_data.header_mode = cx.TextImporterData.HeaderMode.Line

   # This will store the imported arrays into a newly generated DataGroup
   result = cx.ImportTextDataFilter.execute(data_structure=data_structure,
                                         # This will store the imported arrays into a newly generated DataGroup
                                         created_data_group=cx.DataPath(["Imported Data"]),
                                         # We are not using this parameter but it still needs a value
                                         selected_data_group=cx.DataPath(),
                                         # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                         use_existing_group=False,
                                         # The TextImporterData object with all member variables set.
                                         text_importer_data=text_importer_data # The TextImporterData object with all member variables set.
                                         )


.. _ImportHDF5DatasetParameter:
.. py:class:: ImportHDF5DatasetParameter

   This parameter is used for the :ref:`complex.ImportHDF5Dataset<ImportHDF5Dataset>` and holds the information
   to import specific data sets from within the HDF5 file into DREAM3D/complex

   .. py:class:: ImportHDF5DatasetParameter.ValueType

      This holds the main parameter values which consist of the following data members

      :ivar input_file: A "PathLike" value to the HDF5 file on the file system
      :ivar datasets: list[ImportHDF5DatasetParameter.DatasetImportInfo, ....]
      :ivar parent: Optional: The :ref:`DataPath<DataPath>` object to a parente group to create the :ref:`DataArray<DataArray>` into. If left blank the :ref:`DataArray<DataArray>` will be created at the top level of the :ref:`DataStructure<DataStructure>`

   .. py:class:: ImportHDF5DatasetParameter.DatasetImportInfo

      The DatasetImportInfo class has 3 data members that hold information on a specific data set
      inside the HDF5 file that the programmer wants to import.

   :ivar dataset_path: string. The internal HDF5 path to the data set expressed as a path like string "/foo/bar/dataset"
   :ivar tuple_dims: string. A comma separated list of the tuple dimensions from **SLOWEST** to **FASTEST** dimensions ("117,201,189")
   :ivar component_dims: string. A comma separated list of the component dimensions from **SLOWEST** to **FASTEST** dimensions ("1")

   .. code:: python

      dataset1 = cx.ImportHDF5DatasetParameter.DatasetImportInfo()
      dataset1.dataset_path = "/DataStructure/DataContainer/CellData/Confidence Index"
      dataset1.tuple_dims = "117,201,189"
      dataset1.component_dims = "1"

      dataset2 = cx.ImportHDF5DatasetParameter.DatasetImportInfo()
      dataset2.dataset_path = "/DataStructure/DataContainer/CellData/EulerAngles"
      dataset2.tuple_dims = "117,201,189"
      dataset2.component_dims = "3"

      import_hdf5_param = cx.ImportHDF5DatasetParameter.ValueType()
      import_hdf5_param.input_file = "SmallIN100_Final.dream3d"
      import_hdf5_param.datasets = [dataset1, dataset2]
      # import_hdf5_param.parent = cx.DataPath(["Imported Data"])
      result = cx.ImportHDF5Dataset.execute(data_structure=data_structure,
                                          import_hd_f5_file=import_hdf5_param
                                          )


.. _MultiArraySelectionParameter:
.. py:class:: MultiArraySelectionParameter

   This parameter represents a list of :ref:`DataPath<DataPath>` objects where each :ref:`DataPath<DataPath>` object
   points to a  :ref:`DataArray<DataArray>`

   .. code:: python

    path_list = [cx.DataPath(["Group 1", "Array"]), cx.DataPath(["Group 1", "Array 2"])]

.. _MultiPathSelectionParameter:
.. py:class:: MultiPathSelectionParameter

   This parameter represents a list of :ref:`DataPath<DataPath>` objects. The end point of each :ref:`DataPath<DataPath>`
   object can be any object in the  :ref:`DataStructure<DataStructure>`

   .. code:: python

    path_list = [cx.DataPath(["Group 1", "Array"]), cx.DataPath(["Group 1", "Array 2"])]   


.. _NeighborListSelectionParameter:
.. py:class:: NeighborListSelectionParameter

   This parameter represents a :ref:`DataPath<DataPath>` object that has an end point of a 'cx.NeighborList' object

.. _NumericTypeParameter:
.. py:class:: NumericTypeParameter

   This parameter represents a choice from a list of known numeric types. The programmer
   should use the predefined types instead of a plain integer value.

    - cx.NumericType.int8 = 0
    - cx.NumericType.uint8= 1
    - cx.NumericType.int16= 2
    - cx.NumericType.uint16= 3
    - cx.NumericType.int32= 4
    - cx.NumericType.uint32= 5
    - cx.NumericType.int64= 6
    - cx.NumericType.uint64= 7
    - cx.NumericType.float32= 8
    - cx.NumericType.float64= 9

  .. code:: python

    array_type = cx.NumericType.float32

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
