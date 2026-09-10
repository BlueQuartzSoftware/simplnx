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
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <numeric>
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
DataStructure CreateImageDataStructure(const SizeVec3& dimensions, const std::vector<T>& featureIds, const std::vector<std::string>& phaseNames, const FloatVec3& spacing = {0.001F, 0.002F, 0.003F},
                                       const FloatVec3& origin = {0.0F, 0.0F, 0.0F})
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions(dimensions);
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);

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

DynamicTableParameter::ValueType ConvertRotationMatrixToTable(const Eigen::Matrix3f& matrix)
{
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

Eigen::Matrix3f CreateLegacyXRotationMatrix()
{
  const float32 angle = std::numbers::pi_v<float32> / 2.0F;
  const float32 cosAngle = std::cos(angle);
  const float32 sinAngle = std::sin(angle);
  Eigen::Matrix3f matrix;
  matrix << 1.0F, 0.0F, 0.0F, 0.0F, cosAngle, -sinAngle, 0.0F, sinAngle, cosAngle;
  return matrix;
}

Eigen::Matrix3f CreateLegacyYRotationMatrix()
{
  const float32 angle = std::numbers::pi_v<float32> / 2.0F;
  const float32 cosAngle = std::cos(angle);
  const float32 sinAngle = std::sin(angle);
  Eigen::Matrix3f matrix;
  matrix << cosAngle, 0.0F, sinAngle, 0.0F, 1.0F, 0.0F, -sinAngle, 0.0F, cosAngle;
  return matrix;
}

Eigen::Matrix3f CreateLegacyZRotationMatrix()
{
  const float32 angle = std::numbers::pi_v<float32> / 2.0F;
  const float32 cosAngle = std::cos(angle);
  const float32 sinAngle = std::sin(angle);
  Eigen::Matrix3f matrix;
  matrix << cosAngle, -sinAngle, 0.0F, sinAngle, cosAngle, 0.0F, 0.0F, 0.0F, 1.0F;
  return matrix;
}

void RotateFeatureIds(DataStructure& dataStructure, const Eigen::Matrix3f& rotationMatrix)
{
  RotateSampleRefFrameFilter rotateFilter;
  Arguments rotateArgs = rotateFilter.getDefaultArguments();
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                            std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(ConvertRotationMatrixToTable(rotationMatrix)));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotateSliceBySlice_Key, std::make_any<bool>(false));
  rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_KeepInputGeometryOrigin_Key, std::make_any<bool>(false));

  auto rotateResult = rotateFilter.execute(dataStructure, rotateArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(rotateResult.result);
}

std::vector<std::string> CreateSequentialPhaseNames(usize count)
{
  std::vector<std::string> phaseNames(count + 1);
  for(usize index = 1; index < phaseNames.size(); index++)
  {
    phaseNames[index] = fmt::format("Phase{}", index);
  }
  return phaseNames;
}

std::string CreateMatrBlock(const Int32Array& featureIds);

void RequireFeatureIdMultiset(const std::string& matrBlock, const std::vector<int32>& expectedFeatureIds)
{
  std::istringstream matrixStream(matrBlock);
  std::vector<int32> writtenIds{std::istream_iterator<int32>(matrixStream), std::istream_iterator<int32>()};
  std::sort(writtenIds.begin(), writtenIds.end());

  std::vector<int32> sortedExpected = expectedFeatureIds;
  std::sort(sortedExpected.begin(), sortedExpected.end());
  REQUIRE(writtenIds == sortedExpected);
}

