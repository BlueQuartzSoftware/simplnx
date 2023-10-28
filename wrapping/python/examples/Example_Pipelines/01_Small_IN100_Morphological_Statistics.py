import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")
    
#Filter 2

result = cx.FindFeatureCentroidsFilter.execute(
    data_structure=data_structure,
    centroids_array_path=("Centroids"),
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    selected_image_geometry=cx.DataPath("DataContainer"),
)
#Filter 3

result = cx.CalculateFeatureSizesFilter.execute(
    data_structure=data_structure,
    equivalent_diameters_path=("EquivalentDiameters"),
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    num_elements_path=("NumElements"),
    save_element_sizes=False,
    volumes_path=("Size Volumes"),
)
#Filter 4

result = cxor.FindShapesFilter.execute(
    data_structure=data_structure,
    aspect_ratios_array_name=("AspectRatios"),
    axis_euler_angles_array_name=("AxisEulerAngles"),
    axis_lengths_array_name=("AxisLengths"),
    centroids_array_path=cx.DataPath("DataContainer/CellFeatureData/Centroids"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    omega3s_array_name=("Omega3s"),
    selected_image_geometry=cx.DataPath("DataContainer"),
    volumes_array_name=("Shape Volumes"),
)
#Filter 5

result = cx.FindNeighbors.execute(
    data_structure=data_structure,
    #boundary_cells: str = ...,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list=("NeighborList"),
    number_of_neighbors=("NumNeighbors"),
    shared_surface_area_list=("SharedSurfaceAreaList"),
    store_boundary_cells=False,
    store_surface_features=False,
    #surface_features: str = ...
)
#Filter 6

result = cx.FindNeighborhoodsFilter.execute(
    data_structure=data_structure,
    centroids_array_path=cx.DataPath("DataContainer/CellFeatureData/Centroids"),
    equivalent_diameters_array_path=cx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    multiples_of_average=1.0,
    neighborhood_list_array_name=("NeighborhoodList"),
    neighborhoods_array_name=("Neighborhoods"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
)
#Filter 7

result = cx.FindEuclideanDistMapFilter.execute(
    data_structure=data_structure,
    calc_manhattan_dist=True,
    do_boundaries=True,
    do_quad_points=True,
    do_triple_lines=True,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    g_bdistances_array_name=("GBManhattanDistances"),
    #nearest_neighbors_array_name: str = ...,
    q_pdistances_array_name=("QPManhattanDistances"),
    save_nearest_neighbors=False,
    selected_image_geometry=cx.DataPath("DataContainer"),
    t_jdistances_array_name=("TJManhattanDistances"),
)
#Filter 8

result = cx.FindSurfaceAreaToVolumeFilter.execute(
    data_structure=data_structure,
    calculate_sphericity=True,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    num_cells_array_path=cx.DataPath("DataContainer/CellFeatureData/NumElements"),
    selected_image_geometry=cx.DataPath("DataContainer"),
    sphericity_array_name=("Sphericity"),
    surface_area_volume_ratio_array_name=("SurfaceAreaVolumeRatio"),
)
#Filter 9

output_file_path = "Data/Output/Statistics/SmallIN100_Morph.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")