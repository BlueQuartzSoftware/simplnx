TestOne
=======

.. py:module:: TestOne

.. _DynamicTableExample:
.. py:class:: DynamicTableExample

   **UI Display Name:** *Dynamic Table Examples*

   This is just a test filter

   `Link to the full online documentation for DynamicTableExample <http://www.dream3d.io/nx_reference_manual/Filters/DynamicTableExample>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-----------------------+
   | UI Display                    | Python Named Argument |
   +===============================+=======================+
   | Fixed Columns - Fixed Rows    | param1                |
   +-------------------------------+-----------------------+
   | Fixed Columns - Dynamic Row   | param2                |
   +-------------------------------+-----------------------+
   | Dynamic Columns - Fixed Row   | param3                |
   +-------------------------------+-----------------------+
   | Dynamic Columns - Dynamic Row | param4                |
   +-------------------------------+-----------------------+

   .. py:method:: Execute(param1, param2, param3, param4)

      :param complex.DynamicTableParameter param1: DynamicTableParameter Example Help Text
      :param complex.DynamicTableParameter param2: DynamicTableParameter Example Help Text
      :param complex.DynamicTableParameter param3: DynamicTableParameter Example Help Text
      :param complex.DynamicTableParameter param4: DynamicTableParameter Example Help Text
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ErrorWarningFilter:
.. py:class:: ErrorWarningFilter

   **UI Display Name:** *Error Warning and Test Filter*

   This is just a test filter

   `Link to the full online documentation for ErrorWarningFilter <http://www.dream3d.io/nx_reference_manual/Filters/ErrorWarningFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------+-----------------------+
   | UI Display          | Python Named Argument |
   +=====================+=======================+
   | Execute Error       | execute_error         |
   +---------------------+-----------------------+
   | Execute Exception   | execute_exception     |
   +---------------------+-----------------------+
   | Execute Warning     | execute_warning       |
   +---------------------+-----------------------+
   | Preflight Error     | preflight_error       |
   +---------------------+-----------------------+
   | Preflight Exception | preflight_exception   |
   +---------------------+-----------------------+
   | Preflight Warning   | preflight_warning     |
   +---------------------+-----------------------+

   .. py:method:: Execute(execute_error, execute_exception, execute_warning, preflight_error, preflight_exception, preflight_warning)

      :param complex.BoolParameter execute_error: Execute error parameter
      :param complex.BoolParameter execute_exception: Execute exception parameter
      :param complex.BoolParameter execute_warning: Execute warning parameter
      :param complex.BoolParameter preflight_error: Preflight error parameter
      :param complex.BoolParameter preflight_exception: Preflight exception parameter
      :param complex.BoolParameter preflight_warning: Preflight warning parameter
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExampleFilter1:
.. py:class:: ExampleFilter1

   **UI Display Name:** *Example Filter 1*

   This is just a test filter

   `Link to the full online documentation for ExampleFilter1 <http://www.dream3d.io/nx_reference_manual/Filters/ExampleFilter1>`_ 

   Mapping of UI display to python named argument

   +-------------------+-----------------------+
   | UI Display        | Python Named Argument |
   +===================+=======================+
   | Vect<int,2>       | Vec2_Key              |
   +-------------------+-----------------------+
   | RGBA              | Vec4_Key              |
   +-------------------+-----------------------+
   | Input Directory   | input_dir             |
   +-------------------+-----------------------+
   | Input File        | input_file            |
   +-------------------+-----------------------+
   | Ouptut Directory  | output_dir            |
   +-------------------+-----------------------+
   | Output File       | output_file           |
   +-------------------+-----------------------+
   | Float32Parameter  | param1                |
   +-------------------+-----------------------+
   | Int32 Parameter   | param10               |
   +-------------------+-----------------------+
   | BoolParameter     | param2                |
   +-------------------+-----------------------+
   | Int32Parameter    | param3                |
   +-------------------+-----------------------+
   | Input File List   | param4                |
   +-------------------+-----------------------+
   | StringParameter   | param5                |
   +-------------------+-----------------------+
   | Numeric Type      | param6                |
   +-------------------+-----------------------+
   | Data Thresholds   | param7                |
   +-------------------+-----------------------+
   | Bool Parameter    | param8                |
   +-------------------+-----------------------+
   | Choices Parameter | param9                |
   +-------------------+-----------------------+

   .. py:method:: Execute(Vec2_Key, Vec4_Key, input_dir, input_file, output_dir, output_file, param1, param10, param2, param3, param4, param5, param6, param7, param8, param9)

      :param complex.VectorInt32Parameter Vec2_Key: Example int32 vector help text
      :param complex.VectorUInt8Parameter Vec4_Key: Example uint8 vector help text
      :param complex.FileSystemPathParameter input_dir: Example input directory help text
      :param complex.FileSystemPathParameter input_file: Example input file help text
      :param complex.FileSystemPathParameter output_dir: Example output directory help text
      :param complex.FileSystemPathParameter output_file: Example output file help text
      :param complex.Float32Parameter param1: The 1st parameter
      :param complex.Int32Parameter param10: 
      :param complex.BoolParameter param2: The 2nd parameter
      :param complex.Int32Parameter param3: The 1st parameter
      :param complex.GeneratedFileListParameter param4: Data needed to generate the input file list
      :param complex.StringParameter param5: Example string help text
      :param complex.NumericTypeParameter param6: Example numeric type help text
      :param complex.ArrayThresholdsParameter param7: DataArray thresholds to mask
      :param complex.BoolParameter param8: 
      :param complex.ChoicesParameter param9: 
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExampleFilter2:
.. py:class:: ExampleFilter2

   **UI Display Name:** *Example Filter 2*

   This is just a test filter

   `Link to the full online documentation for ExampleFilter2 <http://www.dream3d.io/nx_reference_manual/Filters/ExampleFilter2>`_ 

   Mapping of UI display to python named argument

   +------------------------------+-----------------------+
   | UI Display                   | Python Named Argument |
   +==============================+=======================+
   | DataPathSelectionParameter   | param10               |
   +------------------------------+-----------------------+
   | GeometrySelectionParameter   | param11               |
   +------------------------------+-----------------------+
   | MultiArraySelectionParameter | param12               |
   +------------------------------+-----------------------+
   | DynamicTableParameter        | param13               |
   +------------------------------+-----------------------+
   | ChoicesParameter             | param3                |
   +------------------------------+-----------------------+
   | Array Creation               | param5                |
   +------------------------------+-----------------------+
   | Array Selection              | param6                |
   +------------------------------+-----------------------+
   | Bool Parameter               | param7                |
   +------------------------------+-----------------------+
   | DataGroupCreationParameter   | param8                |
   +------------------------------+-----------------------+
   | DataGroupSelectionParameter  | param9                |
   +------------------------------+-----------------------+

   .. py:method:: Execute(param10, param11, param12, param13, param3, param5, param6, param7, param8, param9)

      :param complex.DataPathSelectionParameter param10: Example data path selection help text
      :param complex.GeometrySelectionParameter param11: Example geometry selection help text
      :param complex.MultiArraySelectionParameter param12: Example multiarray selection help text
      :param complex.DynamicTableParameter param13: DynamicTableParameter Example Help Text
      :param complex.ChoicesParameter param3: Example choices help text
      :param complex.ArrayCreationParameter param5: Example array creation help text
      :param complex.ArraySelectionParameter param6: Example array selection help text
      :param complex.BoolParameter param7: Example bool help text
      :param complex.DataGroupCreationParameter param8: Example data group creation help text
      :param complex.DataGroupSelectionParameter param9: Example data group selection help text
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _TestFilter:
.. py:class:: TestFilter

   **UI Display Name:** *Test Filter*

   This is just a test filter

   `Link to the full online documentation for TestFilter <http://www.dream3d.io/nx_reference_manual/Filters/TestFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------+-----------------------+
   | UI Display      | Python Named Argument |
   +=================+=======================+
   | Parameter 1     | param1                |
   +-----------------+-----------------------+
   | Parameter 2     | param2                |
   +-----------------+-----------------------+
   | Input File List | param3                |
   +-----------------+-----------------------+

   .. py:method:: Execute(param1, param2, param3)

      :param complex.Float32Parameter param1: The 1st parameter
      :param complex.BoolParameter param2: The 2nd parameter
      :param complex.GeneratedFileListParameter param3: Data needed to generate the input file list
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


