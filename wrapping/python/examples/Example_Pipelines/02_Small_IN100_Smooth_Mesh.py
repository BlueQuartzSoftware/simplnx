import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/SurfaceMesh/SmallIN100_Mesh.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#Filter 2

result = cx.LaplacianSmoothingFilter.execute(
    data_structure=data_structure,
    iteration_steps=100,
    lambda_value=0.25,
    mu_factor=0.2,
    quad_point_lambda=0.15000001,
    surface_mesh_node_type_array_path=cx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    surface_point_lambda=0.0,
    surface_quad_point_lambda=0.0,
    surface_triple_line_lambda=0.0,
    triangle_geometry_data_path=cx.DataPath("TriangleDataContainer"),
    triple_line_lambda=0.2,
    use_taubin_smoothing=True,
)
#Filter 3

output_file_path = "Data/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")