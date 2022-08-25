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

namespace
{
constexpr float64 k_max_difference = 0.0001;
}

class RunDihedralAnglesTest
{
public:
  // ctor
  RunDihedralAnglesTest(DataStructure& dataStructure, std::string fileName)
  : m_DataStructure(dataStructure)
  , m_FileName(fileName)
  {
  }
  // virtual dtor
  ~RunDihedralAnglesTest() = default;

   void execute()
  {
    auto acuteTriangle = *createGeom<TriangleGeom>(dataStructure);
    
  }
private:
  DataStore& m_FileName;
  DataStructure& m_DataStructure;
}

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter[acute triangle]", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  DataStructure dataStructure;
  auto acuteTriangle = createGeom<TriangleGeom>(dataStructure);

  acuteTriangle
  

  }

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter[obtuse triangle]", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "Dihedral_Angles";
  DataStructure dataStructure;
  DataArray<float64> correctAngles;

  DreamFileReadInForTest(dataStructure, correctAngles).execute();

}

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter[overlapping vertex]", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "Dihedral_Angles";
  DataStructure dataStructure;
  DataArray<float64> correctAngles;

  DreamFileReadInForTest(dataStructure, correctAngles).execute();
}

TEST_CASE("ComplexCore::TriangleDihedralAngleFilter[disconnected vertex]", "[ComplexCore][TriangleDihedralAngleFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string dihedralAnglesDataArrayName = "Dihedral_Angles";
  DataStructure dataStructure;
  DataArray<float64> correctAngles;

  DreamFileReadInForTest(dataStructure, correctAngles).execute();
}