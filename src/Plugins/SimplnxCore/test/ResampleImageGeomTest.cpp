#include "SimplnxCore/Filters/ResampleImageGeomFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <any>
#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const ChoicesParameter::ValueType k_SpacingModeIndex = 0;
const ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;
} // namespace

struct CompareDataArrayFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right, usize start = 0)
  {
    UnitTest::CompareDataArrays<T>(left, right, start);
  }
};

TEST_CASE("SimplnxCore::ResampleImageGeom: Output keeps the input length unit", "[SimplnxCore][ResampleImageGeom]")
{
  auto app = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool removeOriginal = GENERATE(false, true);
  const auto units = GENERATE(IGeometry::LengthUnit::Millimeter, IGeometry::LengthUnit::Meter, IGeometry::LengthUnit::Unspecified);
  CAPTURE(removeOriginal, units);

  DataStructure dataStructure;
  const DataPath srcGeomPath({"Input Geometry"});
  const DataPath destGeomPath({"Resampled Geometry"});
  const DataPath inputDataPath = srcGeomPath.createChildPath("Cell Data").createChildPath("Values");
  const FloatVec3 origin = {1.0F, -2.0F, 3.0F};
  const CreateImageGeometryAction inputGeometryAction(srcGeomPath, {2, 2, 2}, {1.0F, -2.0F, 3.0F}, {1.0F, 1.0F, 1.0F}, "Cell Data", units);
  auto inputGeometryResult = inputGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(inputGeometryResult);
  const auto* inputGeomPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  REQUIRE((inputGeomPtr != nullptr));
  REQUIRE((inputGeomPtr->getUnits() == units));
  const auto inputGeomId = inputGeomPtr->getId();
  const auto* inputCellDataPtr = inputGeomPtr->getCellData();
  REQUIRE((inputCellDataPtr != nullptr));
  auto* inputValuesPtr = Int32Array::Create(dataStructure, "Values", DataStoreUtilities::CreateDataStore<int32>(dataStructure, inputDataPath, {2, 2, 2}, {1}), inputCellDataPtr->getId());
  REQUIRE((inputValuesPtr != nullptr));
  inputValuesPtr->fill(7);

  const ResampleImageGeomFilter filter;
  Arguments args;
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
  args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.5F, 0.5F, 0.5F}));
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(removeOriginal));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const CreateImageGeometryAction* plannedGeometryPtr = nullptr;
  for(const auto& action : preflightResult.outputActions.value().actions)
  {
    if(const auto* candidatePtr = dynamic_cast<const CreateImageGeometryAction*>(action.get()); candidatePtr != nullptr)
    {
      plannedGeometryPtr = candidatePtr;
      break;
    }
  }
  REQUIRE((plannedGeometryPtr != nullptr));
  CHECK((plannedGeometryPtr->units() == units));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputGeomPath = removeOriginal ? srcGeomPath : destGeomPath;
  const auto* outputGeomPtr = dataStructure.getDataAs<ImageGeom>(outputGeomPath);
  REQUIRE((outputGeomPtr != nullptr));
  CHECK((outputGeomPtr->getUnits() == units));
  CHECK((outputGeomPtr->getDimensions() == SizeVec3{4, 4, 4}));
  CHECK((outputGeomPtr->getSpacing() == FloatVec3{0.5F, 0.5F, 0.5F}));
  CHECK((outputGeomPtr->getOrigin() == origin));
  const auto* outputValuesPtr = dataStructure.getDataAs<Int32Array>(outputGeomPath.createChildPath("Cell Data").createChildPath("Values"));
  REQUIRE((outputValuesPtr != nullptr));
  REQUIRE((outputValuesPtr->getNumberOfTuples() == 64));
  REQUIRE((outputValuesPtr->getNumberOfComponents() == 1));
  for(usize index = 0; index < outputValuesPtr->getSize(); ++index)
  {
    CHECK(((*outputValuesPtr)[index] == 7));
  }

  if(!removeOriginal)
  {
    const auto* retainedGeomPtr = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
    REQUIRE((retainedGeomPtr != nullptr));
    CHECK((retainedGeomPtr->getId() == inputGeomId));
    CHECK((retainedGeomPtr->getUnits() == units));
    CHECK((retainedGeomPtr->getDimensions() == SizeVec3{2, 2, 2}));
    CHECK((retainedGeomPtr->getSpacing() == FloatVec3{1.0F, 1.0F, 1.0F}));
    CHECK((retainedGeomPtr->getOrigin() == origin));
    const auto* retainedValuesPtr = dataStructure.getDataAs<Int32Array>(inputDataPath);
    REQUIRE((retainedValuesPtr != nullptr));
    REQUIRE((retainedValuesPtr->getNumberOfTuples() == 8));
    REQUIRE((retainedValuesPtr->getNumberOfComponents() == 1));
    for(usize index = 0; index < retainedValuesPtr->getSize(); ++index)
    {
      CHECK(((*retainedValuesPtr)[index] == 7));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Invalid Parameters", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  int errCode;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11502;
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11500;
  }

  // We do not need to test the "Exact Dimensions" resampling mode because the parameter type "VectorUInt64Parameter" automatically keeps the input values in range

  // Assign values to the filter parameters
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == errCode);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 3D In Place", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{47, 50, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto destFeatureData = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  const auto destNewGrainData = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  const auto destPhaseData = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);
  REQUIRE(destFeatureData != nullptr);
  REQUIRE(destNewGrainData != nullptr);
  REQUIRE(destPhaseData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();
  const auto destFeatDataDims = destFeatureData->getShape();
  const auto destNewGrainDataDims = destNewGrainData->getShape();
  const auto destPhaseDataDims = destPhaseData->getShape();
  // Get the exemplar data
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  const auto exemplarFeatureData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  const auto exemplarFeatDataDims = exemplarFeatureData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destFeatDataDims[0] == 214);
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());
  REQUIRE(destFeatureData->getSize() == exemplarFeatureData->getSize());
  REQUIRE(destNewGrainData->getSize() == 1);
  REQUIRE(destPhaseData->getSize() == 2);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
  const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, destNewGrainDataPath).value();
  for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
  const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, destPhaseDataPath).value();
  for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 3D Save Geometry", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100.str() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{47, 50, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto destFeatureData = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  const auto destNewGrainData = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  const auto destPhaseData = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  const auto exemplarFeatureData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);
  REQUIRE(destFeatureData != nullptr);
  REQUIRE(destNewGrainData != nullptr);
  REQUIRE(destPhaseData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();
  const auto destFeatDataDims = destFeatureData->getShape();
  const auto destNewGrainDataDims = destNewGrainData->getShape();
  const auto destPhaseDataDims = destPhaseData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  const auto exemplarFeatDataDims = exemplarFeatureData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destFeatDataDims[0] == 214);
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());
  REQUIRE(destFeatureData->getSize() == exemplarFeatureData->getSize());
  REQUIRE(destNewGrainData->getSize() == 1);
  REQUIRE(destPhaseData->getSize() == 2);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
  const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, destNewGrainDataPath).value();
  for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
  const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, destPhaseDataPath).value();
  for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 2D In Place", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({"Image2dDataContainer"});
  DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
  DataPath destGeomPath({srcGeomPath.getTargetName()});
  DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{15, 15, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());

  // check that the geometry(s) and the attribute matrices are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeom: 2D Save Geometry", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar_2.tar.gz", "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({"Image2dDataContainer"});
  DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
  DataPath destGeomPath({srcGeomPath.getTargetName() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());
  DataPath exemplarGeoPath;

  // Create default Parameters for the filter.
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{15, 15, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // check that the geometry(s) and the attribute matrices are all there
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ResampleImageGeomFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ResampleImageGeomFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ResampleImageGeomFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ResampleImageGeomFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_RenumberFeatures_Key) == true);
      CHECK(args.value<bool>(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key) == false);
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ResampleImageGeomFilter::k_CreatedImageGeometry_Key) == DataPath({"DataContainer"}));
    }
  }
}

