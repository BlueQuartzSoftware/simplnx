#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/SharedFeatureFaceFilter.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;

namespace SharedFeatureFaceFilterTest
{
const std::string k_TriangleGeometryName = "TriangleDataContainer";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_FaceFeatureName = "FaceFeatureData";
const std::string k_FaceDataName = "FaceData";
const std::string k_GrainBoundaryName = "GrainBoundaryData";

const std::string k_FeatureFaceIdsArrayName = "SharedFeatureFaceId [NX Computed]";
const std::string k_GrainBoundaryAttributeMatrixName = "SharedFeatureFace [NX Computed]";
const std::string k_FeatureNumTrianglesArrayName = "NumTriangles [NX Computed]";
const std::string k_FeatureFaceLabelsArrayName = "FaceLabels [NX Computed]";

const DataPath k_GeometryPath = DataPath({k_TriangleGeometryName});
const DataPath k_FeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceDataName).createChildPath(k_FaceLabelsName);

} // namespace SharedFeatureFaceFilterTest

using namespace SharedFeatureFaceFilterTest;

TEST_CASE("ComplexCore::SharedFeatureFaceFilter", "[ComplexCore][SharedFeatureFaceFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "12_IN625_GBCD.tar.gz", "12_IN625_GBCD");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    SharedFeatureFaceFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SharedFeatureFaceFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(SharedFeatureFaceFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    // Output Variables
    args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceIdsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureFaceIdsArrayName));
    args.insertOrAssign(SharedFeatureFaceFilter::k_GrainBoundaryAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_GrainBoundaryAttributeMatrixName));
    args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureNumTrianglesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureNumTrianglesArrayName));
    args.insertOrAssign(SharedFeatureFaceFilter::k_FeatureFaceLabelsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureFaceLabelsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the results
  {
    const std::string kExemplarArrayName = "FeatureFaceId";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_FaceDataName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_FaceDataName).createChildPath(k_FeatureFaceIdsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }
  {
    const std::string kExemplarArrayName = "FaceLabels";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryAttributeMatrixName).createChildPath(k_FeatureFaceLabelsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }
  {
    const std::string kExemplarArrayName = "NumTriangles";
    const DataPath kExemplarArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryName).createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_GeometryPath.createChildPath(k_GrainBoundaryAttributeMatrixName).createChildPath(k_FeatureNumTrianglesArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    UnitTest::CompareDataArrays<int32>(kExemplarsArray, kNxArray);
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/shared_feature_faces.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
