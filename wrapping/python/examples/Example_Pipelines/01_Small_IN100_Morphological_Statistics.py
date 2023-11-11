import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None

# Instantiate Filter
filter = cx.ImportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.DeleteData()
# Execute Filter With Parameters
result = filter.execute(
    data_structure=data_structure,
    removed_data_path=[cx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
                       cx.DataPath("DataContainer/CellFeatureData/NumElements"),
                       cx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
                       cx.DataPath("DataContainer/CellFeatureData/NumNeighbors2")]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")
    
# Filter 3
# Instantiate Filter
filter = cx.FindFeatureCentroidsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_path=("Centroids"),
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    selected_image_geometry=cx.DataPath("DataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 4
# Instantiate Filter
filter = cx.CalculateFeatureSizesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    equivalent_diameters_path=("EquivalentDiameters"),
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    num_elements_path=("NumElements"),
    save_element_sizes=False,
    volumes_path=("Size Volumes")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 5
# Instantiate Filter
filter = cxor.FindShapesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    aspect_ratios_array_name=("AspectRatios"),
    axis_euler_angles_array_name=("AxisEulerAngles"),
    axis_lengths_array_name=("AxisLengths"),
    centroids_array_path=cx.DataPath("DataContainer/CellFeatureData/Centroids"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    omega3s_array_name=("Omega3s"),
    selected_image_geometry=cx.DataPath("DataContainer"),
    volumes_array_name=("Shape Volumes")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 6
# Instantiate Filter
filter = cx.FindNeighbors()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    # Boundary cells parameter is not currently part of the code
    # boundary_cells: str = ...,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list=("NeighborList"),
    number_of_neighbors=("NumNeighbors"),
    shared_surface_area_list=("SharedSurfaceAreaList"),
    store_boundary_cells=False,
    # Surface features parameter is not currently part of the code
    # surface_features: str = ...,
    store_surface_features=False
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")



# Filter 7
# Instantiate Filter
filter = cx.FindNeighborhoodsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_path=cx.DataPath("DataContainer/CellFeatureData/Centroids"),
    equivalent_diameters_array_path=cx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    multiples_of_average=1.0,
    neighborhood_list_array_name=("NeighborhoodList"),
    neighborhoods_array_name=("Neighborhoods"),
    selected_image_geometry_path=cx.DataPath("DataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 8
# Instantiate Filter
filter = cx.FindEuclideanDistMapFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calc_manhattan_dist=True,
    do_boundaries=True,
    do_quad_points=True,
    do_triple_lines=True,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    g_bdistances_array_name=("GBManhattanDistances"),
    # Nearest neighbors array name is not currently part of the code
    # nearest_neighbors_array_name: str = ...,
    q_pdistances_array_name=("QPManhattanDistances"),
    save_nearest_neighbors=False,
    selected_image_geometry=cx.DataPath("DataContainer"),
    t_jdistances_array_name=("TJManhattanDistances")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 9
# Instantiate Filter
filter = cx.FindSurfaceAreaToVolumeFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculate_sphericity=True,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    num_cells_array_path=cx.DataPath("DataContainer/CellFeatureData/NumElements"),
    selected_image_geometry=cx.DataPath("DataContainer"),
    sphericity_array_name=("Sphericity"),
    surface_area_volume_ratio_array_name=("SurfaceAreaVolumeRatio")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 10
# Set Output File Path
output_file_path = "Data/Output/Statistics/SmallIN100_Morph.dream3d"
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")