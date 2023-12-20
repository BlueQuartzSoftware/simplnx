import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

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
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter filter")

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
create_array_filter = nx.CreateDataArray()
result = create_array_filter.execute(data_structure=data_structure, component_count=1, data_format="",
                                     initialization_value="10",
                                     numeric_type=array_type, output_data_array=output_array_path)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

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
create_array_filter = nx.CreateDataArray()
result = create_array_filter.execute(data_structure=data_structure,
                                     component_count=1,
                                     data_format="",
                                     initialization_value="0",
                                     numeric_type=array_type,
                                     output_data_array=output_array_path,
                                     tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

x_coords = data_structure[output_array_path].npview()
x_coords = np.squeeze(x_coords, axis=1)
x_coords[:] = np.arange(0, 10, 1)

output_array_path = nx.DataPath("RectGridCoords/Y Coords")
array_type = nx.NumericType.float32
tuple_dims = [[10]]
create_array_filter = nx.CreateDataArray()
result = create_array_filter.execute(data_structure=data_structure,
                                     component_count=1,
                                     data_format="",
                                     initialization_value="0",
                                     numeric_type=array_type,
                                     output_data_array=output_array_path,
                                     tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

y_coords = data_structure[output_array_path].npview()
y_coords = np.squeeze(y_coords, axis=1)
y_coords[:] = np.arange(10, 20, 1)

output_array_path = nx.DataPath("RectGridCoords/Z Coords")
array_type = nx.NumericType.float32
tuple_dims = [[10]]
create_array_filter = nx.CreateDataArray()
result = create_array_filter.execute(data_structure=data_structure,
                                     component_count=1,
                                     data_format="",
                                     initialization_value="0",
                                     numeric_type=array_type,
                                     output_data_array=output_array_path,
                                     tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

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
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter filter")

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
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

# Read the CSV file into the DataArray using the numpy view
vertex_coords = data_structure[array_path].npview()
file_path = 'simplnx/test/Data/VertexCoordinates.csv'
vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

array_path = nx.DataPath('Triangles')
result = nx.CreateDataArray.execute(data_structure,
                                    numeric_type=nx.NumericType.uint64,
                                    component_count=3,
                                    tuple_dimensions=[[242]],
                                    output_data_array=array_path,
                                    initialization_value='0')
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")
# Read the CSV file into the DataArray using the numpy view
triangles = data_structure[array_path].npview()
file_path = 'simplnx/test/Data/TriangleConnectivity.csv'
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
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter (Triangle) filter")

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
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

# Read the CSV file into the DataArray using the numpy view
vertex_coords = data_structure[array_path].npview()
file_path = 'simplnx/test/Data/VertexCoordinates.csv'
vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

array_path = nx.DataPath('Edges')
result = nx.CreateDataArray.execute(data_structure,
                                    numeric_type=nx.NumericType.uint64,
                                    component_count=2,
                                    tuple_dimensions=[[264]],
                                    output_data_array=array_path,
                                    initialization_value='0')
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")

# Read the CSV file into the DataArray using the numpy view
file_path = 'simplnx/test/Data/EdgeConnectivity.csv'
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
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter (Edge) filter")



output_file_path = "geometry_examples.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, export_file_path=output_file_path, write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the WriteDREAM3DFilter filter")
