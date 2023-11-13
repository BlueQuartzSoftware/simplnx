import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

"""
In the code below we are using some data files that are found in the complex
source code repository. It will up to the programmer to correctly set the paths
to the files before running this example.

"""
# Create the DataStructure object
data_structure = cx.DataStructure()
ig_dims = [10, 20, 30] # NOTE: These are in XYZ order
result = cx.CreateGeometryFilter.execute(  data_structure=data_structure,
    array_handling= 0,  # This does not matter for Image Geometry
    cell_attribute_matrix_name="Cell Data",
    dimensions=ig_dims, # Note that the dimensions are list as  X, Y, Z
    geometry_name=cx.DataPath(["Image Geometry"]),
    geometry_type=0, # 0 = Image Geometry
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0])
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter filter")


# Now we can create some (or import from another source) some cell based data
# this is data that lives at the center of each cell
# NOTE: we do *not* need to set the tuple dimensions because we are adding this array to the 
# AttributeMatrix that we generated in the last filter.
output_array_path = cx.DataPath(["Image Geometry", "Cell Data", "Float Cell Data"])
array_type = cx.NumericType.float32
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, component_count=1, data_format="", initialization_value="10", 
                            numeric_type=array_type, output_data_array=output_array_path)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")



#------------------------------------------------------------------------------
# Lets try a Rectilinear Grid Geometry
# We will need 3 arrays for the X, Y, Z created in the group RectGridCoords
#------------------------------------------------------------------------------
result = cx.CreateDataGroup.execute(data_structure=data_structure,
                                    Data_Object_Path=cx.DataPath(['RectGridCoords']))

output_array_path = cx.DataPath(["RectGridCoords", "X Coords"])
array_type = cx.NumericType.float32
tuple_dims = [[10]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, 
                                      component_count=1, 
                                      data_format="", 
                                      initialization_value="0", 
                                        numeric_type=array_type, 
                                        output_data_array=output_array_path, 
                                        tuple_dimensions=tuple_dims)

x_coords =  data_structure[output_array_path].store.npview()
x_coords = np.squeeze(x_coords, axis=1)
x_coords[:] = np.arange(0, 10, 1)

output_array_path = cx.DataPath(["RectGridCoords", "Y Coords"])
array_type = cx.NumericType.float32
tuple_dims = [[10]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, 
                                      component_count=1, 
                                      data_format="", 
                                      initialization_value="0", 
                                        numeric_type=array_type, 
                                        output_data_array=output_array_path, 
                                        tuple_dimensions=tuple_dims)

y_coords =  data_structure[output_array_path].store.npview()
y_coords = np.squeeze(y_coords, axis=1)
y_coords[:] = np.arange(10, 20, 1)

output_array_path = cx.DataPath(["RectGridCoords", "Z Coords"])
array_type = cx.NumericType.float32
tuple_dims = [[10]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, 
                                      component_count=1, 
                                      data_format="", 
                                      initialization_value="0", 
                                        numeric_type=array_type, 
                                        output_data_array=output_array_path, 
                                        tuple_dimensions=tuple_dims)

z_coords =  data_structure[output_array_path].store.npview()
z_coords = np.squeeze(z_coords, axis=1)
z_coords[:] = np.arange(20, 30, 1)

result = cx.CreateGeometryFilter.execute(data_structure=data_structure,
    array_handling= 1,  # Move the arrays from their original location.
    cell_attribute_matrix_name="Cell Data",
    geometry_name=cx.DataPath(["RectGrid Geometry"]),
    geometry_type=1,
    x_bounds=cx.DataPath(["RectGridCoords", "X Coords"]),
    y_bounds=cx.DataPath(["RectGridCoords", "Y Coords"]),
    z_bounds=cx.DataPath(["RectGridCoords", "Z Coords"])
    )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter filter")



#------------------------------------------------------------------------------
# Lets try a Triangle Geometry
# For this we need the vertex data and the Triangle connectivity data
#------------------------------------------------------------------------------
array_path = cx.DataPath(['Vertices'])
result = cx.CreateDataArray.execute(data_structure,
                                    numeric_type=cx.NumericType.float32,
                                    component_count=3,
                                    tuple_dimensions=[[144]],
                                    output_data_array=array_path,
                                    initialization_value='0')

# Read the CSV file into the DataArray using the numpy view
vertex_coords = data_structure[array_path].store.npview()
file_path = 'complex/test/Data/VertexCoordinates.csv'
vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)


array_path = cx.DataPath(['Triangles'])
result = cx.CreateDataArray.execute(data_structure,
                                    numeric_type=cx.NumericType.uint64,
                                    component_count=3,
                                    tuple_dimensions=[[242]],
                                    output_data_array=array_path,
                                    initialization_value='0')

# Read the CSV file into the DataArray using the numpy view
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


#------------------------------------------------------------------------------
# Lets try a Edge Geometry
# For this we need the vertex data and the Edge connectivity data
#------------------------------------------------------------------------------
array_path = cx.DataPath(['Vertices'])
result = cx.CreateDataArray.execute(data_structure,
                                    numeric_type=cx.NumericType.float32,
                                    component_count=3,
                                    tuple_dimensions=[[144]],
                                    output_data_array=array_path,
                                    initialization_value='0')

# Read the CSV file into the DataArray using the numpy view
vertex_coords = data_structure[array_path].store.npview()
file_path = 'complex/test/Data/VertexCoordinates.csv'
vertex_coords[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)


array_path = cx.DataPath(['Edges'])
result = cx.CreateDataArray.execute(data_structure,
                                    numeric_type=cx.NumericType.uint64,
                                    component_count=2,
                                    tuple_dimensions=[[264]],
                                    output_data_array=array_path,
                                    initialization_value='0')

# Read the CSV file into the DataArray using the numpy view
edges = data_structure[array_path].store.npview()
file_path = 'complex/test/Data/EdgeConnectivity.csv'
edges[:] = np.loadtxt(file_path, delimiter=',', skiprows=1)

result = cx.CreateGeometryFilter.execute(data_structure=data_structure,
    array_handling= 1,  # Move the arrays from their original location.
    geometry_name=cx.DataPath(["Edge Geometry"]),
    geometry_type=3,
    edge_attribute_matrix_name="Edge Data",
    vertex_attribute_matrix_name="Vertex Data",
    vertex_list_name=cx.DataPath(['Vertices']),
    edge_list_name=cx.DataPath(['Edges'])
    )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter (Edge) filter")



output_file_path = "geometry_examples.dream3d"
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, export_file_path=output_file_path, write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the WriteDREAM3DFilter filter")
