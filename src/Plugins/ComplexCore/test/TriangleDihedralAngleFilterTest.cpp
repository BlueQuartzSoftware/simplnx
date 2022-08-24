#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"
#include "ComplexCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "complex/Utilities/DataGroupUtilities.cpp"

#include <filesystem>
#include <iostream>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

class DreamFileReadInForTest
{
public:
  // ctor
  DreamFileReadInForTest(DataStructure& dataStructure, DataArray<float64>& dihedralArray)
  : m_DataStructure(dataStructure)
  , m_DihedralArray(dihedralArray)
  {
  }
  // virtual dtor
  ~DreamFileReadInForTest() = default;

   void execute()
  {
    constexpr StringLiteral k_ImportFileData = "Import_File_Data";

    auto filter = filterList->createFilter(k_ImportDream3dFilterHandle);
    REQUIRE(nullptr != filter);

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/TestFiles/.dream3d", unit_test::k_DREAM3DDataDir)); // verify

    Arguments args;
    args.insertOrAssign(k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(m_DataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(m_DataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    auto allDataPaths = m_DataStructure.getAllDataPaths();

    for(auto path : allDataPaths)
    {
      usize found = path.toString().find("FaceDihedralAngles");
      if (found!=std::string::npos)
      {
        m_DihedralArray = m_DataStructure.getDataRefAs(path);
        return {};
      }
    }

  }
private:
  DataArray<float64>& m_DihedralArray;
  DataStructure& m_DataStructure;
}

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "Dihedral_Angles";
  DataStructure dataStructure;
  DataArray<float64> correctAngles;

  DreamFileReadInForTest(dataStructure, correctAngles).execute();

  

  // DataStructure dataGraph;
  //   TriangleNormalFilter filter;
  //   Arguments args;
  //   std::string triangleNormalsName = "Triangle Normals";

  //   DataPath geometryPath = DataPath({triangleGeometryName});

  //   // Create default Parameters for the filter.
  //   DataPath triangleNormalsDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleNormalsName);
  //   args.insertOrAssign(TriangleNormalFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  //   args.insertOrAssign(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayPath_Key, std::make_any<DataPath>(triangleNormalsDataPath));

  //   // Preflight the filter and check result
  //   auto preflightResult = filter.preflight(dataGraph, args);
  //   REQUIRE(preflightResult.outputActions.valid());

  //   // Execute the filter and check the result
  //   auto executeResult = filter.execute(dataGraph, args);
  //   REQUIRE(executeResult.result.valid());

  //   // Let's compare the normals.
  //   DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});
  //   auto& officialNormals = dataGraph.getDataRefAs<Float64Array>(normalsDataPath);
  //   Float64Array& calculatedNormals = dataGraph.getDataRefAs<Float64Array>(triangleNormalsDataPath);
  //   std::vector<double> offNorms, calcNorms;
  //   for(int64 i = 0; i < officialNormals.getSize(); i++)
  //   {
  //     auto result = fabs(officialNormals[i] - calculatedNormals[i]);
  //     REQUIRE(result < ::k_max_difference);
  //   }
  }
