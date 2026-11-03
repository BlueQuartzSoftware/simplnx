#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CombineStlFilesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const DataPath k_ComputedTriangleDataContainerName({"ComputedTriangleDataContainer"});
const DataPath k_ExemplarTriangleDataContainerName({k_TriangleDataContainerName});
} // namespace

TEST_CASE("SimplnxCore::CombineStlFilesFilter: Valid Filter Execution", "[SimplnxCore][CombineStlFilesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_combine_stl_files_v2.tar.gz", "6_6_combine_stl_files.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_combine_stl_files_v2/6_6_combine_stl_files.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  std::string inputStlDir = fmt::format("{}/6_6_combine_stl_files_v2/STL_Models", unit_test::k_TestFilesDir.view());

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CombineStlFilesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>("Face Normals"));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));
  args.insertOrAssign(CombineStlFilesFilter::k_CreatePartNumbers_Key, std::make_any<bool>(true));
  args.insertOrAssign(CombineStlFilesFilter::k_PartNumbersName_Key, std::make_any<std::string>("File Index"));
  args.insertOrAssign(CombineStlFilesFilter::k_LabelVertices_Key, std::make_any<bool>(true));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexLabelName_Key, std::make_any<std::string>("File Index"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const TriangleGeom& computedTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ComputedTriangleDataContainerName);
  const TriangleGeom& exemplarTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_ExemplarTriangleDataContainerName);
  REQUIRE(computedTriangleGeom.getNumberOfFaces() == exemplarTriangleGeom.getNumberOfFaces());
  REQUIRE(computedTriangleGeom.getNumberOfVertices() == exemplarTriangleGeom.getNumberOfVertices());
  REQUIRE(computedTriangleGeom.getFaceAttributeMatrix()->getShape() == exemplarTriangleGeom.getFaceAttributeMatrix()->getShape());

  UnitTest::CompareArrays<float64>(dataStructure, k_ExemplarTriangleDataContainerName.createChildPath(k_FaceData).createChildPath(k_FaceNormals),
                                   k_ComputedTriangleDataContainerName.createChildPath(k_FaceData).createChildPath("Face Normals"));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CombineStlFilesFilter: InValid Filter Execution")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CombineStlFilesFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create invalid Parameters for the filter : no stl files in directory
  std::string inputStlDir = fmt::format("{}/Data/Input", unit_test::k_DREAM3DDataDir.view());

  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>(k_FaceNormals));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CombineStlFilesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CombineStlFilesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CombineStlFilesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CombineStlFilesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(CombineStlFilesFilter::k_StlFilesPath_Key) == fs::path("/test/path/stl_files"));
      CHECK(args.value<DataPath>(CombineStlFilesFilter::k_TriangleGeometryPath_Key) == DataPath({"TriangleDataContainer"}));
      CHECK(args.value<std::string>(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key) == "FaceData");
      CHECK(args.value<std::string>(CombineStlFilesFilter::k_FaceNormalsArrayName_Key) == "FaceNormals");
    }
  }
}

TEST_CASE("SimplnxCore::CombineStlFilesFilter: Part numbers match file list", "[SimplnxCore][CombineStlFilesFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path outputDirectory(unit_test::k_BinaryTestOutputDir.view());
  fs::create_directories(outputDirectory);
  const fs::path directory = outputDirectory / ("CombineStlFiles_L08_" + Uuid::GenerateV4().str());
  // Only clean up a directory this invocation successfully created.
  REQUIRE(fs::create_directory(directory));
  auto cleanup = MakeScopeGuard([&directory]() noexcept {
    std::error_code error;
    fs::remove_all(directory, error);
  });

  // Create in the opposite order to the sorted file list. Each file has one hand-written triangle.
  const std::array<std::string, 2> names = {"part.stl", "part-2.stl"};
  const std::array<float32, 2> origins = {10.0f, 20.0f};
  for(usize file = 0; file < names.size(); file++)
  {
    std::ofstream stream(directory / names[file], std::ios::binary);
    REQUIRE(stream.is_open());
    const std::array<char, 80> header = {};
    stream.write(header.data(), header.size());
    // STL uses little-endian IEEE floats. Emit the bytes explicitly for portability.
    const std::array<char, 4> triangleCount = {1, 0, 0, 0};
    stream.write(triangleCount.data(), triangleCount.size());
    const std::array<float32, 12> values = {0, 0, 1, origins[file], 0, 0, origins[file] + 1, 0, 0, origins[file], 1, 0};
    for(float32 value : values)
    {
      uint32 bits = 0;
      std::memcpy(&bits, &value, sizeof(bits));
      for(usize byte = 0; byte < 4; byte++)
      {
        stream.put(static_cast<char>((bits >> (8 * byte)) & 0xff));
      }
    }
    stream.put(0);
    stream.put(0);
    stream.close();
    REQUIRE(stream.good());
  }

  CombineStlFilesFilter filter;
  DataStructure dataStructure;
  Arguments args;
  const DataPath geometryPath({"Combined"});
  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<fs::path>(directory));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(CombineStlFilesFilter::k_CreatePartNumbers_Key, std::make_any<bool>(true));
  args.insertOrAssign(CombineStlFilesFilter::k_PartNumbersName_Key, std::make_any<std::string>("Parts"));
  args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>("Faces"));
  args.insertOrAssign(CombineStlFilesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("Files"));
  args.insertOrAssign(CombineStlFilesFilter::k_FileListName_Key, std::make_any<std::string>("Paths"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(geometryPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(geometryPath.createChildPath("Faces").createChildPath("Parts")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(geometryPath.createChildPath("Files").createChildPath("Paths")));
  const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(geometryPath);
  const auto& parts = dataStructure.getDataRefAs<Int32Array>(geometryPath.createChildPath("Faces").createChildPath("Parts"));
  const auto& paths = dataStructure.getDataRefAs<StringArray>(geometryPath.createChildPath("Files").createChildPath("Paths"));
  REQUIRE(geometry.getNumberOfFaces() == 2);
  REQUIRE(geometry.getNumberOfVertices() == 6);
  REQUIRE(paths.getNumberOfTuples() == 3);
  CHECK(paths[0].empty());
  CHECK(paths[1] == (directory / "part-2.stl").string());
  CHECK(paths[2] == (directory / "part.stl").string());
  const auto& faces = geometry.getFacesRef();
  const auto& vertices = geometry.getVerticesRef();
  for(usize part = 1; part <= 2; part++)
  {
    const float32 origin = fs::path(paths[part]).filename() == "part.stl" ? 10.0f : 20.0f;
    const std::array<float32, 9> expected = {origin, 0, 0, origin + 1, 0, 0, origin, 1, 0};
    usize count = 0;
    for(usize face = 0; face < geometry.getNumberOfFaces(); face++)
    {
      REQUIRE(parts[face] >= 1);
      REQUIRE(parts[face] <= 2);
      if(parts[face] != static_cast<int32>(part))
      {
        continue;
      }
      count++;
      for(usize corner = 0; corner < 3; corner++)
      {
        const usize vertex = faces[3 * face + corner];
        REQUIRE(vertex < geometry.getNumberOfVertices());
        for(usize component = 0; component < 3; component++)
        {
          INFO("Part " << part << " file " << paths[part]);
          CHECK(vertices[3 * vertex + component] == expected[3 * corner + component]);
        }
      }
    }
    CHECK(count == 1);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
