import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = cx.ImportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        import_file_data=import_data
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.FindNeighbors()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #boundary_cells: str = ...,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list="NeighborList",
    number_of_neighbors="NumNeighbors",
    shared_surface_area_list="SharedSurfaceAreaList",
    store_boundary_cells=False,
    store_surface_features=False
    #surface_features: str = ...
)
if len(result.warnings) !=0:
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
    centroids_array_path="Centroids",
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    selected_image_geometry=cx.DataPath("DataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.FindSurfaceFeatures()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_attribute_matrix_path=cx.DataPath("DataContainer/CellFeatureData"),
    feature_geometry_path=cx.DataPath("DataContainer"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    mark_feature_0_neighbors=True,
    surface_features_array_path="SurfaceFeatures"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cx.FindBiasedFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    biased_features_array_name="BiasedFeatures",
    calc_by_phase=True,
    centroids_array_path=cx.DataPath("DataContainer/CellFeatureData/Centroids"),
    image_geometry_path=cx.DataPath("DataContainer"),
    phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    surface_features_array_path=cx.DataPath("DataContainer/CellFeatureData/SurfaceFeatures")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Output file path for Filter 4
output_file_path = "Data/Output/Examples/FindBiasedFeatures.dream3d"
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")