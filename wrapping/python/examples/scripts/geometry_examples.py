"""
Important Note
==============

This python file can be used as an example of how to execute a number of DREAM3D-NX
filters one after another, if you plan to use the codes below (and you are welcome to),
there are a few things that you, the developer, should take note of:

Import Statements
-----------------

You will most likely *NOT* need to include the following code:

   .. code:: python
      
      import simplnx_test_dirs as nxtest

Filter Error Detection
----------------------

In each section of code a filter is created and executed immediately. This may or
may *not* be what you want to do. You can also preflight the filter to verify the
correctness of the filters before executing the filter **although** this is done
for you when the filter is executed. As such, you will want to check the 'result'
variable to see if there are any errors or warnings. If there **are** any then
you, as the developer, should act appropriately on the errors or warnings. 
More specifically, this bit of code:

   .. code:: python

      nxtest.check_filter_result(nxor.ReadAngDataFilter, result)

is used by the simplnx unit testing framework and should be replaced by your own
error checking code. You are welcome to look up the function definition and use
that.

"""
import simplnx as nx

import itkimageprocessing as nxitk
import orientationanalysis as nxor
import simplnx_test_dirs as nxtest

import numpy as np

#------------------------------------------------------------------------------
# Print the various filesystem paths that are pregenerated for this machine.
#------------------------------------------------------------------------------
nxtest.print_all_paths()

"""
In the code below we are using some data files that are found in the simplnx
source code repository. It will up to the programmer to correctly set the paths
to the files before running this example.

"""
# Create the DataStructure object
data_structure = nx.DataStructure()
ig_dims = [10, 20, 30]  # NOTE: These are in XYZ order
result = nx.CreateGeometryFilter.execute(data_structure=data_structure,
                                         array_handling=0,  # This does not matter for Image Geometry
                                         cell_attribute_matrix_name="Cell Data",
                                         dimensions=ig_dims,  # Note that the dimensions are list as  X, Y, Z
                                         geometry_name=nx.DataPath("Image Geometry"),
                                         geometry_type=0,  # 0 = Image Geometry
                                         origin=[0.0, 0.0, 0.0],
                                         spacing=[1.0, 1.0, 1.0])
nxtest.check_filter_result(nx.CreateGeometryFilter, result)


data_object = data_structure["Image Geometry"]
print(f'data_object: {type(data_object)}')
print(f'data_object: {data_object.type}')
if data_object.type == nx.DataObject.DataObjectType.ImageGeom:
    print("Image Geometry")
else:
    print("NOT Image Geometry")

# Now we can create some (or import from another source) some cell based data
# this is data that lives at the center of each cell
# NOTE: we do *not* need to set the tuple dimensions because we are adding this array to the 
# AttributeMatrix that we generated in the last filter.
output_array_path = nx.DataPath("Image Geometry/Cell Data/Float Cell Data")
array_type = nx.NumericType.float32
create_array_nx_filter = nx.CreateDataArray()
result = create_array_nx_filter.execute(data_structure=data_structure, component_count=1, data_format="",
                                     initialization_value="10",
                                     numeric_type=array_type, output_data_array=output_array_path)
nxtest.check_filter_result(nx.CreateDataArray, result)


# ------------------------------------------------------------------------------
# Lets try a Rectilinear Grid Geometry
# We will need 3 arrays for the X, Y, Z created in the group RectGridCoords
# ------------------------------------------------------------------------------
result = nx.CreateDataGroup.execute(data_structure=data_structure,
                                    data_object_path=nx.DataPath('RectGridCoords'))
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataGroup filter")

output_array_path = nx.DataPath("RectGridCoords/X Coords")
array_type = nx.NumericType.float32
tuple_dims = [[10]]
create_array_nx_filter = nx.CreateDataArray()
result = create_array_nx_filter.execute(data_structure=data_structure,
                                     component_count=1,
                                     data_format="",
                                     initialization_value="0",
                                     numeric_type=array_type,
                                     output_data_array=output_array_path,
                                     tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArray, result)


x_coords = data_structure[output_array_path].npview()
x_coords = np.squeeze(x_coords, axis=1)
x_coords[:] = np.arange(0, 10, 1)

output_array_path = nx.DataPath("RectGridCoords/Y Coords")
array_type = nx.NumericType.float32
tuple_dims = [[10]]
create_array_nx_filter = nx.CreateDataArray()
result = create_array_nx_filter.execute(data_structure=data_structure,
                                     component_count=1,
                                     data_format="",
                                     initialization_value="0",
                                     numeric_type=array_type,
                                     output_data_array=output_array_path,
                                     tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArray, result)


y_coords = data_structure[output_array_path].npview()
y_coords = np.squeeze(y_coords, axis=1)
y_coords[:] = np.arange(10, 20, 1)

output_array_path = nx.DataPath("RectGridCoords/Z Coords")
array_type = nx.NumericType.float32
tuple_dims = [[10]]
create_array_nx_filter = nx.CreateDataArray()
result = create_array_nx_filter.execute(data_structure=data_structure,
                                     component_count=1,
                                     data_format="",
                                     initialization_value="0",
                                     numeric_type=array_type,
                                     output_data_array=output_array_path,
                                     tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArray, result)