TEST_CASE("SimplnxCore::ResampleImageGeom: Child names containing the geometry name are kept", "[SimplnxCore][ResampleImageGeom]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  const DataPath destGeomPath({"Geom Out"});
  auto* geomPtr = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  geomPtr->setOrigin({0.0F, 0.0F, 0.0F});
  geomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setCellData(*elementAmPtr);
  auto* dataPtr =
      Int32Array::Create(dataStructure, "Data", DataStoreUtilities::CreateDataStore<int32>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                         elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr = Int32Array::Create(dataStructure, "Geom Values",
                                       DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(42);
  ResampleImageGeomFilter filter;
  Arguments args;
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0.5F, 0.5F, 0.5F}));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.outputActions.value().actions)
    {
      const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
      if(candidatePtr != nullptr && candidatePtr->path() == srcGeomPath.createChildPath("Geom Feature Data"))
      {
        copyActionPtr = candidatePtr;
        break;
      }
    }
    REQUIRE(copyActionPtr != nullptr);
    const auto createdPaths = copyActionPtr->getAllCreatedPaths();
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedValuesPath) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), renamedAmPath.createChildPath("Geom Out Values")) == createdPaths.end());
  }
  SECTION("Copied objects keep child names")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataAs<AttributeMatrix>(copiedAmPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Int32Array>(copiedValuesPath) != nullptr);
    REQUIRE_FALSE(dataStructure.containsData(renamedAmPath));
    REQUIRE(dataStructure.getDataAs<Int32Array>(destGeomPath.createChildPath("Cell Data").createChildPath("Data")) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include <limits>

namespace
{
void CreateSamplingImage(DataStructure& ds)
{
  auto result = CreateImageGeometryAction(DataPath({"Image"}), {2, 1, 1}, {0, 0, 0}, {1, 1, 1}, "Cell Data").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  auto array = CreateArrayAction(DataType::int32, {1, 1, 2}, {1}, DataPath({"Image", "Cell Data", "Marker"})).apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(array);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "Marker"})));
  auto& values = ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "Marker"}));
  values[0] = 7;
  values[1] = 9;
}
} // namespace

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Spatial parameter domains", "[SimplnxCore][ResampleImageGeomFilter][SpatialParameterDomains]")
{

  DataStructure ds;
  CreateSamplingImage(ds);
  ResampleImageGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, DataPath({"Image"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, DataPath({"Resampled"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{0});
  args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::vector<float32>{std::numeric_limits<float32>::infinity(), 1, 1});
  const int32 expectedCode = -86113;
  const std::string parameter = "Spacing";

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  CHECK(result.outputActions.errors()[0].message.find(parameter) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("X value inf") != std::string::npos);
}

#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include <cmath>

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Parameter boundaries", "[SimplnxCore][ResampleImageGeomFilter][SpatialSamplingControls]")
{

  DataStructure ds;
  CreateSamplingImage(ds);
  ResampleImageGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, DataPath({"Image"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, DataPath({"Resampled"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{0});

  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);
  CAPTURE(row);
  int32 expectedCode = -86113;
  if(row < 3)
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::vector<float32>{row == 0 ? 0.0F : row == 1 ? std::numeric_limits<float32>::quiet_NaN() : -1.0F, 1, 1});
    if(row == 2)
    {
      expectedCode = -11502;
    }
  }
  else if(row < 6)
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{1});
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::vector<float32>{row == 3 ? std::numeric_limits<float32>::quiet_NaN() :
                                                                                     row == 4 ? std::numeric_limits<float32>::infinity() :
                                                                                                0.0F,
                                                                                     100, 100});
    expectedCode = row == 5 ? -11500 : -86114;
  }
  else
  {
    expectedCode = -1;
    if(row == 6)
    {
      args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::vector<float32>{1, 1});
    }
    else
    {
      args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{1});
      args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::vector<float32>{100, 100});
    }
  }
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  if(row >= 6)
  {
    const auto key = row == 6 ? ResampleImageGeomFilter::k_Spacing_Key : ResampleImageGeomFilter::k_Scaling_Key;
    CHECK(filter.parameters().at(key)->humanName() == (row == 6 ? "New Spacing" : "Scale Factor (percentages)"));
    CHECK(result.outputActions.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
  }
  const auto& message = result.outputActions.errors()[0].message;
  if(row < 6)
  {
    CHECK(message.find(row >= 3 ? "Scaling Factor" : "Spacing") != std::string::npos);
    CHECK(message.find("X") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Minimum corner mapping", "[SimplnxCore][ResampleImageGeomFilter][SpatialSamplingControls]")
{

  DataStructure ds;
  CreateSamplingImage(ds);
  ResampleImageGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, DataPath({"Image"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, DataPath({"Resampled"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{0});

  args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::vector<float32>{0.5F, 1, 1});
  args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::vector<float32>{});
  args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::vector<uint64>{});
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"})));
  const auto& geom = ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"}));
  CHECK(geom.getDimensions() == SizeVec3{4, 1, 1});
  CHECK(geom.getSpacing() == FloatVec3{0.5F, 1, 1});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "Marker"})));
  const auto& values = ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "Marker"}));
  REQUIRE(values.getNumberOfTuples() == 4);
  CHECK(values[0] == 7);
  CHECK(values[1] == 7);
  CHECK(values[2] == 9);
  CHECK(values[3] == 9);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "Marker"})));
  const auto& source = ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "Marker"}));
  CHECK(source[0] == 7);
  CHECK(source[1] == 9);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Derived spacing warnings", "[SimplnxCore][ResampleImageGeomFilter][SpatialSamplingControls]")
{

  DataStructure ds;
  CreateSamplingImage(ds);
  ResampleImageGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, DataPath({"Image"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, DataPath({"Resampled"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{0});

  const bool underflow = GENERATE(false, true);
  CAPTURE(underflow);
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Image"})));
  auto& source = ds.getDataRefAs<ImageGeom>(DataPath({"Image"}));
  if(underflow)
  {
    source.setDimensions({1, 1, 1});
    auto resize = source.getCellDataRef().resizeTuples({1, 1, 1});
    SIMPLNX_RESULT_REQUIRE_VALID(resize);
    source.setSpacing({std::numeric_limits<float32>::denorm_min(), 1, 1});
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{1});
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::vector<float32>{200, 100, 100});
  }
  else
  {
    source.setSpacing({3e38F, 1, 1});
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{2});
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::vector<uint64>{1, 1, 1});
  }
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.warnings().size() == 1);
  CHECK(preflight.outputActions.warnings()[0].code == -86129);
  CHECK(preflight.outputActions.warnings()[0].message.find("stand-in") != std::string::npos);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == -86126);
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"})));
  const auto& placeholder = ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"}));
  CHECK(placeholder.getDimensions() == SizeVec3{1, 1, 1});
  CHECK(placeholder.getSpacing() == FloatVec3{1, 1, 1});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "Marker"})));
  CHECK(ds.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "Marker"}))[0] == 7);
}

