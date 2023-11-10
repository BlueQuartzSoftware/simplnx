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
                        import_file_data=import_data)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.FindFeatureCentroidsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_path="Centroids",
    feature_attribute_matrix=cx.DataPath(["DataContainer/CellFeatureData"]),
    feature_ids_path=cx.DataPath(["DataContainer/CellData/FeatureIds"]),
    selected_image_geometry=cx.DataPath(["DataContainer"])
)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.FindBiasedFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    biased_features_array_name="BiasedFeatures",
    calc_by_phase=True,
    centroids_array_path=cx.DataPath(["DataContainer/CellFeatureData/Centroids"]),
    image_geometry_path=cx.DataPath(["DataContainer"]),
    phases_array_path=cx.DataPath(["DataContainer/CellFeatureData/Phases"]),
    surface_features_array_path=cx.DataPath(["DataContainer/CellFeatureData/SurfaceFeatures"])
)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Output file path for Filter 4
output_file_path = "Data/Output/Examples/FindBiasedFeatures.dream3d"
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")