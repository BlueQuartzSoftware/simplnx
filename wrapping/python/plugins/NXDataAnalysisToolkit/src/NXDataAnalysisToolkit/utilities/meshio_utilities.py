import simplnx as nx
import meshio
import numpy as np
from typing import List
from pathlib import Path

def get_geometry_cell_tuple_dimensions(geom: nx.INodeGeometry0D) -> list:
    if geom.type == nx.DataObject.DataObjectType.EdgeGeom:
        return geom.edges.tdims
    if geom.type == nx.DataObject.DataObjectType.TriangleGeom or geom.type == nx.DataObject.DataObjectType.QuadGeom:
        return geom.faces.tdims
    if geom.type == nx.DataObject.DataObjectType.TetrahedralGeom or geom.type == nx.DataObject.DataObjectType.HexahedralGeom:
        return geom.polyhedra.tdims

def create_meshio_cells_list(geom: nx.INodeGeometry0D) -> list:
    # Meshio requires the cells to be either int32 or int64, so use int64
    if geom.type == nx.DataObject.DataObjectType.EdgeGeom:
        cells_list = geom.edges.npview().astype(np.int64)
        cells = [('line', cells_list)]
    elif geom.type == nx.DataObject.DataObjectType.TriangleGeom:
        cells_list = geom.faces.npview().astype(np.int64)
        cells = [('triangle', cells_list)]
    elif geom.type == nx.DataObject.DataObjectType.QuadGeom:
        cells_list = geom.faces.npview().astype(np.int64)
        cells = [('quad', cells_list)]
    elif geom.type == nx.DataObject.DataObjectType.TetrahedralGeom:
        cells_list = geom.polyhedra.npview().astype(np.int64)
        cells = [('tetra', cells_list)]
    else: # Hexhedral geometry
        cells_list = geom.polyhedra.npview().astype(np.int64)
        cells = [('hexahedron', cells_list)]

    return cells

def preflight_meshio_writer_filter(data_structure: nx.DataStructure, input_geometry_path: nx.DataPath, cell_data_array_paths: List[nx.DataPath] = None, point_data_array_paths: List[nx.DataPath] = None) -> nx.IFilter.PreflightResult:
    geom = data_structure[input_geometry_path]
    geom_cell_tdims = get_geometry_cell_tuple_dimensions(geom)

    warnings: List[nx.Warning] = []

    if cell_data_array_paths is not None:
        for data_array_path in cell_data_array_paths:
            data_array: nx.IDataArray = data_structure[data_array_path]
            if data_array.tdims != geom_cell_tdims:
                warnings.append(nx.Warning(-3010, f"Cell data array '{data_array_path}' has tuple dimensions {data_array.tdims} but the input geometry requires tuple dimensions {geom_cell_tdims}.  This MAY still work if the pipeline does not know the actual number of cells yet.  This can happen if the geometry was created earlier in the pipeline."))

    if point_data_array_paths is not None:
        for data_array_path in point_data_array_paths:
            data_array: nx.IDataArray = data_structure[data_array_path]
            if data_array.tdims != geom.vertices.tdims:
                warnings.append(nx.Warning(-3011, f"Point data array '{data_array_path}' has tuple dimensions {data_array.tdims} but the input geometry point data requires tuple dimensions {geom.vertices.tdims}.  This MAY still work if the pipeline does not know the actual number of vertices yet.  This can happen if the geometry was created earlier in the pipeline."))

    return None, warnings

def execute_meshio_writer_filter(file_format: str, data_structure: nx.DataStructure, input_geometry_path: nx.DataPath, cell_data_array_paths: List[nx.DataPath], point_data_array_paths: List[nx.DataPath], output_file_path: Path, remove_array_name_spaces: bool = False, **write_kwargs):
    geom = data_structure[input_geometry_path]
    geom_cell_tdims = get_geometry_cell_tuple_dimensions(geom)

    # Create meshio cells list
    cells = create_meshio_cells_list(geom)

    # Create meshio cell_data
    cell_data = {}
    if cell_data_array_paths is not None:
        for data_array_path in cell_data_array_paths:
            data_array = data_structure[data_array_path]
            if data_array.tdims != geom_cell_tdims:
                return nx.Result(errors=[nx.Error(-4010, f"Cell data array '{data_array_path}' has tuple dimensions {data_array.tdims} but the input geometry requires tuple dimensions {geom_cell_tdims}.")])
            cell_data[data_array.name.replace(' ', '') if remove_array_name_spaces else data_array.name] = [data_array.npview()] # Remove spaces in cell data array names (this is required for some formats like VTK)

    # Create meshio point_data
    point_data = {}
    if point_data_array_paths is not None:
        for data_array_path in point_data_array_paths:
            data_array = data_structure[data_array_path]
            if data_array.tdims != geom.vertices.tdims:
                return nx.Result(errors=[nx.Error(-4011, f"Point data array '{data_array_path}' has tuple dimensions {data_array.tdims} but the input geometry point data requires tuple dimensions {geom.vertices.tdims}.")])
            point_data[data_array.name.replace(' ', '') if remove_array_name_spaces else data_array.name] = [data_array.npview()] # Remove spaces in point data array names (this is required for some formats like VTK)

    # Create meshio Mesh
    mesh = meshio.Mesh(points=list(geom.vertices.npview().astype(np.float64)), cells=cells, cell_data=cell_data, point_data=point_data)

    # Output the mesh
    meshio.write(output_file_path, mesh, file_format=file_format, **write_kwargs)

    return nx.Result()