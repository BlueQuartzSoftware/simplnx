#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/WriteLAMMPSFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path exemplarFilePath = fs::path(fmt::format("{}/write_lammps_test/testing/LAMMPS_Grid.txt", unit_test::k_TestFilesDir));
const fs::path writtenFilePath = fs::path(fmt::format("{}/LAMMPS.txt", unit_test::k_BinaryTestOutputDir));

std::vector<char> readIn(fs::path filePath)
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

void CompareResults() // compare hash of both file strings
{
  REQUIRE(fs::exists(writtenFilePath));
  REQUIRE(fs::exists(exemplarFilePath));
  const std::vector<char> exemplar = readIn(exemplarFilePath);
  const std::vector<char> data = readIn(writtenFilePath);
  for(size_t i = 0; i < 1024; i++)
  {
    if(exemplar[i] != data[i])
    {
      std::cout << "Output difference at byte offset " << i << std::endl;
      REQUIRE(exemplar[i] == data[i]);
      break;
    }
  }
  REQUIRE(exemplar.size() == data.size());
}

const DataPath k_VertexGeom = DataPath({"LAMMPS Geometry"});
const DataPath k_AtomLabelsGeom = k_VertexGeom.createChildPath(Constants::k_Vertex_Data).createChildPath("AtomFeature");
} // namespace

TEST_CASE("SimplnxCore::WriteLAMMPSFileFilter: Valid Filter Execution", "[SimplnxCore][WriteLAMMPSFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "write_lammps_test.tar.gz", "write_lammps_test");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/write_lammps_test/testing/lammps_precursor.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    WriteLAMMPSFileFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(WriteLAMMPSFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(writtenFilePath));
    args.insertOrAssign(WriteLAMMPSFileFilter::k_VertexGeomPath, std::make_any<DataPath>(k_VertexGeom));
    args.insertOrAssign(WriteLAMMPSFileFilter::k_AtomLabelsPath_Key, std::make_any<DataPath>(k_AtomLabelsGeom));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ::CompareResults();

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
