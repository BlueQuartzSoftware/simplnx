from typing import List
from pathlib import Path
import simplnx as nx
import meshio
from .utilities import meshio_utilities as mu

class WriteGmshFile:

# -----------------------------------------------------------------------------
# These methods should not be edited
# -----------------------------------------------------------------------------
  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('2ab68db2-2923-471f-a8b7-cd506a852923')

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return 'WriteGmshFile'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return 'WriteGmshFile'

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  WriteGmshFile
    """
    return WriteGmshFile()

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
    return 'MeshIO: Write Gmsh File (Python)'
 
  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python', 'WriteGmshFile', '.msh', 'msh', 'format', 'input', 'output', 'abaqus', 'compute', 'generate', 'create']
  
  
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  INPUT_GEOMETRY_KEY = 'input_geometry_path'
  CELL_DATA_ARRAY_PATHS_KEY = 'cell_data_array_paths'
  POINT_DATA_ARRAY_PATHS_KEY = 'point_data_array_paths'
  OUTPUT_FILE_PATH_KEY = 'output_file_path'

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user interface
    and pack them up into a dictionary for use in the preflight and execute methods.
    """
    params = nx.Parameters()

    params.insert(params.Separator("Input Parameters"))
    params.insert(nx.GeometrySelectionParameter(WriteGmshFile.INPUT_GEOMETRY_KEY, 'Input Geometry', 'The input geometry that will be written to the ANSYS file.', nx.DataPath(), allowed_types={nx.IGeometry.Type.Edge, nx.IGeometry.Type.Triangle, nx.IGeometry.Type.Quad, nx.IGeometry.Type.Tetrahedral, nx.IGeometry.Type.Hexahedral}))
    params.insert(nx.MultiArraySelectionParameter(WriteGmshFile.CELL_DATA_ARRAY_PATHS_KEY, 'Cell Data Arrays To Write', 'The cell data arrays to write to the ANSYS file.', [], allowed_types={nx.IArray.ArrayType.DataArray}, allowed_data_types=nx.get_all_data_types()))
    params.insert(nx.MultiArraySelectionParameter(WriteGmshFile.POINT_DATA_ARRAY_PATHS_KEY, 'Point Data Arrays To Write', 'The point data arrays to write to the ANSYS file.', [], allowed_types={nx.IArray.ArrayType.DataArray}, allowed_data_types=nx.get_all_data_types()))

    params.insert(params.Separator("Output Parameters"))
    params.insert(nx.FileSystemPathParameter(WriteGmshFile.OUTPUT_FILE_PATH_KEY, 'Output File (.msh)', 'The output file that contains the specified geometry as a mesh.', '', extensions_type={'.msh'}, path_type=nx.FileSystemPathParameter.PathType.OutputFile))

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the array sizes are actually known at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example). If in doubt, set the tuple dimensions of an array to [1].
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """

    # Extract the values from the user interface from the 'args' 
    input_geometry_path: nx.DataPath = args[WriteGmshFile.INPUT_GEOMETRY_KEY]
    cell_data_array_paths = args[WriteGmshFile.CELL_DATA_ARRAY_PATHS_KEY]
    point_data_array_paths = args[WriteGmshFile.POINT_DATA_ARRAY_PATHS_KEY]

    errors, warnings = mu.preflight_meshio_writer_filter(data_structure=data_structure, input_geometry_path=input_geometry_path, cell_data_array_paths=cell_data_array_paths, point_data_array_paths=point_data_array_paths)
    
    # Return the output_actions so the changes are reflected in the User Interface.
    return nx.IFilter.PreflightResult(output_actions=nx.OutputActions(), errors=errors, warnings=warnings, preflight_values=None)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """
  
    # Extract the values from the user interface from the 'args' 
    input_geometry_path: nx.DataPath = args[WriteGmshFile.INPUT_GEOMETRY_KEY]
    output_file_path: Path = args[WriteGmshFile.OUTPUT_FILE_PATH_KEY]
    cell_data_array_paths = args[WriteGmshFile.CELL_DATA_ARRAY_PATHS_KEY]
    point_data_array_paths = args[WriteGmshFile.POINT_DATA_ARRAY_PATHS_KEY]

    return mu.execute_meshio_writer_filter(file_format='gmsh', data_structure=data_structure, input_geometry_path=input_geometry_path, cell_data_array_paths=cell_data_array_paths, point_data_array_paths=point_data_array_paths, output_file_path=output_file_path)