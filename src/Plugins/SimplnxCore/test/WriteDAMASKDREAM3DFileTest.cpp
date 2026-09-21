#include "SimplnxCore/Filters/WriteDAMASKDREAM3DFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_SourceGeomPath({"Source Geometry"});
const DataPath k_SourceCellDataPath = k_SourceGeomPath.createChildPath("Raw Cells");
const DataPath k_SourceCellEulerPath = k_SourceCellDataPath.createChildPath("Measured Orientations");
const DataPath k_SourceCellPhasesPath = k_SourceCellDataPath.createChildPath("Cell Phase Values");
const DataPath k_SourceFeatureIdsPath = k_SourceCellDataPath.createChildPath("Segment Labels");
const DataPath k_SourceUnrelatedPath = k_SourceCellDataPath.createChildPath("Unrelated Values");
const DataPath k_SourceFeatureDataPath = k_SourceGeomPath.createChildPath("Raw Features");
const DataPath k_SourceFeatureEulerPath = k_SourceFeatureDataPath.createChildPath("Average Orientations");
const DataPath k_SourceFeaturePhasesPath = k_SourceFeatureDataPath.createChildPath("Feature Phase Values");
const DataPath k_SourceEnsembleDataPath = k_SourceGeomPath.createChildPath("Raw Ensembles");
const DataPath k_SourcePhaseNamesPath = k_SourceEnsembleDataPath.createChildPath("Material Labels");

const DataPath k_OutputGeomPath({"DataContainer"});
const DataPath k_OutputCellDataPath = k_OutputGeomPath.createChildPath("CellData");
const DataPath k_OutputCellEulerPath = k_OutputCellDataPath.createChildPath("EulerAngles");
const DataPath k_OutputCellPhasesPath = k_OutputCellDataPath.createChildPath("Phases");
const DataPath k_OutputFeatureIdsPath = k_OutputCellDataPath.createChildPath("FeatureIds");
const DataPath k_OutputFeatureDataPath = k_OutputGeomPath.createChildPath("CellFeatureData");
const DataPath k_OutputFeatureEulerPath = k_OutputFeatureDataPath.createChildPath("EulerAngles");
const DataPath k_OutputFeaturePhasesPath = k_OutputFeatureDataPath.createChildPath("Phases");
const DataPath k_OutputEnsembleDataPath = k_OutputGeomPath.createChildPath("CellEnsembleData");
const DataPath k_OutputPhaseNamesPath = k_OutputEnsembleDataPath.createChildPath("PhaseName");

template <typename T>
DataArray<T>& CreateArray(DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const ShapeType& componentShape, const std::vector<T>& values, DataObject::IdType parentId)
{
  auto store = DataStoreUtilities::CreateDataStore<T>(dataStructure, path, tupleShape, componentShape);
  auto* array = DataArray<T>::Create(dataStructure, path.getTargetName(), store, parentId);
  REQUIRE(array != nullptr);
  REQUIRE(values.size() == array->getSize());
  std::copy(values.cbegin(), values.cend(), array->begin());
  return *array;
}

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_SourceGeomPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({2, 2, 1});
  imageGeom->setSpacing({2.0F, 3.0F, 4.0F});
  imageGeom->setOrigin({5.0F, 6.0F, 7.0F});
  imageGeom->setUnits(IGeometry::LengthUnit::Micrometer);

  const ShapeType cellShape = {1, 2, 2};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_SourceCellDataPath.getTargetName(), cellShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  CreateArray<float32>(dataStructure, k_SourceCellEulerPath, cellShape, {3}, {0.0F, 0.0F, 0.0F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F, 0.8F, 0.9F}, cellData->getId());
  CreateArray<int32>(dataStructure, k_SourceCellPhasesPath, cellShape, {1}, {1, 1, 2, 2}, cellData->getId());
  CreateArray<int32>(dataStructure, k_SourceFeatureIdsPath, cellShape, {1}, {0, 1, 2, 2}, cellData->getId());
  CreateArray<float32>(dataStructure, k_SourceUnrelatedPath, cellShape, {1}, {9.0F, 8.0F, 7.0F, 6.0F}, cellData->getId());

  const ShapeType featureShape = {3};
  auto* featureData = AttributeMatrix::Create(dataStructure, k_SourceFeatureDataPath.getTargetName(), featureShape, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  CreateArray<float32>(dataStructure, k_SourceFeatureEulerPath, featureShape, {3}, {0.0F, 0.0F, 0.0F, 0.25F, 0.5F, 0.75F, 1.0F, 1.25F, 1.5F}, featureData->getId());
  CreateArray<int32>(dataStructure, k_SourceFeaturePhasesPath, featureShape, {1}, {0, 1, 2}, featureData->getId());

  const ShapeType ensembleShape = {3};
  auto* ensembleData = AttributeMatrix::Create(dataStructure, k_SourceEnsembleDataPath.getTargetName(), ensembleShape, imageGeom->getId());
  REQUIRE(ensembleData != nullptr);
  auto* phaseNames = StringArray::CreateWithValues(dataStructure, k_SourcePhaseNamesPath.getTargetName(), ensembleShape, {"Unknown", "Ferrite", "Austenite"}, ensembleData->getId());
  REQUIRE(phaseNames != nullptr);
  return dataStructure;
}

