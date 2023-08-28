Complex Python API Docs
=======================

.. py:module:: complex

.. _DataPath:
.. py:class:: DataPath

  The DataPath is used to describe a path from the top level of the DataStructure
  to a target DataObject (Group, Geometry, AttributeMatrix, DataArray)

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])

.. _ArrayThreshold:
.. py:class:: ArrayThreshold

  This class holds the values that are used for comparison in the :ref:`complex.MultiThresholdObjects() <MultiThresholdObjects>` filter.

  .. code:: python

    threshold_1 = cx.ArrayThreshold()
    threshold_1.array_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])
    threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
    threshold_1.value = 0.1


  :ivar array_path: The DataPath_ to the array to use for this ArrayThreshold
  :ivar comparison: The comparison operator to use. 0:">", 1:"<", 2:"=", 3:"!="
  :ivar value: The value for the comparison


.. _ArrayThresholdSet:
.. py:class:: ArrayThresholdSet

  This class holds a list of ArrayThreshold_ objects and eventually used as an argument
  for the :ref:`complex.MultiThresholdObjects() <MultiThresholdObjects>` filter.

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

  :ivar thresholds: List of ArrayThreshold_ objects

.. _ImportData:
.. py:class:: ImportData

  This class holds the information necessary to import a .dream3d file.

  :ivar file_path: Path to the .dream3d file on the file system
  :ivar data_paths: List of DataPath_ objects. Use the python 'None' value to indicate that you want to read **ALL** the data from file.

  .. code:: python

    import_data = cx.Dream3dImportParameter.ImportData()
    import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
    import_data.data_paths = None


Parameters 
----------

.. _ArrayCreationParameter:
.. py:class:: ArrayCreationParameter

   This parameter holds a DataPath_ value that points to the location within the DataStructure of where
   the DataArray will be created.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Data"])   

.. _ArraySelectionParameter:
.. py:class:: ArraySelectionParameter

   This parameter holds a DataPath_ value that points to the location within the DataStructure of where
   the DataArray will be read.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Data"])   

.. _ArrayThresholdsParameter:
.. py:class:: ArrayThresholdsParameter

   This parameter holds a ArrayThresholdSet_ object.

.. _AttributeMatrixSelectionParameter:
.. py:class:: AttributeMatrixSelectionParameter

   This parameter holds a DataPath_ value that points to the location within the DataStructure of a selected AttributeMatrix.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data"])   

.. _BoolParameter:
.. py:class:: BoolParameter

   This parameter holds a True/False value and is represented in the UI with a check box

.. _CalculatorParameter:
.. py:class:: CalculatorParameter

    This parameter does....    

.. _ChoicesParameter:
.. py:class:: ChoicesParameter

   This parameter holds a single value from a list of choices in the form of an integer. The filter documentation
   should have the valid values to chose from. It is represented in the UI through a ComboBox drop down menu.
   It can be initialized with an integer type. 

.. _DataGroupCreationParameter:
.. py:class:: DataGroupCreationParameter

   This parameter holds a DataPath_ value that points to the location within the DataStructure of a DataGroup_ that will be created
   by the filter.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data"])

.. _DataGroupSelectionParameter:
.. py:class:: DataGroupSelectionParameter

   This parameter holds a DataPath_ value that points to the location within the DataStructure of a DataGroup_ that will be used in the filter.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data"])

.. _DataObjectNameParameter:
.. py:class:: DataObjectNameParameter

   This parameter holds a **string** value. It typically is the name of a **DataObject** within the **DataStructure**. 

  .. code:: python

    data_path = "Small IN100"

.. _DataPathSelectionParameter:
.. py:class:: DataPathSelectionParameter

   This parameter holds a DataPath_ object that represents an object within the DataStructure_.

  .. code:: python

    data_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])

.. _DataStoreFormatParameter:
.. py:class:: DataStoreFormatParameter

   This parameter holds a **string** value that represents the kind of DataStore_ that will be used
   to store the data. Depending on the version of complex being used, there can be
   both in-core and out-of-core DataStore_ objects available.


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

   This parameter holds a ImportData_ object.

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

  This is used in combination with the  :ref:`OrientationAnalysis.CreateEnsembleInfoFilter() <CreateEnsembleInfoFilter>` filter.

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


.. _GenerateColorTableParameter:
.. py:class:: GenerateColorTableParameter

   This parameter will 

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

   This parameter represents the DataPath_ to a valid :ref:`complex.Geometry() <Geometry Descriptions>`

.. _ImportCSVDataParameter:
.. py:class:: ImportCSVDataParameter

   This parameter will 

.. _ImportHDF5DatasetParameter:
.. py:class:: ImportHDF5DatasetParameter

   This parameter will 

.. _MultiArraySelectionParameter:
.. py:class:: MultiArraySelectionParameter

   This parameter represents a list of DataPath_ objects where each DataPath_ object
   points to a DataArray_

   .. code:: python

    path_list = [cx.DataPath(["Group 1", "Array"]), cx.DataPath(["Group 1", "Array 2"])]

.. _MultiPathSelectionParameter:
.. py:class:: MultiPathSelectionParameter

   This parameter represents a list of DataPath_ objects. The end point of each DataPath_
   object can be any object in the DataStructure_

   .. code:: python

    path_list = [cx.DataPath(["Group 1", "Array"]), cx.DataPath(["Group 1", "Array 2"])]   

.. _NeighborListSelectionParameter:
.. py:class:: NeighborListSelectionParameter

   This parameter represents a DataPath_ object that has an end point of a cx.NeighborList object

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
using standard python integers or decimal values.

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
dimensions of a DataArray.

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
