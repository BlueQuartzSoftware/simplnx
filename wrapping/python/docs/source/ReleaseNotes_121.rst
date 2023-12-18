Release Notes 1.2.1
===================

The `complex` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.1
-------------

- Documentation has been updated
- Examples updated to use new API


API Additions 1.2.1
^^^^^^^^^^^^^^^^^^^

- DataObject add the "type" property

- Retrieve the children of a DataObject in the DataStructure using a complex.DataPath or "/" delimited string
  
  .. code:: python
  
    #------------------------------------------------------------------------------
    # If you want to list out the children at a specific level of the DataStruture
    #------------------------------------------------------------------------------
    # Use an empty path for the top level objects
    children_paths = data_structure.get_children(cx.DataPath(""))
    print(children_paths)

- Generate a text or GraphViz representation of the DataStructure.

  .. code:: python

    # This will generate the hierarchy as a GraphViz formatted string that you can
    # print or save to a file
    graphviz_content = data_structure.hierarchy_to_graphviz()
    print(graphviz_content)

    # This will generate the hierarchy as an ASCI Formatted string.
    hierarchy_as_str = data_structure.hierarchy_to_str()
    print(hierarchy_as_str)

- Get a numpy view of a DataArray directly from the DataStructure by using the a complex.DataPath or "/" delimited string

  .. code:: python

    data_structure = cx.DataStructure()
    output_array_path = cx.DataPath(["3D Array"])
    tuple_dims = [[3, 2, 5]]
    array_type = cx.NumericType.float32
    create_array_filter = cx.CreateDataArray()
    result  = create_array_filter.execute(data_structure=data_structure, 
                                          component_count=1, 
                                          data_format="", 
                                          initialization_value="10", 
                                          numeric_type=array_type, 
                                          output_data_array=output_array_path, 
                                          tuple_dimensions=tuple_dims)
    npdata = data_structure[output_array_path].npview()



Filter Changes 1.2.1
^^^^^^^^^^^^^^^^^^^^

- ENH: Implement SIMPL pipeline conversion (#779) (d11a7a8dd)
- ENH: ITKImportImageStack Flip about X|Y Axis speed optimization (#799) (2ecb34507)
- DOC: Detailed descriptions for several filter's parameters (#797) (693671664)
- ENH: Allow test data to be downloaded from a different URL. (#789) (1192311e3)
- PYTHON: Add API to DataStructure, Document API, add example uses. (#792) (c1bdb5a08)
- BUG: Ensure Sphinx is installed during the conda build process. (#791) (ab76de704)
- ENH: Fixes to allow ITKImageProcessing to compile against ITK 5.3.0 (#784) (b43f1dad7)
- Add create_child_path method to be able to modify DataPaths in Python. (#788) (87026e508)
- VCPKG: Add Default registry and add OpenSSL as an option (#776) (7b1b9573a)
- BUG: ReadEspritDataFile - If spacing is zero, reset to 1.0 (#782) (a0675ea12)
- BUG: Fix Import Fiji Montage Origin and Grayscale conversions (#777) (462b70b84)
- ENH: Update method names to dump a DataStructure hierarchy to text or .dot (#780) (b32d15bd5)
- ENH: Cleanup warning messages for ReadTextDatArray and CreateDataArray (#781) (749a340ba)
- FILTER: Initialize Data (Initialize Data Rework) (#769) (a06a6d536)
- ENH: Updated vcpkg baseline for fmt 10.1.1 (#775) (bcc6bae79)
- BUG: Fix issue where some plugins were not getting wrapped with Python (#773) (415439adf)
- BUG: ErodeDilateBadData - Fix initialization of ignored data arrays. (#771) (0e847aa3d)
- PYTHON: Add DataStructure and DataPath APIs  (#740) (12ef18ca8)

