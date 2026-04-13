#include "SimplnxCore/Filters/CropVertexGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr uint64 k_TupleCount = 8;
constexpr StringLiteral k_VertexAttributeMatrixName = "VertexData";
const DataPath k_VertexGeomPath{std::vector<std::string>{"VertexGeom"}};
const DataPath k_VertexDataPath = k_VertexGeomPath.createChildPath(k_VertexAttributeMatrixName);
const DataPath k_CroppedGeomPath{std::vector<std::string>{"Cropped VertexGeom"}};
const std::vector<DataPath> targetDataArrays{k_VertexDataPath.createChildPath("DataArray")};

DataStructure createTestData()
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {k_TupleCount}, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "DataArray", {k_TupleCount}, {1}, vertexAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = i;
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::CropVertexGeometryFilter(Instantiate)", "[SimplnxCore][CropVertexGeometryFilter]")
{
  UnitTest::LoadPlugins();

  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometryFilter filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  args.insert(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexAttributeMatrixName));
  args.insert(CropVertexGeometryFilter::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometryFilter::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometryFilter::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropVertexGeometryFilter(Data)", "[SimplnxCore][CropVertexGeometryFilter]")
{
  UnitTest::LoadPlugins();

  static const std::vector<float32> k_MinPos{0, 0, 0};
  static const std::vector<float32> k_MaxPos{5, 6, 7};

  CropVertexGeometryFilter filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  args.insert(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insert(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataPath>(k_CroppedGeomPath));
  args.insert(CropVertexGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexAttributeMatrixName));
  args.insert(CropVertexGeometryFilter::k_MinPos_Key, std::make_any<std::vector<float32>>(k_MinPos));
  args.insert(CropVertexGeometryFilter::k_MaxPos_Key, std::make_any<std::vector<float32>>(k_MaxPos));
  args.insert(CropVertexGeometryFilter::k_TargetArrayPaths_Key, std::make_any<std::vector<DataPath>>(targetDataArrays));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto* croppedGeom = dataStructure.getDataAs<VertexGeom>(k_CroppedGeomPath);
  REQUIRE(croppedGeom != nullptr);

  auto* croppedVertices = croppedGeom->getVertices();
  REQUIRE(croppedVertices != nullptr);

  auto* croppedData = dataStructure.getDataAs<Int32Array>(k_CroppedGeomPath.createChildPath(k_VertexAttributeMatrixName).createChildPath("DataArray"));
  REQUIRE(croppedData != nullptr);

  REQUIRE(croppedData->getNumberOfTuples() == 6);
  REQUIRE(croppedVertices->getNumberOfTuples() == 6);

  auto& croppedDataStore = croppedData->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(croppedDataStore[i] == i);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropVertexGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CropVertexGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CropVertexGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CropVertexGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CropVertexGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatToVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatToVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(CropVertexGeometryFilter::k_SelectedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(CropVertexGeometryFilter::k_CreatedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
