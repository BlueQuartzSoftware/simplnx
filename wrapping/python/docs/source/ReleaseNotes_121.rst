Release Notes 1.2.1
===================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.1
-------------

- Documentation has been updated
- Examples updated to use new API


API Additions 1.2.1
^^^^^^^^^^^^^^^^^^^

- DataObject add the "type" property

- Retrieve the children of a DataObject in the DataStructure using a simplnx.DataPath or "/" delimited string
  
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

- Get a numpy view of a DataArray directly from the DataStructure by using the a simplnx.DataPath or "/" delimited string

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
