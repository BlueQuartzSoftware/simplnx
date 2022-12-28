/**
 * This file is auto generated from the original Reconstruction/MergeTwinsFilter
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[MergeTwinsFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/FindAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/MergeTwinsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "complex_plugins/EbsdLibConstants.hpp"
#include "complex_plugins/Utilities/TestUtilities.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("Reconstruction::MergeTwinsFilter: Instantiation and Parameter Check", "[Reconstruction][MergeTwinsFilter][.][UNIMPLEMENTED][!mayfail]")
{
  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Make sure we can load the needed filters from the plugins

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;

  const std::string k_FeatureAttributeMatrix("CellFeatureData");
  const std::string k_AvgQuatsName("AvgQuats");
  const std::string k_NumNeighborsName("NumNeighbors2");
  const std::string k_NeighborListName("NeighborList2");
  const std::string k_SharedSurfaceAreaListName("SharedSurfaceAreaList2");

  const std::string k_CellParentIdsName("ParentIds");
  const std::string k_NewFeatureAttributeMatrixName("NewCellFeatureData");
  const std::string k_FeatureParentIdsName("ParentIds");
  const std::string k_NewFeatureActiveName("Active");

  const DataPath k_FeatureIdsPath = k_CellAttributeMatrix.createChildPath(k_FeatureIds);
  const DataPath k_CellParentIdsPath = k_CellAttributeMatrix.createChildPath(k_CellParentIdsName);

  const DataPath k_CellFeauteAttributeMatrix = k_DataContainerPath.createChildPath(k_FeatureAttributeMatrix);
  const DataPath k_CellFeaturePhasesPath = k_CellFeauteAttributeMatrix.createChildPath(k_Phases);
  const DataPath k_AvgQuatsPath = k_CellFeauteAttributeMatrix.createChildPath(k_AvgQuatsName);
  const DataPath k_CellFeatureEulerAnglesPath = k_CellFeauteAttributeMatrix.createChildPath(EbsdLib::CellData::EulerAngles);
  const DataPath k_NumNeighborsPath = k_CellFeauteAttributeMatrix.createChildPath(k_NumNeighborsName);
  const DataPath k_NeighborListPath = k_CellFeauteAttributeMatrix.createChildPath(k_NeighborListName);
  const DataPath k_SharedSurfaceAreaListPath = k_CellFeauteAttributeMatrix.createChildPath(k_SharedSurfaceAreaListName);
  const DataPath k_FeatureParentIdsPath = k_CellFeauteAttributeMatrix.createChildPath(k_FeatureParentIdsName);

  const DataPath k_NewFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_NewFeatureAttributeMatrixName);
  const DataPath k_NewFeatureActivePath = k_NewFeatureAttributeMatrix.createChildPath(k_NewFeatureActiveName);

  DataStructure dataStructure;

  // Read input data
  {
    constexpr StringLiteral k_ImportFileData = "Import_File_Data";

    auto filter = filterList->createFilter(k_ImportDream3dFilterHandle);
    REQUIRE(nullptr != filter);

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/TestFiles/neighbor_orientation_correlation.dream3d", unit_test::k_DREAM3DDataDir));

    Arguments args;
    args.insertOrAssign(k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // SegmentFeatures (Misorientation) filter
  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseGoodVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_GoodVoxelsPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_FeatureAttributeMatrix));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // FindFeaturePhases filter
  {
    auto filter = filterList->createFilter(k_FindNFeaturePhasesFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;

    constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
    constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
    constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";

    args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeaturePhasesPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // FindFeatureAverageOrientation filter
  {
    FindAvgOrientationsFilter filter;
    Arguments args;

    args.insertOrAssign(FindAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(FindAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(FindAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(FindAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(FindAvgOrientationsFilter::k_CellFeatureAttributeMatrix_Key, std::make_any<DataPath>(k_CellFeauteAttributeMatrix));
    args.insertOrAssign(FindAvgOrientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<std::string>(k_AvgQuatsName));
    args.insertOrAssign(FindAvgOrientationsFilter::k_AvgEulerAnglesArrayPath_Key, std::make_any<std::string>(EbsdLib::CellData::EulerAngles));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // FindNeighborList Filter
  {
    auto filter = filterList->createFilter(k_FindNeighborsFilterHandle);
    Arguments args;

    constexpr StringLiteral k_StoreBoundary_Key = "store_boundary_cells";
    constexpr StringLiteral k_StoreSurface_Key = "store_surface_features";
    constexpr StringLiteral k_ImageGeom_Key = "image_geometry";
    constexpr StringLiteral k_FeatureIds_Key = "feature_ids";
    constexpr StringLiteral k_CellFeatures_Key = "cell_feature_arrays";
    constexpr StringLiteral k_BoundaryCells_Key = "boundary_cells";
    constexpr StringLiteral k_NumNeighbors_Key = "number_of_neighbors";
    constexpr StringLiteral k_NeighborList_Key = "neighbor_list";
    constexpr StringLiteral k_SharedSurfaceArea_Key = "shared_surface_area_list";
    constexpr StringLiteral k_SurfaceFeatures_Key = "surface_features";

    DataPath boundaryCells;
    DataPath surfaceFeatures;

    args.insert(k_StoreBoundary_Key, std::make_any<bool>(false));
    args.insert(k_StoreSurface_Key, std::make_any<bool>(false));
    args.insert(k_ImageGeom_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insert(k_CellFeatures_Key, std::make_any<DataPath>(k_CellFeauteAttributeMatrix));
    args.insert(k_BoundaryCells_Key, std::make_any<std::string>("BoundaryCells"));
    args.insert(k_NumNeighbors_Key, std::make_any<std::string>(k_NumNeighborsName));
    args.insert(k_NeighborList_Key, std::make_any<std::string>(k_NeighborListName));
    args.insert(k_SharedSurfaceArea_Key, std::make_any<std::string>(k_SharedSurfaceAreaListName));
    args.insert(k_SurfaceFeatures_Key, std::make_any<std::string>("SurfaceFeatures"));

    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);
  }

  // merge twins filter
  {
    MergeTwinsFilter filter;
    Arguments args;

    DataPath nonContiguousNeighborList;

    // Create default Parameters for the filter.
    args.insertOrAssign(MergeTwinsFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
    args.insertOrAssign(MergeTwinsFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(nonContiguousNeighborList));
    args.insertOrAssign(MergeTwinsFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_NeighborListPath));
    args.insertOrAssign(MergeTwinsFilter::k_AxisTolerance_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(MergeTwinsFilter::k_AngleTolerance_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(MergeTwinsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeaturePhasesPath));
    args.insertOrAssign(MergeTwinsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_AvgQuatsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(MergeTwinsFilter::k_CellParentIdsArrayName_Key, std::make_any<std::string>(k_CellParentIdsName));
    args.insertOrAssign(MergeTwinsFilter::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_NewFeatureAttributeMatrixName));
    args.insertOrAssign(MergeTwinsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<std::string>(k_FeatureParentIdsName));
    args.insertOrAssign(MergeTwinsFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_NewFeatureActiveName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // check results
  {
    Int32Array* cellParentIdsArray = dataStructure.getDataAs<Int32Array>(k_CellParentIdsPath);
    REQUIRE(cellParentIdsArray != nullptr);
    auto cellParentIdsTupleShape = cellParentIdsArray->getIDataStore()->getTupleShape();
    auto cellParentIdsNumComps = cellParentIdsArray->getNumberOfComponents();
    REQUIRE(cellParentIdsTupleShape.size() == 3);
    REQUIRE(cellParentIdsTupleShape[0] == 117);
    REQUIRE(cellParentIdsTupleShape[1] == 201);
    REQUIRE(cellParentIdsTupleShape[2] == 189);
    REQUIRE(cellParentIdsNumComps == 1);

    Int32Array* featureParentIdsArray = dataStructure.getDataAs<Int32Array>(k_FeatureParentIdsPath);
    REQUIRE(featureParentIdsArray != nullptr);
    auto featureParentIdsTupleShape = featureParentIdsArray->getIDataStore()->getTupleShape();
    auto featureParentIdsNumComps = featureParentIdsArray->getNumberOfComponents();
    REQUIRE(featureParentIdsTupleShape.size() == 1);
    REQUIRE(featureParentIdsTupleShape[0] == 5416);
    REQUIRE(featureParentIdsNumComps == 1);

    BoolArray* activeArray = dataStructure.getDataAs<BoolArray>(k_NewFeatureActivePath);
    REQUIRE(activeArray != nullptr);
    auto activeTupleShape = activeArray->getIDataStore()->getTupleShape();
    auto activeNumComps = activeArray->getNumberOfComponents();
    REQUIRE(activeTupleShape.size() == 1);
    REQUIRE(activeTupleShape[0] == 4507);
    REQUIRE(activeNumComps == 1);
  }
}
