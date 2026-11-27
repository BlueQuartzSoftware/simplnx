#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/WriteStlFile.hpp"
#include "SimplnxCore/Filters/CombineStlFilesFilter.hpp"
#include "SimplnxCore/Filters/WriteStlFileFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::constants;
namespace
{
const std::string k_ExemplarDir = fmt::format("{}/6_6_write_stl_file_test", unit_test::k_TestFilesDir);
const DataPath k_ComputedTriangleDataContainerName({"ComputedTriangleDataContainer"});
const DataPath k_ExemplarTriangleDataContainerName({k_TriangleDataContainerName});
const std::string k_PartNumberName = "Part Number";

std::vector<char> ReadIn(const fs::path& filePath)
{
  std::ifstream file(filePath.string(), std::ios_base::binary);

  if(file)
  {
    // get file size
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    // read whole file into a vector
    std::vector<char> contents(length); // act as a buffer
    file.read(contents.data(), length);

    // build string from psuedo-buffer
    return contents;
  }
  return {};
}

void CreatePhasesAndFeaturesFixture(DataStructure& dataStructure)
{
  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, "TriGeom");
  REQUIRE(triangleGeomPtr != nullptr);
  auto* verticesPtr =
      Float32ArrayType::Create(dataStructure, "Vertices", data_store_utilities::CreateDataStore<float32>(dataStructure, DataPath({"TriGeom", "Vertices"}), {5}, {3}), triangleGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  const std::array<float32, 15> vertices = {0, 0, 0, 2, 0, 0.5f, 0, 3, 1, 2, 3, 1.5f, 4, 1, 2};
  for(usize index = 0; index < vertices.size(); index++)
  {
    (*verticesPtr)[index] = vertices[index];
  }
  triangleGeomPtr->setVertices(*verticesPtr);

  auto* facesPtr = UInt64ArrayType::Create(dataStructure, "Faces", data_store_utilities::CreateDataStore<uint64>(dataStructure, DataPath({"TriGeom", "Faces"}), {3}, {3}), triangleGeomPtr->getId());
  REQUIRE(facesPtr != nullptr);
  const std::array<uint64, 9> faces = {0, 1, 2, 1, 3, 2, 1, 4, 3};
  for(usize index = 0; index < faces.size(); index++)
  {
    (*facesPtr)[index] = faces[index];
  }
  triangleGeomPtr->setFaceList(*facesPtr);

  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {3}, triangleGeomPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  triangleGeomPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto* labelsPtr =
      Int32ArrayType::Create(dataStructure, "FaceLabels", data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"TriGeom", "FaceData", "FaceLabels"}), {3}, {2}), faceDataPtr->getId());
  auto* phasesPtr =
      Int32ArrayType::Create(dataStructure, "FacePhases", data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"TriGeom", "FaceData", "FacePhases"}), {3}, {2}), faceDataPtr->getId());
  REQUIRE(labelsPtr != nullptr);
  REQUIRE(phasesPtr != nullptr);
  const std::array<int32, 6> labels = {1, 2, 3, 2, 3, -1};
  const std::array<int32, 6> phases = {5, 6, 7, 6, 7, 0};
  for(usize index = 0; index < labels.size(); index++)
  {
    (*labelsPtr)[index] = labels[index];
    (*phasesPtr)[index] = phases[index];
  }
}

struct StlTriangle
{
  std::array<float32, 3> normal = {};
  std::array<float32, 9> vertices = {};
  uint16 attribute = 0;
};

struct BinaryStl
{
  std::string header;
  int32 triangle_count = 0;
  std::vector<StlTriangle> triangles;
};

// Copy each field separately: STL records have no padding and can be unaligned.
BinaryStl ReadBinaryStl(const fs::path& path)
{
  CAPTURE(path);
  const auto bytes = ReadIn(path);
  REQUIRE(bytes.size() >= 84);
  BinaryStl stl;
  stl.header.assign(bytes.begin(), std::find(bytes.begin(), bytes.begin() + 80, '\0'));
  std::memcpy(&stl.triangle_count, bytes.data() + 80, sizeof(int32));
  REQUIRE(stl.triangle_count >= 0);
  REQUIRE(bytes.size() == 84 + 50 * static_cast<usize>(stl.triangle_count));
  for(usize triangleIdx = 0; triangleIdx < static_cast<usize>(stl.triangle_count); triangleIdx++)
  {
    const usize offset = 84 + 50 * triangleIdx;
    StlTriangle triangle;
    std::memcpy(triangle.normal.data(), bytes.data() + offset, 3 * sizeof(float32));
    std::memcpy(triangle.vertices.data(), bytes.data() + offset + 12, 9 * sizeof(float32));
    std::memcpy(&triangle.attribute, bytes.data() + offset + 48, sizeof(uint16));
    stl.triangles.push_back(triangle);
  }
  return stl;
}

