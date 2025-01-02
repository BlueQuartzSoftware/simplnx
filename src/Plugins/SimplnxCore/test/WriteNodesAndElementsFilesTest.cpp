#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/WriteNodesAndElementsFiles.hpp"
#include "SimplnxCore/Filters/WriteNodesAndElementsFilesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
namespace
{
const std::string k_GeometryName = "Geometry";
const DataPath k_GeometryPath = DataPath({k_GeometryName});
const fs::path k_OutputNodeFilePath = fs::temp_directory_path() / "nodes.node";
const fs::path k_OutputElementFilePath = fs::temp_directory_path() / "elements.ele";

void Cleanup()
{
  if(fs::exists(k_OutputNodeFilePath))
  {
    REQUIRE(fs::remove(k_OutputNodeFilePath));
  }
  if(fs::exists(k_OutputElementFilePath))
  {
    REQUIRE(fs::remove(k_OutputElementFilePath));
  }
}

void CreateVertexGeometry(DataStructure& ds)
{
  VertexGeom* geom = VertexGeom::Create(ds, k_GeometryName);
  auto vertexAttrMatrix = AttributeMatrix::Create(ds, "Vertex Data", {2}, geom->getId());
  geom->setVertexAttributeMatrix(*vertexAttrMatrix);
  Float32Array* vertices = UnitTest::CreateTestDataArray<float32>(ds, "Vertices Store", {2}, {3}, geom->getId());
  std::vector<float32> verticesVec = {1, 1.5, 1.75, 2, 3, 4};
  std::copy(verticesVec.begin(), verticesVec.end(), vertices->begin());
  geom->setVertices(*vertices);
}

void CreateEdgeGeometry(DataStructure& ds)
{
  EdgeGeom* geom = EdgeGeom::Create(ds, k_GeometryName);
  auto edgeAttrMatrix = AttributeMatrix::Create(ds, "Edge Data", {1}, geom->getId());
  geom->setEdgeAttributeMatrix(*edgeAttrMatrix);
  auto vertexAttrMatrix = AttributeMatrix::Create(ds, "Vertex Data", {2}, geom->getId());
  geom->setVertexAttributeMatrix(*vertexAttrMatrix);
  Float32Array* vertices = UnitTest::CreateTestDataArray<float32>(ds, "Vertices Store", {2}, {3}, geom->getId());
  std::vector<float32> verticesVec = {1, 1.5, 1.75, 2, 3, 4};
  std::copy(verticesVec.begin(), verticesVec.end(), vertices->begin());
  geom->setVertices(*vertices);
  DataArray<IGeometry::MeshIndexType>* cells = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(ds, "Cells Store", {1}, {2}, geom->getId());
  std::vector<float32> cellsVec = {0, 1};
  std::copy(cellsVec.begin(), cellsVec.end(), cells->begin());
  geom->setEdgeList(*cells);
}

void ValidateFile(const fs::path& filePath, const std::vector<std::string>& expectedHeader, const std::vector<std::vector<std::string>>& expectedContent)
{
  std::ifstream file(filePath.string());
  REQUIRE(file.is_open());

  auto validateTokens = [&](const std::string& line, const std::vector<std::string>& expectedTokens) {
    auto tokens = StringUtilities::split(line, ' ');
    REQUIRE(tokens == expectedTokens);
  };

  std::string line;

  // Skip the comment line
  std::getline(file, line);

  // Validate header if provided
  if(!expectedHeader.empty())
  {
    std::getline(file, line);
    validateTokens(line, expectedHeader);
  }

  // Validate content lines
  for(const auto& expectedTokens : expectedContent)
  {
    std::getline(file, line);
    validateTokens(line, expectedTokens);
  }

  file.close();
}
} // namespace