void RequireReorderedExportMatchesRotation(const SizeVec3& dimensions, const Eigen::Matrix3f& compositeRotation, const fs::path& outputPath, const std::string& filePrefix)
{
  const usize cellCount = dimensions[0] * dimensions[1] * dimensions[2];
  std::vector<int32> featureIds(cellCount);
  std::iota(featureIds.begin(), featureIds.end(), 1);
  const std::vector<std::string> phaseNames = CreateSequentialPhaseNames(cellCount);

  DataStructure dataStructure = CreateImageDataStructure<int32>(dimensions, featureIds, phaseNames, {1.0F, 1.0F, 1.0F});
  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, filePrefix));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  DataStructure expectedDataStructure = CreateImageDataStructure<int32>(dimensions, featureIds, phaseNames, {1.0F, 1.0F, 1.0F});
  RotateFeatureIds(expectedDataStructure, compositeRotation);

  REQUIRE_NOTHROW(expectedDataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& expectedFeatureIds = expectedDataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const std::string contents = ReadFile(outputPath / fmt::format("{}.flxtbl", filePrefix));
  REQUIRE(contents.find("divisions\n4 3 2\n") != std::string::npos);
  REQUIRE(ExtractMatrBlock(contents) == CreateMatrBlock(expectedFeatureIds));
  RequireFeatureIdMultiset(ExtractMatrBlock(contents), featureIds);
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

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Matr Wraps At Forty", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<int32> featureIds(50);
  for(usize index = 0; index < featureIds.size(); index++)
  {
    featureIds[index] = static_cast<int32>(index % 3 + 1);
  }
  DataStructure dataStructure = CreateImageDataStructure<int32>({10, 5, 1}, featureIds, {"", "Phase1", "Phase2", "Phase3"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "MatrWrap";
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_MatrWrap"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expected = "1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1\n"
                               "2 3 1 2 3 1 2 3 1 2";
  REQUIRE(ExtractMatrBlock(ReadFile(outputPath / "OnScale_MatrWrap.flxtbl")) == expected);
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Int64 And UInt64 Feature Ids", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "Int64UInt64";
  fs::create_directories(outputPath);
  const WriteOnScaleTableFileFilter filter;
  const std::string expected = "1 2 3 1 2 3";

  DataStructure int64DataStructure = CreateImageDataStructure<int64>({3, 2, 1}, {1, 2, 3, 1, 2, 3}, {"", "Phase1", "Phase2", "Phase3"});
  auto int64Result = filter.execute(int64DataStructure, CreateArguments(outputPath, "OnScale_Int64"));
  SIMPLNX_RESULT_REQUIRE_VALID(int64Result.result);
  REQUIRE(ExtractMatrBlock(ReadFile(outputPath / "OnScale_Int64.flxtbl")) == expected);

  DataStructure uint64DataStructure = CreateImageDataStructure<uint64>({3, 2, 1}, {1, 2, 3, 1, 2, 3}, {"", "Phase1", "Phase2", "Phase3"});
  auto uint64Result = filter.execute(uint64DataStructure, CreateArguments(outputPath, "OnScale_UInt64"));
  SIMPLNX_RESULT_REQUIRE_VALID(uint64Result.result);
  REQUIRE(ExtractMatrBlock(ReadFile(outputPath / "OnScale_UInt64.flxtbl")) == expected);
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Negative Feature Ids Fail", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({3, 1, 1}, {1, -1, 2}, {"", "Phase1", "Phase2"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "NegativeIds";
  fs::remove_all(outputPath);
  fs::create_directories(outputPath);
  const fs::path outputFile = outputPath / "OnScale_Negative.flxtbl";

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_Negative"));
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().front().code == -12037);
  REQUIRE(executeResult.result.errors().front().message == "Found 1 negative feature ids in 'Geometry/Cell Data/FeatureIds'. OnScale material indices must be 0 or greater.");
  REQUIRE_FALSE(fs::exists(outputFile));
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: No Positive Feature Ids Warns", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({3, 1, 1}, {0, 0, 0}, {""});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "NoPositiveIds";
  fs::create_directories(outputPath);
  std::vector<IFilter::Message> messages;
  const IFilter::MessageHandler messageHandler{[&messages](const IFilter::Message& message) { messages.push_back(message); }};

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_NoPositive"), nullptr, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string contents = ReadFile(outputPath / "OnScale_NoPositive.flxtbl");
  REQUIRE(contents.find("name 0\n\nmatr 3\n0 0 0") != std::string::npos);
  REQUIRE(std::any_of(messages.cbegin(), messages.cend(), [](const IFilter::Message& message) {
    return message.type == IFilter::Message::Type::Warning && message.message == "No positive feature ids were found in 'Geometry/Cell Data/FeatureIds'; the name section is empty.";
  }));
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Cancel Writes Nothing", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({2, 3, 4}, std::vector<int32>(24, 1), {"", "Phase1"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "Cancelled";
  fs::remove_all(outputPath);
  fs::create_directories(outputPath);
  std::atomic_bool shouldCancel{true};

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_Cancelled"), nullptr, {}, shouldCancel);
  REQUIRE(executeResult.result.invalid());
  REQUIRE_FALSE(fs::exists(outputPath / "OnScale_Cancelled.flxtbl"));
  REQUIRE(fs::directory_iterator(outputPath) == fs::directory_iterator());
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Cancel During Reorder Writes Nothing", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({2, 3, 4}, std::vector<int32>(24, 1), {"", "Phase1"});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "CancelledDuringReorder";
  fs::remove_all(outputPath);
  fs::create_directories(outputPath);
  std::atomic_bool shouldCancel{false};
  bool receivedRotateMessage = false;
  const IFilter::MessageHandler messageHandler{[&shouldCancel, &receivedRotateMessage](const IFilter::Message& message) {
    if(message.message.find("Rotating Volume || Copying Data Array") != std::string::npos)
    {
      receivedRotateMessage = true;
      shouldCancel = true;
    }
  }};

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_Cancelled_During_Reorder"), nullptr, messageHandler, shouldCancel);
  REQUIRE(executeResult.result.invalid());
  REQUIRE(receivedRotateMessage);
  REQUIRE_FALSE(fs::exists(outputPath / "OnScale_Cancelled_During_Reorder.flxtbl"));
  REQUIRE(fs::directory_iterator(outputPath) == fs::directory_iterator());
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Non Zero Origin", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({3, 2, 1}, {1, 2, 3, 1, 2, 3}, {"", "Phase1", "Phase2", "Phase3"}, {0.5F, 0.5F, 0.5F}, {1.5F, -2.0F, 0.25F});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "NonZeroOrigin";
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_NonZeroOrigin"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string contents = ReadFile(outputPath / "OnScale_NonZeroOrigin.flxtbl");
  REQUIRE(contents.find("xcrd 4\n1.50000000E+00 2.00000000E+00 2.50000000E+00 3.00000000E+00\n") != std::string::npos);
  REQUIRE(contents.find("ycrd 3\n-2.00000000E+00 -1.50000000E+00 -1.00000000E+00\n") != std::string::npos);
  REQUIRE(contents.find("zcrd 2\n2.50000000E-01 7.50000000E-01\n") != std::string::npos);
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
  RotateFeatureIds(expectedDataStructure, CreateLegacyYRotationMatrix());

  REQUIRE_NOTHROW(expectedDataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& expectedFeatureIds = expectedDataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const std::string contents = ReadFile(outputPath / "OnScale_Reordered.flxtbl");
  REQUIRE(contents.find("xcrd 5\n") != std::string::npos);
  REQUIRE(contents.find("ycrd 4\n") != std::string::npos);
  REQUIRE(contents.find("zcrd 3\n") != std::string::npos);
  REQUIRE(contents.find("xcrd 5\n-8.74227774E-11 3.00000003E-03 6.00000005E-03 8.99999961E-03 1.20000001E-02\n") != std::string::npos);
  REQUIRE(contents.find("ycrd 4\n0.00000000E+00 2.00000009E-03 4.00000019E-03 6.00000005E-03\n") != std::string::npos);
  REQUIRE(contents.find("zcrd 3\n-2.00000056E-03 -1.00000051E-03 -4.65661287E-10\n") != std::string::npos);
  REQUIRE(contents.find("divisions\n4 3 2\n") != std::string::npos);
  REQUIRE(ExtractMatrBlock(contents) == CreateMatrBlock(expectedFeatureIds));
  REQUIRE(ExtractMatrBlock(contents) == "2 8 14 20 4 10 16 22 6 12 18 24 1 7 13 19 3 9 15 21 5 11 17 23");

  RequireFeatureIdMultiset(ExtractMatrBlock(contents), featureIds);
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Reorders Z Then X", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "ReorderedZThenX";
  fs::create_directories(outputPath);
  // The legacy filter left-multiplied each rotation, so Rz followed by Rx gives Rx * Rz.
  const Eigen::Matrix3f compositeRotation = CreateLegacyXRotationMatrix() * CreateLegacyZRotationMatrix();
  RequireReorderedExportMatchesRotation({2, 4, 3}, compositeRotation, outputPath, "OnScale_Reordered_ZX");
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Anisotropic Two Axis Reorder Fails Loudly", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<int32> featureIds(24);
  std::iota(featureIds.begin(), featureIds.end(), 1);
  DataStructure dataStructure = CreateImageDataStructure<int32>({2, 4, 3}, featureIds, CreateSequentialPhaseNames(24), {0.001F, 0.002F, 0.003F});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "AnisotropicTwoAxisReorder";
  fs::remove_all(outputPath);
  fs::create_directories(outputPath);

  const WriteOnScaleTableFileFilter filter;
  auto executeResult = filter.execute(dataStructure, CreateArguments(outputPath, "OnScale_Anisotropic_Two_Axis"));
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().front().code == -12041);
  REQUIRE(executeResult.result.errors().front().message.find("produced dimensions (Z=1, Y=9, X=3)") != std::string::npos);
  REQUIRE(executeResult.result.errors().front().message.find("expected (Z=2, Y=3, X=4)") != std::string::npos);
  REQUIRE_FALSE(fs::exists(outputPath / "OnScale_Anisotropic_Two_Axis.flxtbl"));
}

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Reorders Y Then X", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteOnScaleTableFile" / "ReorderedYThenX";
  fs::create_directories(outputPath);
  // The legacy filter left-multiplied each rotation, so Ry followed by Rx gives Rx * Ry.
  const Eigen::Matrix3f compositeRotation = CreateLegacyXRotationMatrix() * CreateLegacyYRotationMatrix();
  RequireReorderedExportMatchesRotation({3, 2, 4}, compositeRotation, outputPath, "OnScale_Reordered_YX");
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

TEST_CASE("SimplnxCore::WriteOnScaleTableFileFilter: Empty Phase Names Warns During Preflight", "[SimplnxCore][WriteOnScaleTableFileFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateImageDataStructure<int32>({1, 1, 1}, {1}, {});
  const WriteOnScaleTableFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Empty_Phase_Names"));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.warnings().size() == 1);
  REQUIRE(preflightResult.outputActions.warnings().front().code == -12038);
  REQUIRE(preflightResult.outputActions.warnings().front().message == "The phase names StringArray 'Geometry/Cell Ensemble Data/PhaseNames' has 0 tuples. Every name will be written as 'Phase_<id>'.");
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
