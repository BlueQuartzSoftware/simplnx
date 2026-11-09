#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/CombineStlFilesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <any>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const DataPath k_ComputedTriangleDataContainerName({"ComputedTriangleDataContainer"});
const DataPath k_ExemplarTriangleDataContainerName({k_TriangleDataContainerName});

void writeStlTriangle(const fs::path& path, float32 base)
{
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.is_open());
  const std::array<char, 80> header{};
  const uint32 triangleCount = 1;
  const uint16 attributeCount = 0;
  const std::array<float32, 12> triangle = {0.0F, 0.0F, 1.0F, base, 0.0F, 0.0F, base + 1.0F, 0.0F, 0.0F, base, 1.0F, 0.0F};
  const auto countBytes = std::bit_cast<std::array<char, sizeof(triangleCount)>>(triangleCount);
  const auto triangleBytes = std::bit_cast<std::array<char, sizeof(triangle)>>(triangle);
  const auto attributeBytes = std::bit_cast<std::array<char, sizeof(attributeCount)>>(attributeCount);
  output.write(header.data(), static_cast<std::streamsize>(header.size()));
  output.write(countBytes.data(), static_cast<std::streamsize>(countBytes.size()));
  output.write(triangleBytes.data(), static_cast<std::streamsize>(triangleBytes.size()));
  output.write(attributeBytes.data(), static_cast<std::streamsize>(attributeBytes.size()));
  output.close();
  REQUIRE(output.good());
}

void checkNestedVertexLabels(bool requireDiskStores)
{
  const fs::path inputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("NestedCombineStlFiles-" + Uuid::GenerateV4().str());
  const fs::path firstFile = inputDir / "A.stl";
  const fs::path secondFile = inputDir / "B.stl";
  REQUIRE(fs::create_directories(inputDir));
  const auto removeInputs = MakeScopeGuard([&inputDir, &firstFile, &secondFile]() noexcept {
    std::error_code error;
    fs::remove(firstFile, error);
    fs::remove(secondFile, error);
    fs::remove(inputDir, error);
  });
  writeStlTriangle(firstFile, 0.0F);
  writeStlTriangle(secondFile, 10.0F);

  const DataPath geometryPath({"Group", "TriGeom"});
  const std::string vertexDataName = "TriGeom Vertex Data";
  const std::string labelName = "TriGeom File Index";
  const DataPath labelsPath = geometryPath.createChildPath(vertexDataName).createChildPath(labelName);
  const DataPath decoyPath = DataPath({"TriGeom"}).createChildPath(vertexDataName).createChildPath(labelName);
  DataStructure dataStructure;
  REQUIRE((DataGroup::Create(dataStructure, "Group") != nullptr));
  auto* decoyGroupPtr = DataGroup::Create(dataStructure, "TriGeom");
  REQUIRE((decoyGroupPtr != nullptr));
  auto* decoyMatrixPtr = AttributeMatrix::Create(dataStructure, vertexDataName, {6}, decoyGroupPtr->getId());
  REQUIRE((decoyMatrixPtr != nullptr));
  auto decoyStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, decoyPath, {6}, {1});
  REQUIRE((decoyStore != nullptr));
  auto* decoyLabelsPtr = Int32Array::Create(dataStructure, labelName, decoyStore, decoyMatrixPtr->getId());
  REQUIRE((decoyLabelsPtr != nullptr));
  decoyLabelsPtr->fill(-73);

  const CombineStlFilesFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(inputDir));
  args.insertOrAssign(CombineStlFilesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexDataName));
  args.insertOrAssign(CombineStlFilesFilter::k_VertexLabelName_Key, std::make_any<std::string>(labelName));
  args.insertOrAssign(CombineStlFilesFilter::k_LabelVertices_Key, std::make_any<bool>(true));
  const auto preflightResult = filter.preflight(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) -- The project assertion macro uses a do-while wrapper.
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) -- The project assertion macro uses a do-while wrapper.
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& geometry = UnitTest::GetRequiredData<TriangleGeom>(dataStructure, geometryPath);
  const auto& labels = UnitTest::GetRequiredData<Int32Array>(dataStructure, labelsPath);
  REQUIRE((geometry.getNumberOfFaces() == 2));
  REQUIRE((geometry.getNumberOfVertices() == 6));
  REQUIRE((labels.getNumberOfTuples() == 6));
  REQUIRE((geometry.getVertices() != nullptr));
  REQUIRE((geometry.getFaces() != nullptr));
  const auto& vertices = *geometry.getVertices();
  const auto& faces = *geometry.getFaces();
  if(requireDiskStores)
  {
    UnitTest::RequireDiskBackedStore(labels);
    UnitTest::RequireDiskBackedStore(vertices);
    UnitTest::RequireDiskBackedStore(faces);
    UnitTest::RequireDiskBackedStore(*decoyLabelsPtr);
  }

  // The root object is an intentional decoy group; no root triangle geometry is created.
  CHECK((dataStructure.getDataAs<TriangleGeom>(DataPath({"TriGeom"})) == nullptr));
  CHECK((dataStructure.getDataAs<DataGroup>(DataPath({"TriGeom"})) == decoyGroupPtr));
  for(usize index = 0; index < 6; ++index)
  {
    CAPTURE(index, (*decoyLabelsPtr)[index], labels[index]);
    CHECK(((*decoyLabelsPtr)[index] == -73));
    CHECK((labels[index] == (index < 3 ? 1 : 2)));
  }

  // Follow each oriented face to its three written coordinates. Vertex ordering is free.
  for(usize face = 0; face < 2; ++face)
  {
    const float32 base = face == 0 ? 0.0F : 10.0F;
    const std::array<std::array<float32, 3>, 3> expected = {{{base, 0.0F, 0.0F}, {base + 1.0F, 0.0F, 0.0F}, {base, 1.0F, 0.0F}}};
    for(usize corner = 0; corner < 3; ++corner)
    {
      const usize vertex = faces[(face * 3) + corner];
      REQUIRE((vertex < 6));
      CHECK((labels[vertex] == static_cast<int32>(face + 1)));
      for(usize component = 0; component < 3; ++component)
      {
        CHECK((vertices[(vertex * 3) + component] == expected.at(corner).at(component)));
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::CombineStlFilesFilter: nested geometry keeps vertex label path", "[SimplnxCore][CombineStlFilesFilter][NestedPaths]")
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  checkNestedVertexLabels(false);
}

TEST_CASE("SimplnxCore::CombineStlFilesFilter: nested geometry keeps vertex label path with real OOC stores", "[SimplnxCore][CombineStlFilesFilter][NestedPaths][.OocStoreContract]")
{
  REQUIRE(DataStoreUtilities::GetIOCollection().hasDataStoreCreationFunction("HDF5-OOC"));
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  checkNestedVertexLabels(true);
}

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
