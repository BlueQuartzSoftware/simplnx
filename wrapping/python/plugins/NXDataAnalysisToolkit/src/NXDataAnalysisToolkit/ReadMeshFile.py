from typing import List
from pathlib import Path
import simplnx as nx
import meshio
import numpy as np
from .common.Result import Result, make_error_result, make_warning_result
from .utilities import meshio_utilities as mu

# Cache variables
_input_file_path_cache: Path = None
_mesh_cache: meshio.Mesh = None

class ReadMeshFile:

# -----------------------------------------------------------------------------
# These methods should not be edited
# -----------------------------------------------------------------------------
  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('f2b29688-91c7-4c48-9e7d-edd1b582f0b7')

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'ReadMeshFile'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'ReadMeshFile'

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  ReadMeshFile
    """
    return ReadMeshFile()

# -----------------------------------------------------------------------------
# These methods CAN (and probably should) be updated. For instance, the 
# human_name() is what users of the filter will see in the DREAM3D-NX GUI. You
# might want to consider putting spaces between workd, using proper capitalization
# and putting "(Python)" at the end of the name (or beginning if you want the 
# filter list to group your filters togther)
# -----------------------------------------------------------------------------
  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return 'MeshIO: Read Mesh File (Python)'
 
  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python', 'ReadMeshFile', 'inp', 'msh', 'med', 'node', 'ele', 'vtu', 'vtk', 'abaqus', 'ansys', 'gmsh', 'tetgen', 'format', 'input', 'import', 'ingest', 'compute', 'generate', 'create']
  
  
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  INPUT_FILE_PATH_KEY = 'input_file_path'
  CREATED_GEOMETRY_PATH = 'created_geometry_path'
  VERTEX_ATTR_MATRIX_NAME = 'vertex_attr_matrix_name'
  CELL_ATTR_MATRIX_NAME = 'cell_attr_matrix_name'
  VERTEX_ARRAY_NAME = 'vertex_array_name'
  CELL_ARRAY_NAME = 'cell_array_name'

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user interface
    and pack them up into a dictionary for use in the preflight and execute methods.
    """
    params = nx.Parameters()

    params.insert(params.Separator("Input Parameters"))
    params.insert(nx.FileSystemPathParameter(ReadMeshFile.INPUT_FILE_PATH_KEY, 'Input File (.inp, .msh, .med, .node, .ele, .vtu)', 'The input file that contains the mesh which will be read in as a geometry.', '', extensions_type={'.inp', '.msh', '.med', '.node', '.ele', '.vtu'}, path_type=nx.FileSystemPathParameter.PathType.InputFile))

    params.insert(params.Separator("Created Parameters"))
    params.insert(nx.DataGroupCreationParameter(ReadMeshFile.CREATED_GEOMETRY_PATH, 'Created Geometry', 'The path to where the geometry will be created in the data structure.', nx.DataPath()))
    params.insert(nx.DataObjectNameParameter(ReadMeshFile.VERTEX_ATTR_MATRIX_NAME, 'Vertex Attribute Matrix Name', 'The name of the vertex attribute matrix that will be created inside the geometry.', 'Vertex Data'))
    params.insert(nx.DataObjectNameParameter(ReadMeshFile.CELL_ATTR_MATRIX_NAME, 'Cell Attribute Matrix Name', 'The name of the cell attribute matrix that will be created inside the geometry.', 'Cell Data'))
    params.insert(nx.DataObjectNameParameter(ReadMeshFile.VERTEX_ARRAY_NAME, 'Vertex Array Name', 'The name of the vertex array that will be created inside the geometry.', 'Vertices'))
    params.insert(nx.DataObjectNameParameter(ReadMeshFile.CELL_ARRAY_NAME, 'Cell Array Name', 'The name of the cell array that will be created inside the geometry.', 'Cells'))

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the array sizes are actually known at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example). If in doubt, set the tuple dimensions of an array to [1].
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """
    # Extract the values from the user interface from the 'args'
    input_file_path: Path = args[ReadMeshFile.INPUT_FILE_PATH_KEY]
    created_geometry_path: nx.DataPath = args[ReadMeshFile.CREATED_GEOMETRY_PATH]
    vertex_attr_matrix_name: str = args[ReadMeshFile.VERTEX_ATTR_MATRIX_NAME]
    cell_attr_matrix_name: str = args[ReadMeshFile.CELL_ATTR_MATRIX_NAME]
    vertex_array_name: str = args[ReadMeshFile.VERTEX_ARRAY_NAME]
    cell_array_name: str = args[ReadMeshFile.CELL_ARRAY_NAME]

    # Read the mesh file and then cache the contents... we need to know which geometry to create
    global _input_file_path_cache, _mesh_cache
    if _input_file_path_cache is None or _mesh_cache is None or _input_file_path_cache != input_file_path:
      _mesh_cache = meshio.read(str(input_file_path))
      _input_file_path_cache = input_file_path
    
    if len(_mesh_cache.cells) == 0:
      return nx.IFilter.PreflightResult(errors=[make_error_result(code=-3040, message=f"Mesh file '{str(input_file_path)}' does not contain a cell type.  A cell type is required to be able to create a geometry from the mesh file!")])

    if len(_mesh_cache.cells) > 1:
      return nx.IFilter.PreflightResult(errors=[make_error_result(code=-3041, message=f"Mesh file '{str(input_file_path)}' has more than one declared cell type.  Multiple cell types in one file are not supported.")])
    
    # Create the proper geometry
    output_actions: nx.OutputActions = nx.OutputActions()
    warnings: List[nx.Warning] = []
    cell_type: str = _mesh_cache.cells[0].type
    cells_array: np.ndarray = _mesh_cache.cells[0].data
    points_array: np.ndarray = _mesh_cache.points

    if cell_type == mu.EDGE_TYPE_STR:
      output_actions.append_action(nx.CreateEdgeGeometryAction(created_geometry_path, cells_array.shape[0], points_array.shape[0], vertex_attr_matrix_name, cell_attr_matrix_name, vertex_array_name, cell_array_name))
    elif cell_type == mu.TRIANGLE_TYPE_STR:
      output_actions.append_action(nx.CreateTriangleGeometryAction(created_geometry_path, cells_array.shape[0], points_array.shape[0], vertex_attr_matrix_name, cell_attr_matrix_name, vertex_array_name, cell_array_name))
    elif cell_type == mu.QUAD_TYPE_STR:
      output_actions.append_action(nx.CreateQuadGeometryAction(created_geometry_path, cells_array.shape[0], points_array.shape[0], vertex_attr_matrix_name, cell_attr_matrix_name, vertex_array_name, cell_array_name))
    elif cell_type == mu.TETRAHEDRAL_TYPE_STR:
      output_actions.append_action(nx.CreateTetrahedralGeometryAction(created_geometry_path, cells_array.shape[0], points_array.shape[0], vertex_attr_matrix_name, cell_attr_matrix_name, vertex_array_name, cell_array_name))
    elif cell_type == mu.HEXAHEDRAL_TYPE_STR:
      output_actions.append_action(nx.CreateHexahedralGeometryAction(created_geometry_path, cells_array.shape[0], points_array.shape[0], vertex_attr_matrix_name, cell_attr_matrix_name, vertex_array_name, cell_array_name))
    else:
      return nx.IFilter.PreflightResult(errors=[nx.Error(code=-3042, message=f"Unsupported mesh type '{cell_type}'.  Only '{mu.EDGE_TYPE_STR}', '{mu.TRIANGLE_TYPE_STR}', '{mu.QUAD_TYPE_STR}', '{mu.TETRAHEDRAL_TYPE_STR}', and '{mu.HEXAHEDRAL_TYPE_STR}' types are supported.")])

    # NOTE: This filter makes the assumption for cell and point data that the first element in the numpy shape
    # list is the only tuple dimension, and every other shape list element is part of the component dimensions!

    # Create the cell data arrays
    cell_attr_mat_path = created_geometry_path.create_child_path(cell_attr_matrix_name)
    for cell_data_array_name, cell_data_array in _mesh_cache.cell_data.items():
      cell_data_array = cell_data_array[0]
      cell_array_path = cell_attr_mat_path.create_child_path(cell_data_array_name)
      cell_data_tuple_dims = [cell_data_array.shape[0]]
      cell_data_comp_dims = cell_data_array.shape[1:]
      if not cell_data_comp_dims:
          cell_data_comp_dims = [1]
      output_actions.append_action(nx.CreateArrayAction(nx.convert_np_dtype_to_datatype(cell_data_array.dtype), cell_data_tuple_dims, cell_data_comp_dims, cell_array_path))

    # Create the point data arrays
    vertex_attr_mat_path = created_geometry_path.create_child_path(vertex_attr_matrix_name)
    for point_data_array_name, point_data_array in _mesh_cache.point_data.items():
      point_data_array = point_data_array[0]
      point_array_path = vertex_attr_mat_path.create_child_path(point_data_array_name)
      point_data_tuple_dims = [point_data_array.shape[0]]
      point_data_comp_dims = point_data_array.shape[1:]
      if not point_data_comp_dims:
          point_data_comp_dims = [1]
      output_actions.append_action(nx.CreateArrayAction(nx.convert_np_dtype_to_datatype(point_data_array.dtype), point_data_tuple_dims, point_data_comp_dims, point_array_path))

    # Return the output_actions so the changes are reflected in the User Interface.
    return nx.IFilter.PreflightResult(output_actions=output_actions, warnings=warnings)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """
    # Extract the values from the user interface from the 'args'
    # This is basically repeated from the preflight because the variables are scoped to the method()
    created_geometry_path: nx.DataPath = args[ReadMeshFile.CREATED_GEOMETRY_PATH]
    vertex_attr_matrix_name: str = args[ReadMeshFile.VERTEX_ATTR_MATRIX_NAME]
    cell_attr_matrix_name: str = args[ReadMeshFile.CELL_ATTR_MATRIX_NAME]

    # Grab the geometry from the data structure
    geometry: nx.INodeGeometry1D = data_structure[created_geometry_path]

    # Grab the proper cells array from the geometry
    if isinstance(geometry, nx.INodeGeometry3D):
      cells_array = geometry.polyhedra
    elif isinstance(geometry, nx.INodeGeometry2D):
      cells_array = geometry.faces
    elif isinstance(geometry, nx.INodeGeometry1D):
      cells_array = geometry.edges
    else:
      # This SHOULD NOT happen, but we'll check for it anyways...
      return Result(errors=[make_error_result(code=-3042, message=f"Created geometry at path '{str(created_geometry_path)}' with type '{type(geometry).__name__}' is not a 1D, 2D, or 3D node-based geometry.")])

    # Copy over the vertices
    vertices_np_array = geometry.vertices.npview()
    vertices_np_array[:] = _mesh_cache.points[:]

    # Copy over the cells
    cells_np_array = cells_array.npview()
    cells_np_array[:] = _mesh_cache.cells[0].data[:]

    # Copy over the cell data
    cell_attr_mat_path = created_geometry_path.create_child_path(cell_attr_matrix_name)
    for cell_data_array_name, cell_data_array in _mesh_cache.cell_data.items():
      cell_data_array = cell_data_array[0]
      cell_data_array_path = cell_attr_mat_path.create_child_path(cell_data_array_name)
      cell_data_np_array = np.squeeze(data_structure[cell_data_array_path].npview())
      cell_data_np_array[:] = _mesh_cache.cell_data[cell_data_array_name][0][:]

    # Copy over the point data
    vertex_attr_mat_path = created_geometry_path.create_child_path(vertex_attr_matrix_name)
    for point_data_array_name, point_data_array in _mesh_cache.point_data.items():
      point_data_array = point_data_array[0]
      point_data_array_path = vertex_attr_mat_path.create_child_path(point_data_array_name)
      point_data_np_array = np.squeeze(data_structure[point_data_array_path].npview())
      point_data_np_array[:] = _mesh_cache.point_data[point_data_array_name][0][:]

    return nx.Result()