// Hand-derived cross products: t0 and t1 = (-1.5, -2, 6), t2 = (-3.5, -2, 6).
// Side 1 swaps the last two vertices and negates the unit normal.
const StlTriangle k_T0Forward{.normal = {-3.0f / 13, -4.0f / 13, 12.0f / 13}, .vertices = {0, 0, 0, 2, 0, 0.5f, 0, 3, 1}};
const StlTriangle k_T0Reverse{.normal = {3.0f / 13, 4.0f / 13, -12.0f / 13}, .vertices = {0, 0, 0, 0, 3, 1, 2, 0, 0.5f}};
const StlTriangle k_T1Forward{.normal = {-3.0f / 13, -4.0f / 13, 12.0f / 13}, .vertices = {2, 0, 0.5f, 2, 3, 1.5f, 0, 3, 1}};
const StlTriangle k_T1Reverse{.normal = {3.0f / 13, 4.0f / 13, -12.0f / 13}, .vertices = {2, 0, 0.5f, 0, 3, 1, 2, 3, 1.5f}};
const StlTriangle k_T2Forward{.normal = {-7.0f / std::sqrt(209.0f), -4.0f / std::sqrt(209.0f), 12.0f / std::sqrt(209.0f)}, .vertices = {2, 0, 0.5f, 4, 1, 2, 2, 3, 1.5f}};
const StlTriangle k_T2Reverse{.normal = {7.0f / std::sqrt(209.0f), 4.0f / std::sqrt(209.0f), -12.0f / std::sqrt(209.0f)}, .vertices = {2, 0, 0.5f, 2, 3, 1.5f, 4, 1, 2}};

void RequireStl(const fs::path& path, const std::string& header, const std::vector<StlTriangle>& expectedTriangles)
{
  CAPTURE(path);
  REQUIRE(fs::file_size(path) == 84 + 50 * expectedTriangles.size());
  const auto stl = ReadBinaryStl(path);
  REQUIRE(stl.header == header);
  REQUIRE(stl.triangle_count == static_cast<int32>(expectedTriangles.size()));
  for(usize triangleIdx = 0; triangleIdx < expectedTriangles.size(); triangleIdx++)
  {
    CAPTURE(triangleIdx);
    const auto& actual = stl.triangles[triangleIdx];
    const auto& expected = expectedTriangles[triangleIdx];
    for(usize compIdx = 0; compIdx < actual.vertices.size(); compIdx++)
    {
      REQUIRE(actual.vertices[compIdx] == expected.vertices[compIdx]);
    }
    for(usize compIdx = 0; compIdx < actual.normal.size(); compIdx++)
    {
      REQUIRE(actual.normal[compIdx] == Approx(expected.normal[compIdx]).margin(1e-6));
    }
    REQUIRE(actual.attribute == 0);
  }
}

void RequireFileNames(const fs::path& directory, const std::set<std::string>& expected)
{
  std::set<std::string> actual;
  for(const auto& entry : fs::directory_iterator(directory))
  {
    REQUIRE(entry.is_regular_file());
    actual.insert(entry.path().filename().string());
  }
  REQUIRE(actual == expected);
}

fs::path FreshPhasesAndFeaturesDirectory(const std::string& name)
{
  const fs::path directory = fs::path(unit_test::k_BinaryTestOutputDir.view()) / name;
  fs::remove_all(directory);
  fs::create_directories(directory);
  return directory;
}

Arguments PhasesAndFeaturesArguments(const fs::path& directory)
{
  Arguments args;
  args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(directory));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("PF_"));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath({"TriGeom"})));
  args.insertOrAssign(WriteStlFileFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"TriGeom", "FaceData", "FaceLabels"})));
  args.insertOrAssign(WriteStlFileFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"TriGeom", "FaceData", "FacePhases"})));
  return args;
}

