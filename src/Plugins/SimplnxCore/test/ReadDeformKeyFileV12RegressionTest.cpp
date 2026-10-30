#include "SimplnxCore/Filters/ReadDeformKeyFileV12Filter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_RegressionGeomPath({"Deform"});
const DataPath k_RegressionVertexPath = k_RegressionGeomPath.createChildPath("Vertices");

// Record t and variable k contain 100*t+k, with one-based t and k.
constexpr std::array<std::array<float32, 12>, 4> k_ExpectedValues = {{{101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112},
                                                                      {201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212},
                                                                      {301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312},
                                                                      {401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412}}};

void WriteDeformFixture(const fs::path& path, usize failedRecord, bool shortRecord)
{
  std::ofstream stream(path);
  REQUIRE(stream.is_open());
  stream << "* User Defined Variables\n*\nUSRDEF 1 12\n";
  for(usize variable = 1; variable <= 12; variable++)
  {
    stream << "U" << variable << "\n";
  }
  stream << "* Data for Object # 1\n*\nRZ 1 4\n"
            "       1 0 0\n       2 2 0\n       3 2 3\n       4 0 3\n"
            "ELMCON 1 1\n       1 1 2 3 4\nUSRNOD 1 4\n";
  for(usize record = 1; record <= 4; record++)
  {
    stream << fmt::format("{:>8}", record);
    const usize variables = record == failedRecord && shortRecord ? 1 : 12;
    for(usize variable = 1; variable <= variables; variable++)
    {
      if(record == failedRecord && variable == 2)
      {
        stream << " invalid";
      }
      else
      {
        stream << " " << 100 * record + variable;
      }
    }
    stream << "\n";
  }
  stream << "* End Section\n";
  stream.flush();
  REQUIRE(stream.good());
}

Arguments DeformArguments(const ReadDeformKeyFileV12Filter& filter, const fs::path& path)
{
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ReadDeformKeyFileV12Filter::k_InputFilePath_Key, std::make_any<fs::path>(path));
  args.insertOrAssign(ReadDeformKeyFileV12Filter::k_QuadGeomPath_Key, std::make_any<DataPath>(k_RegressionGeomPath));
  args.insertOrAssign(ReadDeformKeyFileV12Filter::k_VertexAMName_Key, std::make_any<std::string>("Vertices"));
  args.insertOrAssign(ReadDeformKeyFileV12Filter::k_CellAMName_Key, std::make_any<std::string>("Cells"));
  return args;
}

void CheckDeformValues(const DataStructure& dataStructure, usize failedRecord)
{
  for(usize variable = 0; variable < 12; variable++)
  {
    const auto* values = dataStructure.getDataAs<Float32Array>(k_RegressionVertexPath.createChildPath(fmt::format("U{}", variable + 1)));
    REQUIRE(values != nullptr);
    REQUIRE(values->getNumberOfTuples() == 4);
    REQUIRE(values->getNumberOfComponents() == 1);
    for(usize record = 0; record < 4; record++)
    {
      CAPTURE(variable, record, failedRecord);
      // U1 completes before U2 fails. Every later scalar keeps its initial zero value.
      const bool completed = failedRecord == 0 || record + 1 < failedRecord || (record + 1 == failedRecord && variable == 0);
      CHECK((*values)[record] == (completed ? k_ExpectedValues[record][variable] : 0.0F));
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::ReadDeformKeyFileV12Filter: USRNOD maps every variable and tuple", "[SimplnxCore][ReadDeformKeyFileV12Filter]")
{
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "c01-deform-valid.key";
  WriteDeformFixture(path, 0, false);
  DataStructure dataStructure;
  ReadDeformKeyFileV12Filter filter;
  auto args = DeformArguments(filter, path);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckDeformValues(dataStructure, 0);
  const auto* geometry = dataStructure.getDataAs<QuadGeom>(k_RegressionGeomPath);
  REQUIRE(geometry != nullptr);
  CHECK(geometry->getNumberOfVertices() == 4);
  CHECK(geometry->getNumberOfFaces() == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadDeformKeyFileV12Filter: USRNOD stops at the first failed scalar", "[SimplnxCore][ReadDeformKeyFileV12Filter]")
{
  const usize failedRecord = GENERATE(1, 3);
  const bool shortRecord = GENERATE(false, true);
  CAPTURE(failedRecord, shortRecord);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("c01-deform-failure-{}-{}.key", failedRecord, shortRecord);
  WriteDeformFixture(path, failedRecord, shortRecord);
  DataStructure dataStructure;
  ReadDeformKeyFileV12Filter filter;
  auto args = DeformArguments(filter, path);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == (shortRecord ? -2023 : -2008));
  CHECK(result.result.errors()[0].message.find("U2") != std::string::npos);
  CHECK(result.result.errors()[0].message.find(fmt::format("record {}", failedRecord)) != std::string::npos);
  CheckDeformValues(dataStructure, failedRecord);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