z_coords = data_structure[output_array_path].npview()
z_coords = np.squeeze(z_coords, axis=1)
z_coords[:] = np.arange(20, 30, 1)

result = nx.CreateGeometryFilter.execute(data_structure=data_structure,
                                         array_handling=1,  # Move the arrays from their original location.
                                         cell_attribute_matrix_name="Cell Data",
                                         geometry_name=nx.DataPath("RectGrid Geometry"),
                                         geometry_type=1,
                                         x_bounds=nx.DataPath("RectGridCoords/X Coords"),
                                         y_bounds=nx.DataPath("RectGridCoords/Y Coords"),
                                         z_bounds=nx.DataPath("RectGridCoords/Z Coords")
                                         )
nxtest.check_filter_result(nx.CreateGeometryFilter, result)


rect_grid_geom = data_structure["RectGrid Geometry"]
x_cell_count = rect_grid_geom.num_x_cells
print(f'num_x_cells: {x_cell_count}')
x_bounds = rect_grid_geom.x_bounds
print(f'x_bounds: {x_bounds.store.npview()}')
print(f'id: {rect_grid_geom.id}')
print(f'name: {rect_grid_geom.name}')

# ------------------------------------------------------------------------------
# Lets try a Triangle Geometry
# For this we need the vertex data and the Triangle connectivity data
# ------------------------------------------------------------------------------
array_path = nx.DataPath('Vertices')
result = nx.CreateDataArray.execute(data_structure,
                                    numeric_type=nx.NumericType.float32,
                                    component_count=3,
                                    tuple_dimensions=[[144]],
                                    output_data_array=array_path,
                                    initialization_value='0')
nxtest.check_filter_result(nx.CreateDataArray, result)


# Read the CSV file into the DataArray using the numpy view
vertex_coords = data_structure[array_path].npview()
file_path = nxtest.get_simplnx_source_dir() / 'test/Data/VertexCoordinates.csv'
vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

array_path = nx.DataPath('Triangles')
result = nx.CreateDataArray.execute(data_structure,
                                    numeric_type=nx.NumericType.uint64,
                                    component_count=3,
                                    tuple_dimensions=[[242]],
                                    output_data_array=array_path,
                                    initialization_value='0')
nxtest.check_filter_result(nx.CreateDataArray, result)

# Read the CSV file into the DataArray using the numpy view
triangles = data_structure[array_path].npview()
file_path = nxtest.get_simplnx_source_dir() / 'test/Data/TriangleConnectivity.csv'
triangles[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

result = nx.CreateGeometryFilter.execute(data_structure=data_structure,
                                         array_handling=1,  # Move the arrays from their original location.
                                         geometry_name=nx.DataPath("Triangle Geometry"),
                                         geometry_type=4,
                                         face_attribute_matrix_name="Triangle Data",
                                         edge_attribute_matrix_name="Triangle Edge Data",
                                         vertex_attribute_matrix_name="Vertex Data",
                                         vertex_list_name=nx.DataPath('Vertices'),
                                         triangle_list_name=nx.DataPath('Triangles')
                                         )
nxtest.check_filter_result(nx.CreateGeometryFilter, result)


# ------------------------------------------------------------------------------
# Lets try a Edge Geometry
# For this we need the vertex data and the Edge connectivity data
# ------------------------------------------------------------------------------
array_path = nx.DataPath('Vertices')
result = nx.CreateDataArray.execute(data_structure,
                                    numeric_type=nx.NumericType.float32,
                                    component_count=3,
                                    tuple_dimensions=[[144]],
                                    output_data_array=array_path,
                                    initialization_value='0')
nxtest.check_filter_result(nx.CreateDataArray, result)


# Read the CSV file into the DataArray using the numpy view
vertex_coords = data_structure[array_path].npview()
file_path = nxtest.get_simplnx_source_dir() / 'test/Data/VertexCoordinates.csv'
vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

array_path = nx.DataPath('Edges')
result = nx.CreateDataArray.execute(data_structure,
                                    numeric_type=nx.NumericType.uint64,
                                    component_count=2,
                                    tuple_dimensions=[[264]],
                                    output_data_array=array_path,
                                    initialization_value='0')
nxtest.check_filter_result(nx.CreateDataArray, result)


# Read the CSV file into the DataArray using the numpy view
file_path = nxtest.get_simplnx_source_dir() / 'test/Data/EdgeConnectivity.csv'
edges_view = data_structure["Edges"].npview()
edges_view[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

result = nx.CreateGeometryFilter.execute(data_structure=data_structure,
                                         array_handling=1,  # Move the arrays from their original location.
                                         geometry_name=nx.DataPath("Edge Geometry"),
                                         geometry_type=3,
                                         edge_attribute_matrix_name="Edge Data",
                                         vertex_attribute_matrix_name="Vertex Data",
                                         vertex_list_name=nx.DataPath('Vertices'),
                                         edge_list_name=nx.DataPath('Edges')
                                         )
nxtest.check_filter_result(nx.CreateGeometryFilter, result)


output_file_path = nxtest.get_test_temp_directory() / "geometry_examples.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, export_file_path=output_file_path,
                                       write_xdmf_file=True)
nxtest.check_filter_result(nx.WriteDREAM3DFilter, result)