TEST_CASE("SimplnxCore::WriteNodesAndElementsFilesFilter: Valid Execution", "[SimplnxCore][WriteNodesAndElementsFilesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteNodesAndElementsFilesFilter filter;

  DataStructure dataStructure;
  Arguments args;

  CreateEdgeGeometry(dataStructure);

  bool writeNodeFile = false;
  bool numberNodes = false;
  bool includeNodeFileHeader = false;
  bool writeElementFile = false;
  bool numberElements = false;
  bool includeElementFileHeader = false;

  SECTION("Node File")
  {
    writeNodeFile = true;

    SECTION("Number Nodes")
    {
      numberNodes = true;
    }
    SECTION("Include File Header")
    {
      includeNodeFileHeader = true;
    }
    SECTION("Both Number Nodes & Include File Header")
    {
      numberNodes = true;
      includeNodeFileHeader = true;
    }
  }
  SECTION("Element File")
  {
    writeElementFile = true;

    SECTION("Number Elements")
    {
      numberElements = true;
    }
    SECTION("Include File Header")
    {
      includeElementFileHeader = true;
    }
    SECTION("Both Number Elements & Include File Header")
    {
      numberElements = true;
      includeElementFileHeader = true;
    }
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_SelectedGeometry, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_WriteNodeFile, std::make_any<bool>(writeNodeFile));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NumberNodes, std::make_any<bool>(numberNodes));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_IncludeNodeFileHeader, std::make_any<bool>(includeNodeFileHeader));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NodeFilePath, std::make_any<fs::path>(k_OutputNodeFilePath));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_WriteElementFile, std::make_any<bool>(writeElementFile));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NumberElements, std::make_any<bool>(numberElements));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_IncludeElementFileHeader, std::make_any<bool>(includeElementFileHeader));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_ElementFilePath, std::make_any<fs::path>(k_OutputElementFilePath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  if(writeNodeFile)
  {
    std::vector<std::string> expectedHeader = includeNodeFileHeader ? std::vector<std::string>{"X", "Y", "Z"} : std::vector<std::string>{};
    std::vector<std::vector<std::string>> expectedContent = {{"1.0000", "1.5000", "1.7500"}, {"2.0000", "3.0000", "4.0000"}};
    if(numberNodes)
    {
      if(includeNodeFileHeader)
      {
        expectedHeader.insert(expectedHeader.begin(), "NODE_NUM");
      }
      expectedContent[0].insert(expectedContent[0].begin(), "0");
      expectedContent[1].insert(expectedContent[1].begin(), "1");
    }
    ValidateFile(k_OutputNodeFilePath, expectedHeader, expectedContent);
  }

  if(writeElementFile)
  {
    std::vector<std::string> expectedHeader = includeElementFileHeader ? std::vector<std::string>{"NUM_VERTS_IN_ELEMENT", "V0_Index", "V1_Index"} : std::vector<std::string>{};
    std::vector<std::vector<std::string>> expectedContent = {{"2", "0", "1"}};
    if(numberElements)
    {
      if(includeElementFileHeader)
      {
        expectedHeader.insert(expectedHeader.begin(), "ELEMENT_NUM");
      }
      expectedContent[0].insert(expectedContent[0].begin(), "0");
    }
    ValidateFile(k_OutputElementFilePath, expectedHeader, expectedContent);
  }

  // Clean up the files
  Cleanup();
}

TEST_CASE("SimplnxCore::WriteNodesAndElementsFilesFilter: Invalid Execution", "[SimplnxCore][WriteNodesAndElementsFilesFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteNodesAndElementsFilesFilter filter;

  DataStructure dataStructure;
  Arguments args;

  int32 code = -1;
  SECTION("No File Writer Chosen")
  {
    CreateEdgeGeometry(dataStructure);
    code = to_underlying(WriteNodesAndElementsFiles::ErrorCodes::NoFileWriterChosen);

    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_WriteNodeFile, std::make_any<bool>(false));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NumberNodes, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_IncludeNodeFileHeader, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_WriteElementFile, std::make_any<bool>(false));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NumberElements, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_IncludeElementFileHeader, std::make_any<bool>(true));
  }
  SECTION("Writing A Node File Using A Vertex Geometry")
  {
    CreateVertexGeometry(dataStructure);
    code = to_underlying(WriteNodesAndElementsFiles::ErrorCodes::VertexGeomHasNoElements);

    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_WriteNodeFile, std::make_any<bool>(false));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NumberNodes, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_IncludeNodeFileHeader, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_WriteElementFile, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NumberElements, std::make_any<bool>(true));
    args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_IncludeElementFileHeader, std::make_any<bool>(true));
  }

  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_SelectedGeometry, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_NodeFilePath, std::make_any<fs::path>(k_OutputNodeFilePath));
  args.insertOrAssign(WriteNodesAndElementsFilesFilter::k_ElementFilePath, std::make_any<fs::path>(k_OutputElementFilePath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);
}
