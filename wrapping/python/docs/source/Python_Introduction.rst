Python Introduction
===================



Basic Import Statements
-----------------------

At the top of your python file you will need a few import statements to bring in the *complex* library and it*s plugins. There
are three plugins, but only 2 of those you will need to explicitly import.

+ ComplexCode: This plugin is included into the *complex* module automatically
+ OrientationAnalysis: This plugin should be imported if any of the functionality is needed.
+ ITKImageProcessing: This plugin should be imported if any of the functionality is needed.

In your python file you will need the following

.. code:: python

    import complex as cx

If you will need functionality from either of the other *complex* plugins then you will need the following.

.. code:: python

    import itkimageprocessing as cxitk
    import orientationanalysis as cxor

Creating the DataStructure
--------------------------

In order to effectively use the *complex* classes and functions, you will need to create at leaset one *DataStructure* object. 
The *DataStructure* object holds the various *DataGroup*, *AttributeMatrix*, and *DataArray* that will be created. When
then *DataStructure* goes out of scope those items will also be cleaned up. The code to create the
*DataStructure* object is straight forward.

.. code:: python

    # Create a Data Structure
    data_structure = cx.DataStructure()


Creating a DataGroup
--------------------

 A :ref:`DataGroup` can be created with the 
 :ref:`complex.CreateDataGroup.Execute() <CreateDataGroup>` method.

.. code:: python

    #------------------------------------------------------------------------------
    # Create a top level group: (Not needed)
    #------------------------------------------------------------------------------
    result = cx.CreateDataGroup.execute(data_structure=data_structure,
                                        Data_Object_Path=cx.DataPath(['Group']))

Creating a DataArray
--------------------

*complex* stores data in a :ref:`DataArray` object that is created through the :ref:`complex.CreateDataArray.Execute() <CreateDataArray>` method.
This will allow you to create an array that you can then fill with data using any python API that you wish. A basic use
of the method is as follows.

.. code:: python

    # Instantiate and execute immediately teh CreateDataArray Filter
    result  = cx.CreateDataArray.execute(data_structure=data_structure, 
                                        component_count=1, 
                                        data_format="", 
                                        initialization_value="10", 
                                        numeric_type=cx.NumericType.float32, 
                                        output_data_array=cx.DataPath(["3D Array"]), 
                                        tuple_dimensions= [[3, 2, 5]])
    # The returned result holds any warnings or errors that occurred during execution
    if len(result.errors) != 0:
        print('Errors: {}', result.errors)
        print('Warnings: {}', result.warnings)
    else:
        print("No errors running the filter")
    # We get a numpy view of the created DataArray
    npdata = data_structure[cx.DataPath(["3D Array"])].store.npview()

This can be useful to get external data into the DataStructure so that any of the filters
can be applied to that data. For instance, using Numpy's 'load' function can load
external data sources into the DataArray.

.. code:: python

    # Read the CSV file into the DataArray using the numpy view
    file_path = 'angles.csv'
    npdata[:] = np.loadtxt(file_path, delimiter=',')

Within the **complex** code repository, there are example python files that can be used 
as a starting point. `GitHub.com <https://github.com/BlueQuartzSoftware/complex/tree/develop/wrapping/python/examples>`_

Importing a .dream3d File
-------------------------

The native file storage for **complex** is in the form of an `HDF5 <https://www.hdfgroup.org>`_ file. The typical extension 
used from within **complex** is '.dream3d'. The complex python API has 2 separate classes to either import or export
a .dream3d file.

In order to import a .dream3d file, we need to tell **complex** which data to import from the file. 

.. code:: python

    # Create the DataStructure object    data_structure = cx.DataStructure()
    # Create a cx.Dream3dImportParameter.ImportData object and set its values
    import_data = cx.Dream3dImportParameter.ImportData()
    # Set the path to the file on the file system
    import_data.file_path = "/tmp/basic_ebsd.dream3d"
    # Set the import_data.data_paths value to 'None' which signals to the filter to
    # import EVERY piece of data from the file.
    import_data.data_paths = None
    # Instantiate and execte the filter immediately.
    result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure, import_file_data=import_data)
    # Check for any execution warnings or errors
    if len(result.errors) != 0:
        print('Errors: {}', result.errors)
        print('Warnings: {}', result.warnings)
    else:
        print("No errors running the ImportDREAM3DFilter filter")

If you want to only import specific data sets from the file you can set the **import_data.data_paths** to a *List[DataPath]* objects.
Once the filter executes, you can fetch data from the DataStructure using the usual methods shown in the `Creating a DataArray`_ section.

.. code:: python

    # We get a numpy view of the created DataArray
    npdata = data_structure[cx.DataPath(["3D Array"])].store.npview()