void CompareMultipleResults() // compare hash of both file strings
{
  fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/TriangleFeature_0.stl");
  REQUIRE(fs::exists(writtenFilePath));
  fs::path exemplarFilePath = fs::path(k_ExemplarDir + "/ExemplarFeature_0.stl");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(ReadIn(writtenFilePath) == ReadIn(exemplarFilePath));
  fs::path writtenFilePath2 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/TriangleFeature_1.stl");
  REQUIRE(fs::exists(writtenFilePath2));
  fs::path exemplarFilePath2 = fs::path(k_ExemplarDir + "/ExemplarFeature_1.stl");
  REQUIRE(fs::exists(exemplarFilePath2));
  REQUIRE(ReadIn(writtenFilePath2) == ReadIn(exemplarFilePath2));
}

void CompareSingleResult() // compare hash of both file strings
{
  fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Generated.stl");
  REQUIRE(fs::exists(writtenFilePath));
  fs::path exemplarFilePath = fs::path(k_ExemplarDir + "/Exemplar.stl");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(ReadIn(writtenFilePath) == ReadIn(exemplarFilePath));
}

void CompareDirectories(const fs::path& exemplarDirPath, const fs::path& computedDirPath)
{
  REQUIRE(fs::directory_iterator(exemplarDirPath)->exists());
  REQUIRE(fs::directory_iterator(computedDirPath)->exists());

  usize exemplarEntryCount = 0;
  for(const auto& exemplarDirectoryEntry : fs::directory_iterator(exemplarDirPath))
  {
    exemplarEntryCount++;

    bool found = false;
    const fs::path& targetPath = exemplarDirectoryEntry.path();
    auto exemplarFileContents = ReadIn(targetPath);
    std::string exemplarMD5Hash = nx::core::unit_test::ComputeMD5Hash(exemplarFileContents);

    for(const auto& computedDirectoryEntry : fs::directory_iterator(computedDirPath))
    {
      const fs::path& currentPath = computedDirectoryEntry.path();
      if(currentPath.filename() == targetPath.filename())
      {
        found = true;

        auto computedFileContents = ReadIn(currentPath);
        std::string computedMD5Hash = nx::core::unit_test::ComputeMD5Hash(computedFileContents);

        REQUIRE(computedMD5Hash == exemplarMD5Hash);

        break;
      }
    }

    REQUIRE(found);
  }

  usize computedEntryCount = 0;
  for(const auto& computedDirectoryEntry : fs::directory_iterator(computedDirPath))
  {
    computedEntryCount++;
  }

  REQUIRE(computedEntryCount == exemplarEntryCount);
}
} // namespace

