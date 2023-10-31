import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
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
    centroids_array_path="Centroids",
    feature_attribute_matrix=cx.DataPath(["DataContainer/CellFeatureData"]),
    feature_ids_path=cx.DataPath(["DataContainer/CellData/FeatureIds"]),
    selected_image_geometry=cx.DataPath(["DataContainer"])
)

#Filter 3

result = cx.FindBiasedFeaturesFilter.execute(
    data_structure=data_structure,
    biased_features_array_name="BiasedFeatures",
    calc_by_phase=True,
    centroids_array_path=cx.DataPath(["DataContainer/CellFeatureData/Centroids"]),
    image_geometry_path=cx.DataPath(["DataContainer"]),
    phases_array_path=cx.DataPath(["DataContainer/CellFeatureData/Phases"]),
    surface_features_array_path=cx.DataPath(["DataContainer/CellFeatureData/SurfaceFeatures"])
)

#Filter 4

output_file_path = "Data/Output/Examples/FindBiasedFeatures.dream3d",
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")
