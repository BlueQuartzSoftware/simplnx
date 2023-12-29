import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Import Data
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        import_file_data=import_data
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.DeleteData()
# Execute Filter With Parameters
result = filter.execute(
    data_structure=data_structure,
    removed_data_path=[nx.DataPath("DataContainer/CellFeatureData/Centroids"),
                       nx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
                       nx.DataPath("DataContainer/CellFeatureData/NumElements"),
                       nx.DataPath("DataContainer/CellFeatureData/Omega3s"),
                       nx.DataPath("DataContainer/CellFeatureData/AxisLengths"),
                       nx.DataPath("DataContainer/CellFeatureData/AxisEulerAngles"),
                       nx.DataPath("DataContainer/CellFeatureData/AspectRatios"),
                       nx.DataPath("DataContainer/CellFeatureData/Shape Volumes"),
                       nx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
                       nx.DataPath("DataContainer/CellFeatureData/NeighborList"),
                       nx.DataPath("DataContainer/CellFeatureData/SharedSurfaceAreaList"),
                       nx.DataPath("DataContainer/CellFeatureData/NeighborhoodList"),
                       nx.DataPath("DataContainer/CellFeatureData/SurfaceAreaVolumeRatio"),
                       nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
                       nx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles")]
)
nxtest.check_filter_result(filter, result)


# Filter 2
# Instantiate Filter
filter = nx.FindNeighbors()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #boundary_cells: str = ...,
    cell_feature_arrays=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=nx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=nx.DataPath("DataContainer"),
    neighbor_list="NeighborList",
    number_of_neighbors="NumNeighbors",
    shared_surface_area_list="SharedSurfaceAreaList",
    store_boundary_cells=False,
    store_surface_features=False
    #surface_features: str = ...
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.FindFeatureCentroidsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_path="Centroids",
    feature_attribute_matrix=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    selected_image_geometry=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(filter, result)

# Filter 4
# Instantiate Filter
filter = nx.FindSurfaceFeatures()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_attribute_matrix_path=nx.DataPath("DataContainer/CellFeatureData"),
    feature_geometry_path=nx.DataPath("DataContainer"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    mark_feature_0_neighbors=True,
    surface_features_array_path="SurfaceFeatures"
)
nxtest.check_filter_result(filter, result)

# Filter 5
# Instantiate Filter
filter = nx.FindBiasedFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    biased_features_array_name="BiasedFeatures",
    calc_by_phase=True,
    centroids_array_path=nx.DataPath("DataContainer/CellFeatureData/Centroids"),
    image_geometry_path=nx.DataPath("DataContainer"),
    phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    surface_features_array_path=nx.DataPath("DataContainer/CellFeatureData/SurfaceFeatures")
)
nxtest.check_filter_result(filter, result)

# Filter 6
# Output file path for Filter 4
output_file_path = nxtest.GetDataDirectory() + "/Output/Examples/FindBiasedFeatures.dream3d"
# Instantiate Filter
filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")