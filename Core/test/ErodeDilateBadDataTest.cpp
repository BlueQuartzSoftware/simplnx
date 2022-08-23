#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "complex_plugins/Utilities/SmallIN100Utilties.hpp"
#include "complex_plugins/Utilities/TestUtilities.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/ErodeDilateBadDataFilter.hpp"

/**
 * @brief Test Setup
 *
 * Read SmallIN100.dream3d
 * Crop Image Geometry (X: 0-max, Y: 0-max. Z:0-1)
 * Threshold Objects (CI>01., IQ>120)
 * Convert Orientations Representation (Eulers->Quaternions)
 * Identify Sample
 * EbsdSegmentFeatures
 * ErodeDilateBadData
 */

using namespace complex;

TEST_CASE("Core::ErodeDilateBadDataFilter: Small IN100 Pipeline", "[Core][ErodeDilateBadDataFilter]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/TestFiles/6_6_erode_dilate_bad_data.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure exemplarDataStructure = complex::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/Small_IN100.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  const std::string k_OrigDataContainer("Original DataContainer");
  const DataPath k_OrigDataContainerPath({k_OrigDataContainer});
  // Rename Data Object
  {
    auto filter = filterList->createFilter(k_RenameDataObjectFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys
    constexpr StringLiteral k_DataObject_Key = "data_object";
    constexpr StringLiteral k_NewName_Key = "new_name";

    Arguments args;
    args.insertOrAssign(k_DataObject_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(k_NewName_Key, std::make_any<std::string>(k_OrigDataContainer));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Crop Geometry (Image)
  {
    const DataPath k_OrigCellAttributeMatrix = k_OrigDataContainerPath.createChildPath("CellData");

    const DataPath k_OrigConfidenceIndexArrayPath = k_OrigCellAttributeMatrix.createChildPath(k_ConfidenceIndex);
    const DataPath k_OrigEulersArrayPath = k_OrigCellAttributeMatrix.createChildPath("EulerAngles");
    const DataPath k_OrigImageQualityArrayPath = k_OrigCellAttributeMatrix.createChildPath(k_ImageQuality);
    const DataPath k_OrigPhasesArrayPath = k_OrigCellAttributeMatrix.createChildPath(k_Phases);

    auto filter = filterList->createFilter(k_CropImageGeometryFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys
    constexpr StringLiteral k_MinVoxel_Key = "min_voxel";
    constexpr StringLiteral k_MaxVoxel_Key = "max_voxel";
    constexpr StringLiteral k_UpdateOrigin_Key = "update_origin";
    constexpr StringLiteral k_ImageGeom_Key = "image_geom";
    constexpr StringLiteral k_NewImageGeom_Key = "new_image_geom";
    constexpr StringLiteral k_VoxelArrays_Key = "voxel_arrays";
    constexpr StringLiteral k_RenumberFeatures_Key = "renumber_features";
    //    constexpr StringLiteral k_FeatureIds_Key = "feature_ids";
    constexpr StringLiteral k_CreatedCellData_Key = "new_features_group_name";

    Arguments args;
    // Create default Parameters for the filter.
    VectorUInt64Parameter::ValueType minValues = {0, 0, 116};
    VectorUInt64Parameter::ValueType maxValues = {188, 200, 116};

    args.insertOrAssign(k_ImageGeom_Key, std::make_any<DataPath>(k_OrigDataContainerPath));
    args.insertOrAssign(k_NewImageGeom_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(k_MinVoxel_Key, std::make_any<VectorUInt64Parameter::ValueType>(minValues));
    args.insertOrAssign(k_MaxVoxel_Key, std::make_any<VectorUInt64Parameter::ValueType>(maxValues));
    args.insertOrAssign(k_UpdateOrigin_Key, std::make_any<BoolParameter::ValueType>(false));
    args.insertOrAssign(k_VoxelArrays_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>({k_OrigConfidenceIndexArrayPath, k_OrigEulersArrayPath, k_OrigImageQualityArrayPath, k_OrigPhasesArrayPath}));
    args.insertOrAssign(k_RenumberFeatures_Key, std::make_any<BoolParameter::ValueType>(false));
    args.insertOrAssign(k_CreatedCellData_Key, std::make_any<std::string>(k_CellData));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Move the Ensemble AttributeMatrix from the original Image Geometry to the cropped Image Geometry
  auto origDataContainerId = dataStructure.getId(k_OrigDataContainerPath).value();
  auto ensembleGroupId = dataStructure.getId(k_OrigDataContainerPath.createChildPath(k_EnsembleAttributeMatrix)).value();
  auto dataContainerId = dataStructure.getId(k_DataContainerPath).value();

  dataStructure.setAdditionalParent(ensembleGroupId, dataContainerId);
  dataStructure.removeParent(ensembleGroupId, origDataContainerId);

  // Write the DataStructure to an output file
  WriteTestDataStructure(dataStructure, fmt::format("{}/Test_Output/erode_dilate_bad_data.dream3d", unit_test::k_BinaryTestOutputDir));

  // Delete Data

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Identify Sample Filter
  SmallIn100::ExecuteIdentifySample(dataStructure, *filterList);

  // Segment Features Misorientation
  SmallIn100::ExecuteEbsdSegmentFeatures(dataStructure, *filterList);

  // ErodeDilateBadDataFilter
  {
    ErodeDilateBadDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(1));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedFeatureDataGroup_Key, std::make_any<DataPath>(k_CellFeatureAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer);
}