TEST_CASE("SimplnxCore::ResampleImageGeomFilter: Pipeline provisional spacing", "[SimplnxCore][ResampleImageGeomFilter][SpatialSamplingControls]")
{

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  Pipeline pipeline;
  ApplyTransformationToGeometryFilter transform;
  auto first = transform.getDefaultArguments();
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, DataPath({"Image"}));
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, DataPath({"Image", "Cell Data"}));
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, uint64{1});
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, uint64{0});
  first.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, DataPath({"Matrix"}));
  REQUIRE(pipeline.push_back(std::make_unique<ApplyTransformationToGeometryFilter>(), first));
  ResampleImageGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, DataPath({"Image"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, DataPath({"Resampled"}));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, false);
  args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, uint64{1});
  args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::vector<float32>{5e-37F, 100, 100});
  REQUIRE(pipeline.push_back(std::make_unique<ResampleImageGeomFilter>(), args));
  for(bool execute : {false, true})
  {
    DataStructure ds;
    CreateSamplingImage(ds);
    REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Image"})));
    ds.getDataRefAs<ImageGeom>(DataPath({"Image"})).setSpacing({2, 1, 1});
    auto array = CreateArrayAction(DataType::float32, {1}, {16}, DataPath({"Matrix"})).apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(array);
    REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(DataPath({"Matrix"})));
    auto& matrix = ds.getDataRefAs<Float32Array>(DataPath({"Matrix"}));
    const std::array<float32, 16> entries = {0.5F, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    for(usize idx = 0; idx < 16; ++idx)
    {
      matrix[idx] = entries[idx];
    }
    if(!execute)
    {
      REQUIRE(pipeline.preflight(ds, false));
      const auto* node = dynamic_cast<const PipelineFilter*>(pipeline.at(1));
      REQUIRE(node != nullptr);
      bool found = false;
      for(const auto& warning : node->getWarnings())
      {
        if(warning.code == -86129)
        {
          found = true;
        }
      }
      CHECK(found);
    }
    else
    {
      REQUIRE(scope.execute([&] { return pipeline.execute(ds, false); }));
      REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"})));
      const auto& output = ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"}));
      CHECK(std::isfinite(output.getSpacing()[0]));
      CHECK(output.getSpacing()[0] == Approx(2e38F));
      REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "Marker"})));
      CHECK(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "Marker"}))[0] == 7);
    }
  }
}
