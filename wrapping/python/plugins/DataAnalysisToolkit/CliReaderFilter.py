# https://www.hmilch.net/downloads/cli_format.html

import simplnx as nx
import numpy as np
import sys
from typing import List, Dict
from pathlib import Path
from DataAnalysisToolkit.utilities.cli_tools import parse_file, parse_geometry_array_names

class CliReaderFilter:
  CLI_FILE_PATH = 'cli_file_path'
  MASK_X_DIMENSION = 'mask_x_dimension'
  MASK_Y_DIMENSION = 'mask_y_dimension'
  MASK_Z_DIMENSION = 'mask_z_dimension'
  MIN_MAX_X_COORDS = 'min_max_x_coords'
  MIN_MAX_Y_COORDS = 'min_max_y_coords'
  MIN_MAX_Z_COORDS = 'min_max_z_coords'
  OUTPUT_EDGE_GEOM_PATH = 'output_edge_geom_path'
  OUTPUT_VERTEX_ATTRMAT_NAME = 'output_vertex_attrmat_name'
  OUTPUT_EDGE_ATTRMAT_NAME = 'output_edge_attrmat_name'
  OUTPUT_FEATURE_ATTRMAT_NAME = 'output_feature_attrmat_name'
  SHARED_VERTICES_ARRAY_NAME = 'shared_vertices_array_name'
  SHARED_EDGES_ARRAY_NAME = 'shared_edges_array_name'
  LAYER_ARRAY_NAME = 'Layer'
  LABEL_ARRAY_NAME = 'Label'

  def uuid(self) -> nx.Uuid:
    return nx.Uuid('ef2e9ad6-862c-4142-982f-704618a8855c')

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

  def parameters(self) -> nx.Parameters:
    params = nx.Parameters()

    params.insert(nx.Parameters.Separator("Parameters"))
    params.insert(nx.FileSystemPathParameter(CliReaderFilter.CLI_FILE_PATH, 'Input CLI File', 'The path to the input CLI file that will be read.', '', {'.cli'}, nx.FileSystemPathParameter.PathType.InputFile))
    params.insert_linkable_parameter(nx.BoolParameter(CliReaderFilter.MASK_X_DIMENSION, 'Mask X Dimension', 'Determines whether or not to use X bounds to mask out any part of the dataset that is outside the bounds in the X dimension.', False))
    params.insert(nx.VectorFloat64Parameter(CliReaderFilter.MIN_MAX_X_COORDS, 'X Min/Max', 'The minimum and maximum X coordinate for the X bounds.', [0.0, 100.0], ['X Min', 'X Max']))
    params.insert_linkable_parameter(nx.BoolParameter(CliReaderFilter.MASK_Y_DIMENSION, 'Mask Y Dimension', 'Determines whether or not to use Y bounds to mask out any part of the dataset that is outside the bounds in the Y dimension.', False))
    params.insert(nx.VectorFloat64Parameter(CliReaderFilter.MIN_MAX_Y_COORDS, 'Y Min/Max', 'The minimum and maximum Y coordinate for the Y bounds.', [0.0, 100.0], ['Y Min', 'Y Max']))
    params.insert_linkable_parameter(nx.BoolParameter(CliReaderFilter.MASK_Z_DIMENSION, 'Mask Z Dimension', 'Determines whether or not to use Z bounds to mask out any part of the dataset that is outside the bounds in the Z dimension.', False))
    params.insert(nx.VectorFloat64Parameter(CliReaderFilter.MIN_MAX_Z_COORDS, 'Z Min/Max', 'The minimum and maximum Z coordinate for the Z bounds.', [0.0, 100.0], ['Z Min', 'Z Max']))
    params.insert(nx.Parameters.Separator("Created Data Objects"))
    params.insert(nx.DataGroupCreationParameter(CliReaderFilter.OUTPUT_EDGE_GEOM_PATH, 'Output Edge Geometry', 'The path to the newly created edge geometry.', nx.DataPath("[Edge Geometry]")))
    params.insert(nx.DataObjectNameParameter(CliReaderFilter.OUTPUT_VERTEX_ATTRMAT_NAME, 'Output Vertex Attribute Matrix Name', 'The name of the newly created vertex attribute matrix.', 'Vertex Data'))
    params.insert(nx.DataObjectNameParameter(CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME, 'Output Edge Attribute Matrix Name', 'The name of the newly created edge attribute matrix.', 'Edge Data'))
    params.insert(nx.DataObjectNameParameter(CliReaderFilter.OUTPUT_FEATURE_ATTRMAT_NAME, 'Output Feature Attribute Matrix Name', 'The name of the newly created feature attribute matrix.', 'Feature Data'))
    params.insert(nx.DataObjectNameParameter(CliReaderFilter.SHARED_VERTICES_ARRAY_NAME, 'Shared Vertices Array Name', 'The name of the newly created shared vertices array.', 'Shared Vertices'))
    params.insert(nx.DataObjectNameParameter(CliReaderFilter.SHARED_EDGES_ARRAY_NAME, 'Shared Edges Array Name', 'The name of the newly created shared edges array.', 'Shared Edges'))

    params.link_parameters(CliReaderFilter.MASK_X_DIMENSION, CliReaderFilter.MIN_MAX_X_COORDS, True)
    params.link_parameters(CliReaderFilter.MASK_Y_DIMENSION, CliReaderFilter.MIN_MAX_Y_COORDS, True)
    params.link_parameters(CliReaderFilter.MASK_Z_DIMENSION, CliReaderFilter.MIN_MAX_Z_COORDS, True)

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    cli_file_path: str = args[CliReaderFilter.CLI_FILE_PATH]
    output_edge_geom_path: nx.DataPath = args[CliReaderFilter.OUTPUT_EDGE_GEOM_PATH]
    output_vertex_attrmat_name: str = args[CliReaderFilter.OUTPUT_VERTEX_ATTRMAT_NAME]
    output_edge_attrmat_name: str = args[CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME]
    output_feature_attrmat_name: str = args[CliReaderFilter.OUTPUT_FEATURE_ATTRMAT_NAME]
    shared_vertices_array_name: str = args[CliReaderFilter.SHARED_VERTICES_ARRAY_NAME]
    shared_edges_array_name: str = args[CliReaderFilter.SHARED_EDGES_ARRAY_NAME]
    mask_x_dimension: bool = args[CliReaderFilter.MASK_X_DIMENSION]
    mask_y_dimension: bool = args[CliReaderFilter.MASK_Y_DIMENSION]
    mask_z_dimension: bool = args[CliReaderFilter.MASK_Z_DIMENSION]
    min_max_x_coords: list = args[CliReaderFilter.MIN_MAX_X_COORDS]
    min_max_y_coords: list = args[CliReaderFilter.MIN_MAX_Y_COORDS]
    min_max_z_coords: list = args[CliReaderFilter.MIN_MAX_Z_COORDS]
    
    if mask_x_dimension and min_max_x_coords[0] > min_max_x_coords[1]:
      return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-9100, f"Invalid Bounding Box Mask: The minimum X coordinate ({min_max_x_coords[0]}) is larger than the maximum X coordinate ({min_max_x_coords[1]}).")])
    if mask_y_dimension and min_max_y_coords[0] > min_max_y_coords[1]:
        return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-9101, f"Invalid Bounding Box Mask: The minimum Y coordinate ({min_max_y_coords[0]}) is larger than the maximum Y coordinate ({min_max_y_coords[1]}).")])
    if mask_z_dimension and min_max_z_coords[0] > min_max_z_coords[1]:
        return nx.IFilter.PreflightResult(nx.OutputActions(), [nx.Error(-9102, f"Invalid Bounding Box Mask: The minimum Z coordinate ({min_max_z_coords[0]}) is larger than the maximum Z coordinate ({min_max_z_coords[1]}).")])

    # Here we create the Edge Geometry (and the 2 internal Attribute Matrix to hold vertex and edge data arrays.)
    # Because this is a "reader" type of filter we do not know (at least in this reader implementation)
    # the number of vertices or edges at preflight time. During execute we will need to ensure that
    # everything is sized correctly.
    output_actions = nx.OutputActions()
    output_actions.append_action(nx.CreateEdgeGeometryAction(geometry_path=output_edge_geom_path, num_edges=1, num_vertices=1, vertex_attribute_matrix_name=output_vertex_attrmat_name, edge_attribute_matrix_name=output_edge_attrmat_name, shared_vertices_name=shared_vertices_array_name, shared_edges_name=shared_edges_array_name))

    array_names, num_of_labels = parse_geometry_array_names(Path(cli_file_path))
    # Because extra geometric data is not included in the specification, we are setting 'Layer' array to int32 and all other arrays to float32
    edge_attr_mat_path = output_edge_geom_path.create_child_path(output_edge_attrmat_name)
    for array_name in array_names:
      dtype = nx.DataType.float32
      if array_name == self.LAYER_ARRAY_NAME:
        dtype = nx.DataType.int32
      array_path = edge_attr_mat_path.create_child_path(array_name)
      output_actions.append_action(nx.CreateArrayAction(dtype, [1], [1], array_path))
    
    if num_of_labels > 0:
      array_path = edge_attr_mat_path.create_child_path(self.LABEL_ARRAY_NAME)
      output_actions.append_action(nx.CreateArrayAction(nx.DataType.int32, [1], [1], array_path))

      feature_attr_mat_path = output_edge_geom_path.create_child_path(output_feature_attrmat_name)
      output_actions.append_action(nx.CreateAttributeMatrixAction(feature_attr_mat_path, [num_of_labels]))

      label_array_path = feature_attr_mat_path.create_child_path(self.LABEL_ARRAY_NAME)
      output_actions.append_action(nx.CreateStringArrayAction([num_of_labels], label_array_path))

    return nx.IFilter.PreflightResult(output_actions)

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    cli_file_path: str = args[CliReaderFilter.CLI_FILE_PATH]
    output_edge_geom_path: nx.DataPath = args[CliReaderFilter.OUTPUT_EDGE_GEOM_PATH]
    output_edge_attrmat_name: str = args[CliReaderFilter.OUTPUT_EDGE_ATTRMAT_NAME]
    output_feature_attrmat_name: str = args[CliReaderFilter.OUTPUT_FEATURE_ATTRMAT_NAME]
    mask_x_dimension: bool = args[CliReaderFilter.MASK_X_DIMENSION]
    mask_y_dimension: bool = args[CliReaderFilter.MASK_Y_DIMENSION]
    mask_z_dimension: bool = args[CliReaderFilter.MASK_Z_DIMENSION]
    min_max_x_coords: list = args[CliReaderFilter.MIN_MAX_X_COORDS]
    min_max_y_coords: list = args[CliReaderFilter.MIN_MAX_Y_COORDS]
    min_max_z_coords: list = args[CliReaderFilter.MIN_MAX_Z_COORDS]

    bounding_box_coords = [-sys.float_info.max, sys.float_info.max] * 3
    if mask_x_dimension:
      bounding_box_coords[0:2] = min_max_x_coords
    if mask_y_dimension:
      bounding_box_coords[2:4] = min_max_y_coords
    if mask_z_dimension:
      bounding_box_coords[4:6] = min_max_z_coords

    try:
      result = parse_file(Path(cli_file_path), bounding_box=bounding_box_coords)
      if result.invalid():
        return nx.Result(errors=result.errors)
      layer_features, layer_heights, hatch_labels = result.value
    except Exception as e:
      return nx.Result([nx.Error(-2010, f"An error occurred while parsing the CLI file '{cli_file_path}': {e}")])

    start_vertices = []
    end_vertices = []
    data_arrays = {}
    if len(hatch_labels) > 0:
      data_arrays[self.LABEL_ARRAY_NAME] = []
    num_of_hatches = 0
    for layer_idx in range(len(layer_features)):
      layer = layer_features[layer_idx]
      if not layer:
        message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Skipping layer {layer_idx + 1}/{len(layer_features)}...'))
        continue

      message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Importing layer {layer_idx + 1}/{len(layer_features)}...'))

      for hatch in layer:
        if hatch.n == 0:
          continue
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

    edge_geom: nx.EdgeGeom = data_structure[output_edge_geom_path]
    
    # Tell the Edge Geometry to resize the shared vertex list so that we can 
    # copy in the vertices.
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Saving Vertex List...'))
    vertex_list = [item for pair in zip(start_vertices, end_vertices) for item in pair]
    edge_geom.resize_vertices(len(vertex_list))

    vertices_array = edge_geom.vertices
    vertices_view = vertices_array.store.npview()
    vertices_view[:] = vertex_list

    # Tell the Edge Geometry to resize the shared edge list so that we can
    # copy in the edge list and also copy in all the edge arrays
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Saving Edges...'))
    edge_geom.resize_edges(num_of_hatches)
    edges_array = edge_geom.edges
    edges_view = edges_array.store.npview()
    edges_view[:] = [[i, i+1] for i in range(0, len(vertex_list), 2)]

    # Get the nx.DataPath to the Edge Attribute Matrix
    edge_attr_mat_path = output_edge_geom_path.create_child_path(output_edge_attrmat_name)
    # Copy the all the edge data into the edge attribute matrix
    for array_name, values in data_arrays.items():
      message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f"Saving Cell Array '{array_name}'..."))
      array_path = edge_attr_mat_path.create_child_path(array_name)
      array: nx.IDataArray = data_structure[array_path]
      values_arr = np.array(values)
      values_arr = values_arr.reshape([len(values)] + array.cdims)
      array_view = array.store.npview()
      array_view[:] = values_arr
    

    # Save the feature level data
    feature_attr_mat_path = output_edge_geom_path.create_child_path(output_feature_attrmat_name)
    label_feature_array_path = feature_attr_mat_path.create_child_path(self.LABEL_ARRAY_NAME)
    label_feature_array: nx.StringArray = data_structure[label_feature_array_path]
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f"Saving Feature Array '{self.LABEL_ARRAY_NAME}'..."))
    label_feature_array.initialize_with_list(list(hatch_labels.values()))

    # Filter is complete, return the results.
    return nx.Result()