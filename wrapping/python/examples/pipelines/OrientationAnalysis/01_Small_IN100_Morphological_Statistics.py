import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = str(nxtest.get_data_directory() / "Output/Reconstruction/SmallIN100_Final.dream3d")
import_data.data_paths = None

# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        import_file_data=import_data
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.DeleteData()
# Execute Filter With Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    removed_data_path=[nx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
                       nx.DataPath("DataContainer/CellFeatureData/NumElements"),
                       nx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
                       nx.DataPath("DataContainer/CellFeatureData/NumNeighbors2")]
)
nxtest.check_filter_result(nx_filter, result)
    
# Filter 3
# Instantiate Filter
nx_filter = nx.FindFeatureCentroidsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    centroids_array_path=("Centroids"),
    feature_attribute_matrix=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    selected_image_geometry=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 4
# Instantiate Filter
nx_filter = nx.CalculateFeatureSizesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    equivalent_diameters_path=("EquivalentDiameters"),
    feature_attribute_matrix=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=nx.DataPath("DataContainer"),
    num_elements_path=("NumElements"),
    save_element_sizes=False,
    volumes_path=("Size Volumes")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 5
# Instantiate Filter
nx_filter = cxor.FindShapesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    aspect_ratios_array_name=("AspectRatios"),
    axis_euler_angles_array_name=("AxisEulerAngles"),
    axis_lengths_array_name=("AxisLengths"),
    centroids_array_path=nx.DataPath("DataContainer/CellFeatureData/Centroids"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    omega3s_array_name=("Omega3s"),
    selected_image_geometry=nx.DataPath("DataContainer"),
    volumes_array_name=("Shape Volumes")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 6
# Instantiate Filter
nx_filter = nx.FindNeighbors()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    # Boundary cells parameter is not currently part of the code
    # boundary_cells: str = ...,
    cell_feature_arrays=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=nx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=nx.DataPath("DataContainer"),
    neighbor_list=("NeighborList"),
    number_of_neighbors=("NumNeighbors"),
    shared_surface_area_list=("SharedSurfaceAreaList"),
    store_boundary_cells=False,
    # Surface features parameter is not currently part of the code
    # surface_features: str = ...,
    store_surface_features=False
)
nxtest.check_filter_result(nx_filter, result)



# Filter 7
# Instantiate Filter
nx_filter = nx.FindNeighborhoodsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    centroids_array_path=nx.DataPath("DataContainer/CellFeatureData/Centroids"),
    equivalent_diameters_array_path=nx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    multiples_of_average=1.0,
    neighborhood_list_array_name=("NeighborhoodList"),
    neighborhoods_array_name=("Neighborhoods"),
    selected_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 8
# Instantiate Filter
nx_filter = nx.FindEuclideanDistMapFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    calc_manhattan_dist=True,
    do_boundaries=True,
    do_quad_points=True,
    do_triple_lines=True,
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    g_bdistances_array_name=("GBManhattanDistances"),
    # Nearest neighbors array name is not currently part of the code
    # nearest_neighbors_array_name: str = ...,
    q_pdistances_array_name=("QPManhattanDistances"),
    save_nearest_neighbors=False,
    selected_image_geometry=nx.DataPath("DataContainer"),
    t_jdistances_array_name=("TJManhattanDistances")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 9
# Instantiate Filter
nx_filter = nx.FindSurfaceAreaToVolumeFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    calculate_sphericity=True,
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    num_cells_array_path=nx.DataPath("DataContainer/CellFeatureData/NumElements"),
    selected_image_geometry=nx.DataPath("DataContainer"),
    sphericity_array_name=("Sphericity"),
    surface_area_volume_ratio_array_name=("SurfaceAreaVolumeRatio")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 10
# Set Output File Path
output_file_path = nxtest.get_data_directory() / "Output/Statistics/SmallIN100_Morph.dream3d"
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(import_data.file_path)
# *****************************************************************************


print("===> Pipeline Complete")
