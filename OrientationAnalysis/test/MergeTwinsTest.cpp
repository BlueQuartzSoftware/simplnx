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
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/FindAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/MergeTwinsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace EbsdLib
{
namespace Ang
{
const std::string ConfidenceIndex("Confidence Index");
const std::string ImageQuality("Image Quality");

} // namespace Ang
namespace CellData
{
inline const std::string EulerAngles("EulerAngles");
inline const std::string Phases("Phases");
} // namespace CellData
namespace EnsembleData
{
inline const std::string CrystalStructures("CrystalStructures");
inline const std::string LatticeConstants("LatticeConstants");
inline const std::string MaterialName("MaterialName");
} // namespace EnsembleData
} // namespace EbsdLib

struct make_shared_enabler : public complex::Application
{
};

TEST_CASE("Reconstruction::MergeTwinsFilter: Instantiation and Parameter Check", "[Reconstruction][MergeTwinsFilter][.][UNIMPLEMENTED][!mayfail]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Make sure we can load the needed filters from the plugins
  const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");

  const Uuid k_ImportDream3dFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
  const FilterHandle k_ImportDream3dFilterHandle(k_ImportDream3dFilterId, k_ComplexCorePluginId);
  const Uuid k_FindFeaturePhasesObjectsId = *Uuid::FromString("6334ce16-cea5-5643-83b5-9573805873fa");
  const FilterHandle k_FindNFeaturePhasesFilterHandle(k_FindFeaturePhasesObjectsId, k_ComplexCorePluginId);
  const Uuid k_FindNeighborsObjectsId = *Uuid::FromString("97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac");
  const FilterHandle k_FindNeighborsFilterHandle(k_FindNeighborsObjectsId, k_ComplexCorePluginId);

  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableData::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};

  const std::string k_Quats("Quats");
  const std::string k_Phases("Phases");
  const std::string k_ConfidenceIndex = EbsdLib::Ang::ConfidenceIndex;
  const std::string k_ImageQuality = EbsdLib::Ang::ImageQuality;
  const std::string k_FeatureIds("FeatureIds");
  const std::string k_FeatureAttributeMatrix("CellFeatureData");
  const std::string k_Active("Active");

  const std::string k_Mask("Mask");
  const std::string k_DataContainer("Exemplar Data");
  const DataPath k_DataContainerPath({k_DataContainer});

  const DataPath k_CalculatedShiftsPath = k_DataContainerPath.createChildPath("Calculated Shifts");

  const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath("CellData");
  const DataPath k_EulersArrayPath = k_CellAttributeMatrix.createChildPath(EbsdLib::CellData::EulerAngles);
  const DataPath k_QuatsArrayPath = k_CellAttributeMatrix.createChildPath(k_Quats);
  const DataPath k_PhasesArrayPath = k_CellAttributeMatrix.createChildPath(k_Phases);
  const DataPath k_ConfidenceIndexArrayPath = k_CellAttributeMatrix.createChildPath(k_ConfidenceIndex);
  const DataPath k_ImageQualityArrayPath = k_CellAttributeMatrix.createChildPath(k_ImageQuality);
  const DataPath k_MaskArrayPath = k_CellAttributeMatrix.createChildPath(k_Mask);
  const DataPath k_FeatureIdsPath = k_CellAttributeMatrix.createChildPath(k_FeatureIds);
  const DataPath k_CellParentIdsPath = k_CellAttributeMatrix.createChildPath("ParentIds");

  const DataPath k_CellEnsembleAttributeMatrix = k_DataContainerPath.createChildPath("CellEnsembleData");
  const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleAttributeMatrix.createChildPath(EbsdLib::EnsembleData::CrystalStructures);

  const DataPath k_CellFeauteAttributeMatrix = k_DataContainerPath.createChildPath(k_FeatureAttributeMatrix);
  const DataPath k_CellFeaturePhasesPath = k_CellFeauteAttributeMatrix.createChildPath(k_Phases);
  const DataPath k_AvgQuatsPath = k_CellFeauteAttributeMatrix.createChildPath("AvgQuats");
  const DataPath k_CellFeatureEulerAnglesPath = k_CellFeauteAttributeMatrix.createChildPath(EbsdLib::CellData::EulerAngles);
  const DataPath k_NumNeighborsPath = k_CellFeauteAttributeMatrix.createChildPath("NumNeighbors2");
  const DataPath k_NeighborListPath = k_CellFeauteAttributeMatrix.createChildPath("NeighborList2");
  const DataPath k_SharedSurfaceAreaListPath = k_CellFeauteAttributeMatrix.createChildPath("SharedSurfaceAreaList2");
  const DataPath k_FeatureParentIdsPath = k_CellFeauteAttributeMatrix.createChildPath("ParentIds");

  const DataPath k_NewFeauteAttributeMatrix = k_DataContainerPath.createChildPath("NewCellFeatureData");
  const DataPath k_NewFeauteActivePath = k_NewFeauteAttributeMatrix.createChildPath("Active");

  DataStructure ds;

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
    auto preflightResult = filter->preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(ds, args);
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
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_CellFeauteAttributeMatrix));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<DataPath>(k_CellFeauteAttributeMatrix.createChildPath(k_Active)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  }

  // FindFeaturePhases filter
  {
    auto filter = filterList->createFilter(k_FindNFeaturePhasesFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;

    constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
    constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
    constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";

    args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeaturePhasesPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(ds, args);
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
    args.insertOrAssign(FindAvgOrientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_AvgQuatsPath));
    args.insertOrAssign(FindAvgOrientationsFilter::k_AvgEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureEulerAnglesPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
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
    args.insert(k_BoundaryCells_Key, std::make_any<DataPath>(boundaryCells));
    args.insert(k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighborsPath));
    args.insert(k_NeighborList_Key, std::make_any<DataPath>(k_NeighborListPath));
    args.insert(k_SharedSurfaceArea_Key, std::make_any<DataPath>(k_SharedSurfaceAreaListPath));
    args.insert(k_SurfaceFeatures_Key, std::make_any<DataPath>(surfaceFeatures));

    auto preflightResult = filter->preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter->execute(ds, args);
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
    args.insertOrAssign(MergeTwinsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(MergeTwinsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(MergeTwinsFilter::k_CellParentIdsArrayName_Key, std::make_any<DataPath>(k_CellParentIdsPath));
    args.insertOrAssign(MergeTwinsFilter::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_NewFeauteAttributeMatrix));
    args.insertOrAssign(MergeTwinsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<DataPath>(k_FeatureParentIdsPath));
    args.insertOrAssign(MergeTwinsFilter::k_ActiveArrayName_Key, std::make_any<DataPath>(k_NewFeauteActivePath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  }

  // check results
  {
    Int32Array* cellParentIdsArray = ds.getDataAs<Int32Array>(k_CellParentIdsPath);
    REQUIRE(cellParentIdsArray != nullptr);
    auto cellParentIdsTupleShape = cellParentIdsArray->getIDataStore()->getTupleShape();
    auto cellParentIdsNumComps = cellParentIdsArray->getNumberOfComponents();
    REQUIRE(cellParentIdsTupleShape.size() == 3);
    REQUIRE(cellParentIdsTupleShape[0] == 117);
    REQUIRE(cellParentIdsTupleShape[1] == 201);
    REQUIRE(cellParentIdsTupleShape[2] == 189);
    REQUIRE(cellParentIdsNumComps == 1);

    Int32Array* featureParentIdsArray = ds.getDataAs<Int32Array>(k_FeatureParentIdsPath);
    REQUIRE(featureParentIdsArray != nullptr);
    auto featureParentIdsTupleShape = featureParentIdsArray->getIDataStore()->getTupleShape();
    auto featureParentIdsNumComps = featureParentIdsArray->getNumberOfComponents();
    REQUIRE(featureParentIdsTupleShape.size() == 1);
    REQUIRE(featureParentIdsTupleShape[0] == 5416);
    REQUIRE(featureParentIdsNumComps == 1);

    BoolArray* activeArray = ds.getDataAs<BoolArray>(k_NewFeauteActivePath);
    REQUIRE(activeArray != nullptr);
    auto activeTupleShape = activeArray->getIDataStore()->getTupleShape();
    auto activeNumComps = activeArray->getNumberOfComponents();
    REQUIRE(activeTupleShape.size() == 1);
    REQUIRE(activeTupleShape[0] == 4507);
    REQUIRE(activeNumComps == 1);
  }
}
