import simplnx as sx
import numpy as np
from typing import List, Dict
from pathlib import Path
from ExamplePlugin.utilities.cli_tools import parse_file, parse_geometry_array_names

class CliReaderFilter:
  CLI_FILE_PATH = 'cli_file_path'
  OUTPUT_EDGE_GEOM_PATH = 'output_edge_geom_path'
  OUTPUT_VERTEX_ATTRMAT_NAME = 'output_vertex_attrmat_name'
  OUTPUT_EDGE_ATTRMAT_NAME = 'output_edge_attrmat_name'
  OUTPUT_FEATURE_ATTRMAT_NAME = 'output_feature_attrmat_name'
  SHARED_VERTICES_ARRAY_NAME = 'shared_vertices_array_name'
  SHARED_EDGES_ARRAY_NAME = 'shared_edges_array_name'
  LAYER_ARRAY_NAME = 'Layer'
  LABEL_ARRAY_NAME = 'Label'

  def uuid(self) -> sx.Uuid:
    return sx.Uuid('ef2e9ad6-862c-4142-982f-704618a8855c')

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

  def parameters(self) -> sx.Parameters:
    params = sx.Parameters()

    params.insert(sx.Parameters.Separator("Parameters"))
    params.insert(sx.FileSystemPathParameter(CliReaderFilter.CLI_FILE_PATH, 'Input CLI File', 'The path to the input CLI file that will be read.', '', {'.cli'}, sx.FileSystemPathParameter.PathType.InputFile))
    params.insert(sx.Parameters.Separator("Created Data Objects"))
    params.insert(sx.DataGroupCreationParameter(CliReaderFilter.OUTPUT_EDGE_GEOM_PATH, 'Output Edge Geometry', 'The path to the newly created edge geometry.', sx.DataPath("[Edge Geometry]")))
    params.insert(sx.DataObjectNameParameter(CliReaderFilter.OUTPUT_VERTEX_ATTRMAT_NAME, 'Output Vertex Attribute Matrix Name', 'The name of the newly created vertex attribute matrix.', 'Vertex Data'))
    params.insert(sx.DataObjectNameParameter(CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME, 'Output Edge Attribute Matrix Name', 'The name of the newly created edge attribute matrix.', 'Edge Data'))
    params.insert(sx.DataObjectNameParameter(CliReaderFilter.OUTPUT_FEATURE_ATTRMAT_NAME, 'Output Feature Attribute Matrix Name', 'The name of the newly created feature attribute matrix.', 'Feature Data'))
    params.insert(sx.DataObjectNameParameter(CliReaderFilter.SHARED_VERTICES_ARRAY_NAME, 'Shared Vertices Array Name', 'The name of the newly created shared vertices array.', 'Shared Vertices'))
    params.insert(sx.DataObjectNameParameter(CliReaderFilter.SHARED_EDGES_ARRAY_NAME, 'Shared Edges Array Name', 'The name of the newly created shared edges array.', 'Shared Edges'))

    return params

  def preflight_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.PreflightResult:
    cli_file_path: str = args[CliReaderFilter.CLI_FILE_PATH]
    output_edge_geom_path: sx.DataPath = args[CliReaderFilter.OUTPUT_EDGE_GEOM_PATH]
    output_vertex_attrmat_name: str = args[CliReaderFilter.OUTPUT_VERTEX_ATTRMAT_NAME]
    output_edge_attrmat_name: str = args[CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME]
    output_feature_attrmat_name: str = args[CliReaderFilter.OUTPUT_FEATURE_ATTRMAT_NAME]
    shared_vertices_array_name: str = args[CliReaderFilter.SHARED_VERTICES_ARRAY_NAME]
    shared_edges_array_name: str = args[CliReaderFilter.SHARED_EDGES_ARRAY_NAME]

    output_actions = sx.OutputActions()
    output_actions.append_action(sx.CreateEdgeGeometryAction(geometry_path=output_edge_geom_path, num_edges=1, num_vertices=1, vertex_attribute_matrix_name=output_vertex_attrmat_name, edge_attribute_matrix_name=output_edge_attrmat_name, shared_vertices_name=shared_vertices_array_name, shared_edges_name=shared_edges_array_name))

    array_names, num_of_labels = parse_geometry_array_names(Path(cli_file_path))
    # Because extra geometric data is not included in the specification, we are setting 'Layer' array to int32 and all other arrays to float32
    edge_attr_mat_path = output_edge_geom_path.create_child_path(output_edge_attrmat_name)
    for array_name in array_names:
      dtype = sx.DataType.float32
      if array_name == self.LAYER_ARRAY_NAME:
        dtype = sx.DataType.int32
      array_path = edge_attr_mat_path.create_child_path(array_name)
      output_actions.append_action(sx.CreateArrayAction(dtype, [1], [1], array_path))
    
    if num_of_labels > 0:
      array_path = edge_attr_mat_path.create_child_path(self.LABEL_ARRAY_NAME)
      output_actions.append_action(sx.CreateArrayAction(sx.DataType.int32, [1], [1], array_path))

      feature_attr_mat_path = output_edge_geom_path.create_child_path(output_feature_attrmat_name)
      output_actions.append_action(sx.CreateAttributeMatrixAction(feature_attr_mat_path, [num_of_labels]))

      label_array_path = feature_attr_mat_path.create_child_path(self.LABEL_ARRAY_NAME)
      output_actions.append_action(sx.CreateStringArrayAction([num_of_labels], label_array_path))

    return sx.IFilter.PreflightResult(output_actions)

  def execute_impl(self, data_structure: sx.DataStructure, args: dict, message_handler: sx.IFilter.MessageHandler, should_cancel: sx.AtomicBoolProxy) -> sx.IFilter.ExecuteResult:
    cli_file_path: str = args[CliReaderFilter.CLI_FILE_PATH]
    output_edge_geom_path: sx.DataPath = args[CliReaderFilter.OUTPUT_EDGE_GEOM_PATH]
    output_edge_attrmat_name: str = args[CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME]
    output_feature_attrmat_name: str = args[CliReaderFilter.OUTPUT_FEATURE_ATTRMAT_NAME]

    layer_features = []

    try:
      layer_features, layer_heights, hatch_labels = parse_file(Path(cli_file_path))
    except Exception as e:
      return sx.Result([sx.Error(-1000, f"An error occurred while parsing the CLI file '{cli_file_path}': {e}")])

    start_vertices = []
    end_vertices = []
    data_arrays = {}
    if len(hatch_labels) > 0:
      data_arrays[self.LABEL_ARRAY_NAME] = []
    num_of_hatches = 0
    for layer_idx in range(len(layer_features)):
      layer = layer_features[layer_idx]
      if not layer:
        message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f'Skipping layer {layer_idx + 1}/{len(layer_features)}...'))
        continue

      message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f'Importing layer {layer_idx + 1}/{len(layer_features)}...'))

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
        data_arrays[self.LABEL_ARRAY_NAME].extend([hatch.hatch_id] * hatch.n)

    edge_geom: sx.EdgeGeom = data_structure[output_edge_geom_path]
    
    message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f'Saving Vertex List...'))
    vertex_list = [item for pair in zip(start_vertices, end_vertices) for item in pair]
    vertices_array = edge_geom.vertices
    vertices_array.resize_tuples([len(vertex_list)])
    vertices_view = vertices_array.store.npview()
    vertices_view[:] = vertex_list

    message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f'Saving Edges...'))
    edges_array = edge_geom.edges
    edges_array.resize_tuples([num_of_hatches])
    edges_view = edges_array.store.npview()
    edges_view[:] = [[i, i+1] for i in range(0, len(vertex_list), 2)]

    edge_attr_mat_path = output_edge_geom_path.create_child_path(output_edge_attrmat_name)
    for array_name, values in data_arrays.items():
      message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f"Saving Cell Array '{array_name}'..."))
      array_path = edge_attr_mat_path.create_child_path(array_name)
      array: sx.IDataArray = data_structure[array_path]
      array.resize_tuples([num_of_hatches])
      values_arr = np.array(values)
      values_arr = values_arr.reshape([len(values)] + array.cdims)
      array_view = array.store.npview()
      array_view[:] = values_arr
    
    feature_attr_mat_path = output_edge_geom_path.create_child_path(output_feature_attrmat_name)
    label_feature_array_path = feature_attr_mat_path.create_child_path(self.LABEL_ARRAY_NAME)
    label_feature_array: sx.StringArray = data_structure[label_feature_array_path]
    message_handler(sx.IFilter.Message(sx.IFilter.Message.Type.Info, f"Saving Feature Array '{self.LABEL_ARRAY_NAME}'..."))
    label_feature_array.initialize_with_list(list(hatch_labels.values()))
    return sx.Result()