Arguments CreateArguments(const fs::path& outputFile, ChoicesParameter::ValueType representation, bool writePhaseNames)
{
  Arguments args = WriteDAMASKDREAM3DFileFilter().getDefaultArguments();
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_UseCompression_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_CompressionLevel_Key, std::make_any<int32>(5));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_Representation_Key, std::make_any<ChoicesParameter::ValueType>(representation));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_WritePhaseNames_Key, std::make_any<bool>(writePhaseNames));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_ScaleToMeters_Key, std::make_any<float64>(1.0));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_SourceGeomPath));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_SourceCellEulerPath));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_SourceCellPhasesPath));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_SourceFeatureIdsPath));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_SourceFeatureEulerPath));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_SourceFeaturePhasesPath));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_PhaseNamesArrayPath_Key, std::make_any<DataPath>(k_SourcePhaseNamesPath));
  return args;
}

DataStructure ReadOutput(const fs::path& outputFile)
{
  auto readResult = DREAM3D::ReadFile(outputFile);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  return std::move(readResult.value().second);
}

void CheckGeometry(const DataStructure& outputDataStructure)
{
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<ImageGeom>(k_OutputGeomPath));
  const auto& outputGeom = outputDataStructure.getDataRefAs<ImageGeom>(k_OutputGeomPath);
  REQUIRE(outputGeom.getDimensions() == SizeVec3(2, 2, 1));
  REQUIRE(outputGeom.getUnits() == IGeometry::LengthUnit::Meter);
  REQUIRE(outputGeom.getSpacing()[0] == Approx(2.0e-6));
  REQUIRE(outputGeom.getSpacing()[1] == Approx(3.0e-6));
  REQUIRE(outputGeom.getSpacing()[2] == Approx(4.0e-6));
  REQUIRE(outputGeom.getOrigin()[0] == Approx(5.0e-6));
  REQUIRE(outputGeom.getOrigin()[1] == Approx(6.0e-6));
  REQUIRE(outputGeom.getOrigin()[2] == Approx(7.0e-6));
}
} // namespace

TEST_CASE("SimplnxCore::WriteDAMASKDREAM3DFileFilter: Canonical Pointwise Export", "[SimplnxCore][WriteDAMASKDREAM3DFileFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateDataStructure();
  const fs::path outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteDAMASKDREAM3DFile";
  fs::create_directories(outputDir);
  const fs::path outputFile = outputDir / "pointwise.dream3d";
  fs::remove(outputFile);

  const WriteDAMASKDREAM3DFileFilter filter;
  Arguments args = CreateArguments(outputFile, 0, true);
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_UseCompression_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_CompressionLevel_Key, std::make_any<int32>(5));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataStructure outputDataStructure = ReadOutput(outputFile);
  CheckGeometry(outputDataStructure);
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<Float32Array>(k_OutputCellEulerPath));
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<Int32Array>(k_OutputCellPhasesPath));
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<StringArray>(k_OutputPhaseNamesPath));
  UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<Float32Array>(k_SourceCellEulerPath), outputDataStructure.getDataRefAs<Float32Array>(k_OutputCellEulerPath));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_SourceCellPhasesPath), outputDataStructure.getDataRefAs<Int32Array>(k_OutputCellPhasesPath));
  REQUIRE(outputDataStructure.getDataRefAs<StringArray>(k_OutputPhaseNamesPath).values() == std::vector<std::string>{"Unknown", "Ferrite", "Austenite"});
  REQUIRE_FALSE(outputDataStructure.containsData(k_OutputFeatureIdsPath));
  REQUIRE_FALSE(outputDataStructure.containsData(k_OutputFeatureDataPath));
  REQUIRE_FALSE(outputDataStructure.containsData(k_SourceUnrelatedPath));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_SourceGeomPath));
  const auto& sourceGeom = dataStructure.getDataRefAs<ImageGeom>(k_SourceGeomPath);
  REQUIRE(sourceGeom.getSpacing() == FloatVec3(2.0F, 3.0F, 4.0F));
  REQUIRE(sourceGeom.getOrigin() == FloatVec3(5.0F, 6.0F, 7.0F));
  REQUIRE(sourceGeom.getUnits() == IGeometry::LengthUnit::Micrometer);
  UnitTest::CheckArraysInheritTupleDims(outputDataStructure);
}

