#include "SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.hpp"
#include "SimplnxCore/Filters/RotateSampleRefFrameFilter.hpp"
#include "SimplnxCore/Filters/WriteOnScaleTableFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <Eigen/Dense>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_GeometryPath({"Geometry"});
const DataPath k_CellDataPath = k_GeometryPath.createChildPath("Cell Data");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_EnsembleDataPath = k_GeometryPath.createChildPath("Cell Ensemble Data");
const DataPath k_PhaseNamesPath = k_EnsembleDataPath.createChildPath("PhaseNames");
const DataPath k_WrongFeatureIdsPath({"Wrong FeatureIds"});

template <class T>
DataStructure CreateImageDataStructure(const SizeVec3& dimensions, const std::vector<T>& featureIds, const std::vector<std::string>& phaseNames)
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions(dimensions);
  imageGeom->setSpacing({0.001F, 0.002F, 0.003F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), cellShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<T>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<T>::Create(dataStructure, k_FeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  REQUIRE(featureIdsArray != nullptr);
  REQUIRE(featureIds.size() == featureIdsArray->getNumberOfTuples());
  std::copy(featureIds.cbegin(), featureIds.cend(), featureIdsArray->begin());

  auto* ensembleData = AttributeMatrix::Create(dataStructure, k_EnsembleDataPath.getTargetName(), {phaseNames.size()}, imageGeom->getId());
  REQUIRE(ensembleData != nullptr);
  REQUIRE(StringArray::CreateWithValues(dataStructure, k_PhaseNamesPath.getTargetName(), {phaseNames.size()}, phaseNames, ensembleData->getId()) != nullptr);

  return dataStructure;
}

template <class T>
DataStructure CreateRectGridDataStructure(const SizeVec3& dimensions, const std::array<std::vector<float32>, 3>& bounds, const std::vector<T>& featureIds, const std::vector<std::string>& phaseNames)
{
  DataStructure dataStructure;
  auto* rectGrid = RectGridGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(rectGrid != nullptr);
  rectGrid->setDimensions(dimensions);

  const std::array<std::string, 3> boundsNames = {"X Bounds", "Y Bounds", "Z Bounds"};
  std::array<Float32Array*, 3> boundsArrays = {};
  for(usize axis = 0; axis < bounds.size(); axis++)
  {
    boundsArrays[axis] = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, boundsNames[axis], {bounds[axis].size()}, {1}, rectGrid->getId());
    REQUIRE(boundsArrays[axis] != nullptr);
    std::copy(bounds[axis].cbegin(), bounds[axis].cend(), boundsArrays[axis]->begin());
  }
  rectGrid->setXBoundsId(boundsArrays[0]->getId());
  rectGrid->setYBoundsId(boundsArrays[1]->getId());
  rectGrid->setZBoundsId(boundsArrays[2]->getId());

  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), cellShape, rectGrid->getId());
  REQUIRE(cellData != nullptr);
  rectGrid->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<T>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<T>::Create(dataStructure, k_FeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  REQUIRE(featureIdsArray != nullptr);
  REQUIRE(featureIds.size() == featureIdsArray->getNumberOfTuples());
  std::copy(featureIds.cbegin(), featureIds.cend(), featureIdsArray->begin());

  auto* ensembleData = AttributeMatrix::Create(dataStructure, k_EnsembleDataPath.getTargetName(), {phaseNames.size()}, rectGrid->getId());
  REQUIRE(ensembleData != nullptr);
  REQUIRE(StringArray::CreateWithValues(dataStructure, k_PhaseNamesPath.getTargetName(), {phaseNames.size()}, phaseNames, ensembleData->getId()) != nullptr);

  return dataStructure;
}

Arguments CreateArguments(const fs::path& outputPath, const std::string& prefix, const VectorInt32Parameter::ValueType& numKeypoints = {2, 3, 4})
{
  Arguments args = WriteOnScaleTableFileFilter().getDefaultArguments();
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>(prefix));
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_NumKeypoints_Key, std::make_any<VectorInt32Parameter::ValueType>(numKeypoints));
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_PhaseNamesArrayPath_Key, std::make_any<DataPath>(k_PhaseNamesPath));
  return args;
}

