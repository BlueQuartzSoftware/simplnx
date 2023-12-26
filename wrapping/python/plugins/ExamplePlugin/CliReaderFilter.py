import complex as cx
import numpy as np
from typing import List, Dict
from pathlib import Path
from ExamplePlugin.utilities.cli_tools import parse_file, parse_geometry_array_info

class CliReaderFilter:
  CLI_FILE_PATH = 'cli_file_path'
  OUTPUT_EDGE_GEOM_PATH = 'output_edge_geom_path'
  OUTPUT_VERTEX_ATTRMAT_NAME = 'output_vertex_attrmat_name'
  OUTPUT_EDGE_ATTRMAT_NAME = 'output_edge_attrmat_name'
  SHARED_VERTICES_ARRAY_NAME = 'shared_vertices_array_name'
  SHARED_EDGES_ARRAY_NAME = 'shared_edges_array_name'

  def uuid(self) -> cx.Uuid:
    return cx.Uuid('ef2e9ad6-862c-4142-982f-704618a8855c')

  def human_name(self) -> str:
    return 'Read CLI File (Python)'

  def class_name(self) -> str:
    return 'CliReaderFilter'

  def name(self) -> str:
    return 'CliReaderFilter'

  def default_tags(self) -> List[str]:
    return ['python']

  def clone(self):
    return CliReaderFilter()

  def parameters(self) -> cx.Parameters:
    params = cx.Parameters()

    params.insert(cx.Parameters.Separator("Parameters"))
    params.insert(cx.FileSystemPathParameter(CliReaderFilter.CLI_FILE_PATH, 'Input CLI File', 'The path to the input CLI file that will be read.', '', {'.cli'}, cx.FileSystemPathParameter.PathType.InputFile))
    params.insert(cx.Parameters.Separator("Created Data Objects"))
    params.insert(cx.DataGroupCreationParameter(CliReaderFilter.OUTPUT_EDGE_GEOM_PATH, 'Output Edge Geometry', 'The path to the newly created edge geometry.', cx.DataPath("[Edge Geometry]")))
    params.insert(cx.DataObjectNameParameter(CliReaderFilter.OUTPUT_VERTEX_ATTRMAT_NAME, 'Output Vertex Attribute Matrix Name', 'The name of the newly created vertex attribute matrix.', 'Vertex Data'))
    params.insert(cx.DataObjectNameParameter(CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME, 'Output Edge Attribute Matrix Name', 'The name of the newly created edge attribute matrix.', 'Edge Data'))
    params.insert(cx.DataObjectNameParameter(CliReaderFilter.SHARED_VERTICES_ARRAY_NAME, 'Shared Vertices Array Name', 'The name of the newly created shared vertices array.', 'Shared Vertices'))
    params.insert(cx.DataObjectNameParameter(CliReaderFilter.SHARED_EDGES_ARRAY_NAME, 'Shared Edges Array Name', 'The name of the newly created shared edges array.', 'Shared Edges'))

    return params

  def preflight_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.PreflightResult:
    cli_file_path: str = args[CliReaderFilter.CLI_FILE_PATH]
    output_edge_geom_path: cx.DataPath = args[CliReaderFilter.OUTPUT_EDGE_GEOM_PATH]
    output_vertex_attrmat_name: str = args[CliReaderFilter.OUTPUT_VERTEX_ATTRMAT_NAME]
    output_edge_attrmat_name: str = args[CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME]
    shared_vertices_array_name: str = args[CliReaderFilter.SHARED_VERTICES_ARRAY_NAME]
    shared_edges_array_name: str = args[CliReaderFilter.SHARED_EDGES_ARRAY_NAME]

    output_actions = cx.OutputActions()
    output_actions.append_action(cx.CreateEdgeGeometryAction(geometry_path=output_edge_geom_path, num_edges=1, num_vertices=1, vertex_attribute_matrix_name=output_vertex_attrmat_name, edge_attribute_matrix_name=output_edge_attrmat_name, shared_vertices_name=shared_vertices_array_name, shared_edges_name=shared_edges_array_name))

    array_info: Dict[str, np.dtype] = parse_geometry_array_info(Path(cli_file_path))
    for array_name, np_type in array_info.items():
      array_path = output_edge_geom_path.create_child_path(output_edge_attrmat_name).create_child_path(array_name)
      output_actions.append_action(cx.CreateArrayAction(cx.DataType.float32, [1], [1], array_path))

    return cx.IFilter.PreflightResult(output_actions)

  def execute_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.ExecuteResult:
    cli_file_path: str = args[CliReaderFilter.CLI_FILE_PATH]
    output_edge_geom_path: cx.DataPath = args[CliReaderFilter.OUTPUT_EDGE_GEOM_PATH]
    output_edge_attrmat_name: str = args[CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME]

    layer_features = []

    try:
      layer_features, layer_heights = parse_file(Path(cli_file_path))
    except Exception as e:
      return cx.Result([cx.Error(-1000, f"An error occurred while parsing the CLI file '{cli_file_path}': {e}")])

    start_vertices = []
    end_vertices = []
    data_arrays = {}
    num_of_hatches = 0
    for layer_idx in range(len(layer_features)):
      layer = layer_features[layer_idx]
      if not layer:
        message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f'Skipping layer {layer_idx + 1}/{len(layer_features)}...'))
        continue

      message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f'Importing layer {layer_idx + 1}/{len(layer_features)}...'))
      for hatch in layer:
        num_of_hatches += hatch.n
        for start_x, start_y in zip(hatch.start_xvals, hatch.start_yvals):
          start_vertices.append([start_x, start_y, hatch.z_height])
        for end_x, end_y in zip(hatch.end_xvals, hatch.end_yvals):
          end_vertices.append([end_x, end_y, hatch.z_height])
        for array_name, value in hatch.data.items():
          if not array_name in data_arrays:
            data_arrays[array_name] = [value] * hatch.n
          else:
            data_arrays[array_name].extend([value] * hatch.n)

    edge_geom: cx.EdgeGeom = data_structure[output_edge_geom_path]
    
    message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f'Saving Vertex List...'))
    vertex_list = [item for pair in zip(start_vertices, end_vertices) for item in pair]
    vertices_array = edge_geom.vertices
    vertices_array.resize_tuples([len(vertex_list)])
    vertices_view = vertices_array.store.npview()
    vertices_view[:] = vertex_list

    message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f'Saving Edges...'))
    edges_array = edge_geom.edges
    edges_array.resize_tuples([num_of_hatches])
    edges_view = edges_array.store.npview()
    edges_view[:] = [[i, i+1] for i in range(0, len(vertex_list), 2)]

    for array_name, values in data_arrays.items():
      message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f"Saving Array '{array_name}'..."))
      array_path = output_edge_geom_path.create_child_path(output_edge_attrmat_name).create_child_path(array_name)
      array: cx.IDataArray = data_structure[array_path]
      array.resize_tuples([num_of_hatches])
      values_arr = np.array(values)
      values_arr = values_arr.reshape([len(values)] + array.cdims)
      array_view = array.store.npview()
      array_view[:] = values_arr

    return cx.Result()