TEST_CASE("SimplnxCore::WriteDAMASKDREAM3DFileFilter: Canonical Grainwise Export", "[SimplnxCore][WriteDAMASKDREAM3DFileFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateDataStructure();
  const fs::path outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteDAMASKDREAM3DFile";
  fs::create_directories(outputDir);
  const fs::path outputFile = outputDir / "grainwise.dream3d";
  fs::remove(outputFile);

  const WriteDAMASKDREAM3DFileFilter filter;
  Arguments args = CreateArguments(outputFile, 1, false);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataStructure outputDataStructure = ReadOutput(outputFile);
  CheckGeometry(outputDataStructure);
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<Int32Array>(k_OutputFeatureIdsPath));
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<Float32Array>(k_OutputFeatureEulerPath));
  REQUIRE_NOTHROW(outputDataStructure.getDataRefAs<Int32Array>(k_OutputFeaturePhasesPath));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_SourceFeatureIdsPath), outputDataStructure.getDataRefAs<Int32Array>(k_OutputFeatureIdsPath));
  UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<Float32Array>(k_SourceFeatureEulerPath), outputDataStructure.getDataRefAs<Float32Array>(k_OutputFeatureEulerPath));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_SourceFeaturePhasesPath), outputDataStructure.getDataRefAs<Int32Array>(k_OutputFeaturePhasesPath));
  REQUIRE_FALSE(outputDataStructure.containsData(k_OutputCellEulerPath));
  REQUIRE_FALSE(outputDataStructure.containsData(k_OutputCellPhasesPath));
  REQUIRE_FALSE(outputDataStructure.containsData(k_OutputEnsembleDataPath));
  REQUIRE_FALSE(outputDataStructure.containsData(k_SourceUnrelatedPath));
  UnitTest::CheckArraysInheritTupleDims(outputDataStructure);
}

TEST_CASE("SimplnxCore::WriteDAMASKDREAM3DFileFilter: Preflight Errors", "[SimplnxCore][WriteDAMASKDREAM3DFileFilter]")
{
  UnitTest::LoadPlugins();
  const WriteDAMASKDREAM3DFileFilter filter;
  const fs::path outputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteDAMASKDREAM3DFile" / "invalid.dream3d";

  SECTION("Cell Tuple Count")
  {
    DataStructure dataStructure = CreateDataStructure();
    dataStructure.getDataRefAs<ImageGeom>(k_SourceGeomPath).setDimensions({5, 1, 1});
    Arguments args = CreateArguments(outputFile, 0, false);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -12105);
  }

  SECTION("Feature Tuple Count")
  {
    DataStructure dataStructure = CreateDataStructure();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_SourceFeaturePhasesPath));
    auto resizeResult = dataStructure.getDataRefAs<Int32Array>(k_SourceFeaturePhasesPath).resizeTuples({2});
    SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
    Arguments args = CreateArguments(outputFile, 1, false);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -12103);
  }

  SECTION("Compression Level")
  {
    DataStructure dataStructure = CreateDataStructure();
    Arguments args = CreateArguments(outputFile, 0, false);
    args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_UseCompression_Key, std::make_any<bool>(true));
    args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_CompressionLevel_Key, std::make_any<int32>(0));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -12101);
  }

  SECTION("Unknown Units Scale")
  {
    DataStructure dataStructure = CreateDataStructure();
    dataStructure.getDataRefAs<ImageGeom>(k_SourceGeomPath).setUnits(IGeometry::LengthUnit::Unknown);
    Arguments args = CreateArguments(outputFile, 0, false);
    args.insertOrAssign(WriteDAMASKDREAM3DFileFilter::k_ScaleToMeters_Key, std::make_any<float64>(0.0));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -12104);
  }
}