Exporting a .dream3d File
-------------------------

After processing the data if you would like to store your data in the native HDF5 file format, then the
:ref:`complex.ExportDREAM3DFilter.Execute() <ExportDREAM3DFilter>` can be used to accomplish this goal. 
The filter will write the **complete** contents of the DataStructure to the HDF5 file.

.. code:: python

    output_file_path = "output_file_example.dream3d"
    result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                            export_file_path=output_file_path, 
                                            write_xdmf_file=True)
    if len(result.errors) != 0:
        print('Errors: {}', result.errors)
        print('Warnings: {}', result.warnings)
    else:
        print("No errors running the filter")


Executing Filters (Immediate and Delayed)
-------------------------------------------

In the **complex** python API, the programmer can execute any filter immediately by 
simply calling the *Execute()* function of the filter. This has been shown
in the above sections. The programmer simply needs to find the appropriate filter for their
needs and use the API accordingly. Another option is to build up the filters and
then execute each filter instance one after another. The programmer should note that
if this is the design they are selecting to use none of the Data Structure objects will
be ready to use until the **Execute()** method is run.

Creating Geometries in Complex
------------------------------

Each of the supported **Geometry** objects can be created using the :ref:`complex.CreateGeometryFilter.Execute() <CreateGeometryFilter>` 
Here is an example of creating the simplest of Geometries, the Image Geometry

.. code:: python

    # Create the DataStructure object    data_structure = cx.DataStructure()
    ig_dims = [10, 20, 30] # <===== NOTE: These are in XYZ order
    result = cx.CreateGeometryFilter.execute(  data_structure=data_structure,
        array_handling= 0,  # This does not matter for Image Geometry
        cell_attribute_matrix_name="Cell Data",
        dimensions=ig_dims, # Note that the dimensions are list as  X, Y, Z
        geometry_name=cx.DataPath(["Image Geometry"]),
        geometry_type=0, # 0 = Image Geometry. See the complete fiter documentation for the possible values
        origin=[0.0, 0.0, 0.0],
        spacing=[1.0, 1.0, 1.0])
    if len(result.errors) != 0:
        print('Errors: {}', result.errors)
        print('Warnings: {}', result.warnings)
    else:
        print("No errors running the CreateGeometryFilter filter")

Any of the Node based geometries can also be created by ensuring that the programmer has
the appropriate input data arrays ready to pass into the filter. These will consist
of the list of vertex values (XYZ as 32 bit floating point values) and the connectivity
list for the 1D, 2D and 3D geometries. :ref:`Please see the appropriate sections in the 
manual for detailed descriptions. <Geometry Descriptions>`

There are working examples within the python file <https://www.github.com/bluequartzsoftware/complex/wrapping/python/examples/geometry_examples.py>. 
The below code will create a TriangleGeometry by importing the vertices and triangle
connectivity from a sample file.

.. code:: python

    # Create the vertex array and fill it from data on disk
    array_path = cx.DataPath(['Vertices'])
    result = cx.CreateDataArray.execute(data_structure,
                                        numeric_type=cx.NumericType.float32,
                                        component_count=3,
                                        tuple_dimensions=[[144]],
                                        output_data_array=array_path,
                                        initialization_value='0')
    vertex_coords = data_structure[array_path].store.npview()
    file_path = 'complex/test/Data/VertexCoordinates.csv'
    vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

    # Create the triangle connectivity array and fill it from data on disk
    array_path = cx.DataPath(['Triangles'])
    result = cx.CreateDataArray.execute(data_structure,
                                        numeric_type=cx.NumericType.uint64,
                                        component_count=3,
                                        tuple_dimensions=[[242]],
                                        output_data_array=array_path,
                                        initialization_value='0')
    triangles = data_structure[array_path].store.npview()
    file_path = 'complex/test/Data/TriangleConnectivity.csv'
    triangles[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

    result = cx.CreateGeometryFilter.execute(data_structure=data_structure,
        array_handling= 1,  # Move the arrays from their original location.
        geometry_name=cx.DataPath(["Triangle Geometry"]),
        geometry_type=4,
        face_attribute_matrix_name="Triangle Data",
        edge_attribute_matrix_name="Triangle Edge Data",
        vertex_attribute_matrix_name="Vertex Data",
        vertex_list_name=cx.DataPath(['Vertices']),
        triangle_list_name=cx.DataPath(['Triangles'])
        )
    if len(result.errors) != 0:
        print('Errors: {}', result.errors)
        print('Warnings: {}', result.warnings)
    else:
        print("No errors running the CreateGeometryFilter (Triangle) filter")