TEST_CASE("SimplnxCore::WriteStlFileFilter: Multiple File Valid", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteStlFileFilter filter;
  auto exemplarFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("Triangle"));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath({"TriangleDataContainer"})));
  args.insertOrAssign(WriteStlFileFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"TriangleDataContainer", "FaceData", "FaceLabels"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareMultipleResults();

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Single File Valid", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteStlFileFilter filter;
  auto exemplarFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Generated.stl")));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath({"TriangleDataContainer"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareSingleResult();

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter:Part_Number", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  const nx::core::unit_test::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_TestFilesDir, "6_6_combine_stl_files_v2.tar.gz", "6_6_combine_stl_files.dream3d");
  DataStructure dataStructure;

  {
    CombineStlFilesFilter filter;
    Arguments args;
    std::string inputStlDir = fmt::format("{}/6_6_combine_stl_files_v2/STL_Models", unit_test::k_TestFilesDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
    args.insertOrAssign(CombineStlFilesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
    args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
    args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>("Face Normals"));
    args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));
    args.insertOrAssign(CombineStlFilesFilter::k_CreatePartNumbers_Key, std::make_any<bool>(true));
    args.insertOrAssign(CombineStlFilesFilter::k_PartNumbersName_Key, std::make_any<std::string>(k_PartNumberName));
    args.insertOrAssign(CombineStlFilesFilter::k_LabelVertices_Key, std::make_any<bool>(true));
    args.insertOrAssign(CombineStlFilesFilter::k_VertexLabelName_Key, std::make_any<std::string>(k_PartNumberName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    WriteStlFileFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(3));
    args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
    args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("Part_Number_"));
    args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
    args.insertOrAssign(WriteStlFileFilter::k_PartNumberPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName.createChildPath(k_FaceData).createChildPath(k_PartNumberName)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Part_Number_1.stl");
  REQUIRE(fs::exists(writtenFilePath));
  auto fileContents = ReadIn(writtenFilePath);
  std::string md5Hash = nx::core::unit_test::ComputeMD5Hash(fileContents);
  REQUIRE(md5Hash == "a0383b898d0668d70f08e135e5064efb");

  writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Part_Number_2.stl");
  REQUIRE(fs::exists(writtenFilePath));
  fileContents = ReadIn(writtenFilePath);
  md5Hash = nx::core::unit_test::ComputeMD5Hash(fileContents);
  REQUIRE(md5Hash == "d45a0d99495df506384fdbbb46a79f5c");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Overflow Single File Valid", "[SimplnxCore][WriteStlFileFilter]")
{
  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  auto dataFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(dataFilePath);

  const fs::path firstFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/overflow/single/Generated.stl");
  const fs::path computedDirPath = firstFilePath.parent_path();

  WriteStlFileInputValues inputValues;

  inputValues.grouping_type = static_cast<ChoicesParameter::ValueType>(2);
  inputValues.output_stl_file = firstFilePath;
  inputValues.triangle_geom_path = DataPath({"TriangleDataContainer"});

  inputValues.hidden_max_triangles_per_file = static_cast<usize>(10);

  REQUIRE(WriteStlFile(dataStructure, IFilter::MessageHandler{}, std::atomic_bool{false}, &inputValues)().valid());

  // validation check
  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "write_stl_overflow_test.tar.gz", "write_stl_overflow_test");

  const fs::path exemplarDirPath = fs::path(std::string(nx::core::unit_test::k_TestFilesDir) + "/write_stl_overflow_test/single");

  ::CompareDirectories(exemplarDirPath, computedDirPath);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Overflow Multiple File Valid", "[SimplnxCore][WriteStlFileFilter]")
{
  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  // Instantiate a DataStructure object and an Arguments Object
  auto exemplarFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  const fs::path computedDirPath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/overflow/multiple");

  WriteStlFileInputValues inputValues;

  inputValues.grouping_type = static_cast<ChoicesParameter::ValueType>(0);
  inputValues.output_stl_directory = computedDirPath;
  inputValues.output_stl_prefix = "Triangle";
  inputValues.triangle_geom_path = DataPath({"TriangleDataContainer"});
  inputValues.feature_ids_path = DataPath({"TriangleDataContainer", "FaceData", "FaceLabels"});

  inputValues.hidden_max_triangles_per_file = static_cast<usize>(10);

  REQUIRE(WriteStlFile(dataStructure, IFilter::MessageHandler{}, std::atomic_bool{false}, &inputValues)().valid());

  // validation check
  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "write_stl_overflow_test.tar.gz", "write_stl_overflow_test");

  const fs::path exemplarDirPath = fs::path(std::string(nx::core::unit_test::k_TestFilesDir) + "/write_stl_overflow_test/multiple");

  ::CompareDirectories(exemplarDirPath, computedDirPath);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Phases and Features writes one file per feature named by phase", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreatePhasesAndFeaturesFixture(dataStructure);
  const auto directory = FreshPhasesAndFeaturesDirectory("write_stl_phases_features");
  const auto args = PhasesAndFeaturesArguments(directory);
  WriteStlFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  RequireFileNames(directory, {"PF_Ensemble_5_Feature_1.stl", "PF_Ensemble_6_Feature_2.stl", "PF_Ensemble_7_Feature_3.stl", "PF_Ensemble_0_Feature_-1.stl"});
  RequireStl(directory / "PF_Ensemble_5_Feature_1.stl", "DREAM3D Generated For Feature ID 1 Phase 5", {k_T0Forward});
  RequireStl(directory / "PF_Ensemble_6_Feature_2.stl", "DREAM3D Generated For Feature ID 2 Phase 6", {k_T0Reverse, k_T1Reverse});
  RequireStl(directory / "PF_Ensemble_7_Feature_3.stl", "DREAM3D Generated For Feature ID 3 Phase 7", {k_T1Forward, k_T2Forward});
  RequireStl(directory / "PF_Ensemble_0_Feature_-1.stl", "DREAM3D Generated For Feature ID -1 Phase 0", {k_T2Reverse});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Phases and Features rejects a 1-component phases array", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreatePhasesAndFeaturesFixture(dataStructure);
  const auto directory = FreshPhasesAndFeaturesDirectory("write_stl_phases_features_one_component");
  auto args = PhasesAndFeaturesArguments(directory);
  auto* faceDataPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"TriGeom", "FaceData"}));
  REQUIRE(faceDataPtr != nullptr);
  const DataPath phasesPath({"TriGeom", "FaceData", "FacePhases1"});
  auto* phasesPtr = Int32ArrayType::Create(dataStructure, "FacePhases1", data_store_utilities::CreateDataStore<int32>(dataStructure, phasesPath, {3}, {1}), faceDataPtr->getId());
  REQUIRE(phasesPtr != nullptr);
  phasesPtr->fill(5);
  args.insertOrAssign(WriteStlFileFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(phasesPath));

  WriteStlFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Phases and Features requires one tuple per face", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();
  const std::array<std::string, 3> cases = {"phases with 2 tuples", "phases with 4 tuples", "labels with 2 tuples"};
  for(usize caseIdx = 0; caseIdx < cases.size(); caseIdx++)
  {
    DYNAMIC_SECTION(cases[caseIdx])
    {
      DataStructure dataStructure;
      CreatePhasesAndFeaturesFixture(dataStructure);
      const auto directory = FreshPhasesAndFeaturesDirectory("write_stl_phases_features_tuple_count_" + std::to_string(caseIdx));
      auto args = PhasesAndFeaturesArguments(directory);
      const bool hasShortLabels = caseIdx == 2;
      const usize tupleCount = caseIdx == 1 ? 4 : 2;
      const std::string matrixName = caseIdx == 1 ? "Long" : "Short";
      auto* triangleGeomPtr = dataStructure.getDataAs<TriangleGeom>(DataPath({"TriGeom"}));
      REQUIRE(triangleGeomPtr != nullptr);
      auto* matrixPtr = AttributeMatrix::Create(dataStructure, matrixName, {tupleCount}, triangleGeomPtr->getId());
      REQUIRE(matrixPtr != nullptr);
      const std::string arrayName = hasShortLabels ? "FaceLabels" : "FacePhases";
      const DataPath arrayPath({"TriGeom", matrixName, arrayName});
      auto* arrayPtr = Int32ArrayType::Create(dataStructure, arrayName, data_store_utilities::CreateDataStore<int32>(dataStructure, arrayPath, {tupleCount}, {2}), matrixPtr->getId());
      REQUIRE(arrayPtr != nullptr);
      arrayPtr->fill(1);
      args.insertOrAssign(hasShortLabels ? WriteStlFileFilter::k_FeatureIdsPath_Key : WriteStlFileFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(arrayPath));

      // Preflight must reject the mismatch before execution can access a missing tuple.
      WriteStlFileFilter filter;
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
      REQUIRE_FALSE(preflightResult.output_actions.errors().empty());
      REQUIRE(preflightResult.output_actions.errors().front().code == -27888);
      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Part Number requires one tuple per face", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreatePhasesAndFeaturesFixture(dataStructure);
  const auto directory = FreshPhasesAndFeaturesDirectory("write_stl_part_number_tuple_count");
  auto args = PhasesAndFeaturesArguments(directory);
  args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(3));

  auto* triangleGeomPtr = dataStructure.getDataAs<TriangleGeom>(DataPath({"TriGeom"}));
  REQUIRE(triangleGeomPtr != nullptr);
  auto* shortPtr = AttributeMatrix::Create(dataStructure, "Short", {2}, triangleGeomPtr->getId());
  REQUIRE(shortPtr != nullptr);
  const DataPath parts2Path({"TriGeom", "Short", "Parts2"});
  auto* parts2Ptr = Int32ArrayType::Create(dataStructure, "Parts2", data_store_utilities::CreateDataStore<int32>(dataStructure, parts2Path, {2}, {1}), shortPtr->getId());
  REQUIRE(parts2Ptr != nullptr);
  parts2Ptr->fill(1);

  auto* faceDataPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"TriGeom", "FaceData"}));
  REQUIRE(faceDataPtr != nullptr);
  const DataPath parts3Path({"TriGeom", "FaceData", "Parts3"});
  auto* parts3Ptr = Int32ArrayType::Create(dataStructure, "Parts3", data_store_utilities::CreateDataStore<int32>(dataStructure, parts3Path, {3}, {1}), faceDataPtr->getId());
  REQUIRE(parts3Ptr != nullptr);
  (*parts3Ptr)[0] = 1;
  (*parts3Ptr)[1] = 2;
  (*parts3Ptr)[2] = 1;

  WriteStlFileFilter filter;
  DYNAMIC_SECTION("mismatched")
  {
    args.insertOrAssign(WriteStlFileFilter::k_PartNumberPath_Key, std::make_any<DataPath>(parts2Path));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
    REQUIRE_FALSE(preflightResult.output_actions.errors().empty());
    REQUIRE(preflightResult.output_actions.errors().front().code == -27888);
  }
  DYNAMIC_SECTION("matched")
  {
    args.insertOrAssign(WriteStlFileFilter::k_PartNumberPath_Key, std::make_any<DataPath>(parts3Path));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Phases and Features splits groups into overflow files", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreatePhasesAndFeaturesFixture(dataStructure);
  const auto directory = FreshPhasesAndFeaturesDirectory("write_stl_phases_features_overflow");
  WriteStlFileInputValues inputValues{};
  inputValues.grouping_type = 1;
  inputValues.output_stl_directory = directory;
  inputValues.output_stl_prefix = "PF_";
  inputValues.triangle_geom_path = DataPath({"TriGeom"});
  inputValues.feature_ids_path = DataPath({"TriGeom", "FaceData", "FaceLabels"});
  inputValues.feature_phases_path = DataPath({"TriGeom", "FaceData", "FacePhases"});
  inputValues.hidden_max_triangles_per_file = 1;
  auto result = WriteStlFile(dataStructure, IFilter::MessageHandler{}, std::atomic_bool{false}, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  RequireFileNames(directory, {"PF_Ensemble_5_Feature_1.stl", "PF_Ensemble_6_Feature_2.stl", "PF_Ensemble_6_Feature_2_overflow_1.stl", "PF_Ensemble_7_Feature_3.stl",
                               "PF_Ensemble_7_Feature_3_overflow_1.stl", "PF_Ensemble_0_Feature_-1.stl"});
  RequireStl(directory / "PF_Ensemble_5_Feature_1.stl", "DREAM3D Generated For Feature ID 1 Phase 5", {k_T0Forward});
  RequireStl(directory / "PF_Ensemble_6_Feature_2.stl", "DREAM3D Generated For Feature ID 2 Phase 6", {k_T0Reverse});
  RequireStl(directory / "PF_Ensemble_6_Feature_2_overflow_1.stl", "DREAM3D Generated For Feature ID 2 Phase 6", {k_T1Reverse});
  RequireStl(directory / "PF_Ensemble_7_Feature_3.stl", "DREAM3D Generated For Feature ID 3 Phase 7", {k_T1Forward});
  RequireStl(directory / "PF_Ensemble_7_Feature_3_overflow_1.stl", "DREAM3D Generated For Feature ID 3 Phase 7", {k_T2Forward});
  RequireStl(directory / "PF_Ensemble_0_Feature_-1.stl", "DREAM3D Generated For Feature ID -1 Phase 0", {k_T2Reverse});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Phases and Features names a feature by the phase of its first triangle", "[SimplnxCore][WriteStlFileFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreatePhasesAndFeaturesFixture(dataStructure);
  auto* phasesPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({"TriGeom", "FaceData", "FacePhases"}));
  REQUIRE(phasesPtr != nullptr);
  // This pins the first-occurrence rule for inconsistent input: feature 3 first occurs at t1, side 0.
  (*phasesPtr)[2] = 8;
  const auto directory = FreshPhasesAndFeaturesDirectory("write_stl_phases_features_first_occurrence");
  const auto args = PhasesAndFeaturesArguments(directory);
  WriteStlFileFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  RequireFileNames(directory, {"PF_Ensemble_5_Feature_1.stl", "PF_Ensemble_6_Feature_2.stl", "PF_Ensemble_8_Feature_3.stl", "PF_Ensemble_0_Feature_-1.stl"});
  RequireStl(directory / "PF_Ensemble_8_Feature_3.stl", "DREAM3D Generated For Feature ID 3 Phase 8", {k_T1Forward, k_T2Forward});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteStlFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteStlFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteStlFileFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<WriteStlFileFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteStlFileFilter::k_OutputStlDirectory_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<std::string>(WriteStlFileFilter::k_OutputStlPrefix_Key) == "TestName");
      CHECK(args.value<DataPath>(WriteStlFileFilter::k_TriangleGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteStlFileFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