std::string ReadFile(const fs::path& filePath)
{
  std::ifstream input(filePath, std::ios::binary);
  REQUIRE(input.is_open());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string ExtractMatrBlock(const std::string& contents)
{
  const usize headerPosition = contents.find("matr ");
  REQUIRE(headerPosition != std::string::npos);
  const usize valuesPosition = contents.find('\n', headerPosition);
  REQUIRE(valuesPosition != std::string::npos);
  return contents.substr(valuesPosition + 1);
}

DynamicTableParameter::ValueType CreateLegacyYRotationTable()
{
  const float32 angle = std::numbers::pi_v<float32> / 2.0F;
  const float32 cosAngle = std::cos(angle);
  const float32 sinAngle = std::sin(angle);
  Eigen::Matrix3f matrix;
  matrix << Eigen::Vector3f{cosAngle, 0.0F, -sinAngle}, Eigen::Vector3f{0.0F, 1.0F, 0.0F}, Eigen::Vector3f{sinAngle, 0.0F, cosAngle};

  DynamicTableParameter::ValueType table(4, std::vector<float64>(4, 0.0));
  for(usize row = 0; row < 3; row++)
  {
    for(usize col = 0; col < 3; col++)
    {
      table[row][col] = matrix(row, col);
    }
  }
  table[3][3] = 1.0;
  return table;
}

std::string CreateMatrBlock(const Int32Array& featureIds)
{
  std::string block;
  for(usize index = 0; index < featureIds.getNumberOfTuples(); index++)
  {
    if(index != 0)
    {
      block += index % 40 == 0 ? "\n" : " ";
    }
    block += fmt::format("{}", featureIds[index]);
  }
  return block;
}
} // namespace

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Image Geometry Descending Dims", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<int32> featureIds = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};
  DataStructure dataStructure = CreateImageDataStructure<int32>({4, 3, 2}, featureIds, {"", "pzt4t11", "pzt4t12"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "DescendingDims";
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  const Arguments args = CreateArguments(outputPath, "OnScale_Test");
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expected = R"(hedr 0
info 1
xcrd 5
0.00000000E+00 1.00000005E-03 2.00000009E-03 3.00000003E-03 4.00000019E-03
ycrd 4
0.00000000E+00 2.00000009E-03 4.00000019E-03 6.00000005E-03
zcrd 3
0.00000000E+00 3.00000003E-03 6.00000005E-03
keypoints
2 3 4
divisions
4 3 2
name 3
pzt4t11 pzt4t12 Phase_3 
matr 24
1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3)";
  REQUIRE(ReadFile(outputPath / "OnScale_Test.flxtbl") == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: UInt8 Feature Ids", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint8> featureIds = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};
  DataStructure dataStructure = CreateImageDataStructure<uint8>({4, 3, 2}, featureIds, {"", "pzt4t11", "pzt4t12"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "UInt8";
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_UInt8"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(ExtractMatrBlock(ReadFile(outputPath / "OnScale_UInt8.flxtbl")) == "1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3");
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: RectGrid Geometry", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<std::vector<float32>, 3> bounds = {{{0.0F, 0.25F, 1.0F, 2.5F}, {0.0F, 0.1F, 0.75F}, {0.0F, 3.25F}}};
  DataStructure dataStructure = CreateRectGridDataStructure<int32>({3, 2, 1}, bounds, {1, 2, 3, 1, 2, 3}, {"", "Phase1", "Phase2", "Phase3"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "RectGrid";
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_RectGrid"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string contents = ReadFile(outputPath / "OnScale_RectGrid.flxtbl");
  REQUIRE(contents.find("xcrd 4\n0.00000000E+00 2.50000000E-01 1.00000000E+00 2.50000000E+00\n") != std::string::npos);
  REQUIRE(contents.find("ycrd 3\n0.00000000E+00 1.00000001E-01 7.50000000E-01\n") != std::string::npos);
  REQUIRE(contents.find("zcrd 2\n0.00000000E+00 3.25000000E+00\n") != std::string::npos);
  REQUIRE(contents.find("divisions\n3 2 1\n") != std::string::npos);
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Ascending Dims Are Reordered", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<int32> featureIds(24);
  std::iota(featureIds.begin(), featureIds.end(), 1);
  std::vector<std::string> phaseNames(25);
  for(usize index = 1; index < phaseNames.size(); index++)
  {
    phaseNames[index] = fmt::format("Phase{}", index);
  }

  DataStructure dataStructure = CreateImageDataStructure<int32>({2, 3, 4}, featureIds, phaseNames);
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "Reordered";
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_Reordered"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  DataStructure expectedDataStructure = CreateImageDataStructure<int32>({2, 3, 4}, featureIds, phaseNames);
  RotateSampleRefFrameFilter rotateFilter;
  Arguments rotateArgs = rotateFilter.getDefaultArguments();
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                            std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(CreateLegacyYRotationTable()));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotateSliceBySlice_Key, std::make_any<bool>(false));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_KeepInputGeometryOrigin_Key, std::make_any<bool>(false));
  auto rotatePreflight = rotateFilter.preflight(expectedDataStructure, rotateArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(rotatePreflight.outputActions);
  auto rotateExecute = rotateFilter.execute(expectedDataStructure, rotateArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(rotateExecute.result);

  REQUIRE_NOTHROW(expectedDataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& expectedFeatureIds = expectedDataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const std::string contents = ReadFile(outputPath / "OnScale_Reordered.flxtbl");
  REQUIRE(contents.find("xcrd 5\n") != std::string::npos);
  REQUIRE(contents.find("ycrd 4\n") != std::string::npos);
  REQUIRE(contents.find("zcrd 3\n") != std::string::npos);
  REQUIRE(contents.find("divisions\n4 3 2\n") != std::string::npos);
  REQUIRE(ExtractMatrBlock(contents) == CreateMatrBlock(expectedFeatureIds));

  std::istringstream matrixStream(ExtractMatrBlock(contents));
  std::vector<int32> writtenIds{std::istream_iterator<int32>(matrixStream), std::istream_iterator<int32>()};
  std::sort(writtenIds.begin(), writtenIds.end());
  REQUIRE(writtenIds == featureIds);
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: RectGrid Needs Reorder Fails Preflight", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<std::vector<float32>, 3> bounds = {{{0.0F, 1.0F}, {0.0F, 1.0F, 2.0F}, {0.0F, 1.0F, 2.0F, 3.0F}}};
  DataStructure dataStructure = CreateRectGridDataStructure<int32>({1, 2, 3}, bounds, {1, 2, 3, 1, 2, 3}, {"", "Phase1", "Phase2", "Phase3"});
  const WriteOnScaleTableFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "RectGrid_Reorder"));
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Tuple Count Mismatch", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({2, 2, 1}, {1, 2, 3, 1}, {"", "Phase1", "Phase2", "Phase3"});
  auto wrongFeatureIdsStore = DataStoreUtilities::CreateDataStore<int32>({3}, {1}, IDataAction::Mode::Execute);
  REQUIRE(Int32Array::Create(dataStructure, k_WrongFeatureIdsPath.getTargetName(), wrongFeatureIdsStore) != nullptr);
  const WriteOnScaleTableFileFilter filter;
  Arguments args = CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Tuple_Mismatch");
  args.insertOrAssign(WriteOnScaleTableFileFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_WrongFeatureIdsPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Missing Output Directory", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({1, 1, 1}, {1}, {"", "Phase1"});
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path missingPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteOnScaleTableFile_Missing_{}", uniqueSuffix);
  REQUIRE_FALSE(fs::exists(missingPath));

  const WriteOnScaleTableFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, CreateArguments(missingPath, "Missing_Output_Directory"));
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";
  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteOnScaleTableFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteOnScaleTableFileFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteOnScaleTableFileFilter>::uuid);
      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteOnScaleTableFileFilter::k_OutputPath_Key) == fs::path("/test/path"));
      CHECK(args.value<std::string>(WriteOnScaleTableFileFilter::k_FilePrefix_Key) == "TestPrefix");
      CHECK(args.value<VectorInt32Parameter::ValueType>(WriteOnScaleTableFileFilter::k_NumKeypoints_Key) == VectorInt32Parameter::ValueType{3, 4, 5});
      CHECK(args.value<DataPath>(WriteOnScaleTableFileFilter::k_InputGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteOnScaleTableFileFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "FeatureIds"}));
      CHECK(args.value<DataPath>(WriteOnScaleTableFileFilter::k_PhaseNamesArrayPath_Key) == DataPath({"DataContainer", "CellEnsembleData", "PhaseName"}));
    }
  }
}
