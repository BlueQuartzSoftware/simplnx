#include "SimplnxCore/Filters/ComputeTriangleGeomVolumesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace ComputeTriangleGeomSizesFilterTest
{
const std::string k_TriangleGeometryName = "TriangleDataContainer";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_FaceFeatureName = "FaceFeatureData";
const std::string k_FaceAttributeMatrixName = "FaceData";
const std::string k_VolumesArrayName = "Volumes [NX Computed]";

const DataPath k_GeometryPath = DataPath({k_TriangleGeometryName});
const DataPath k_FeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(k_FaceLabelsName);

} // namespace ComputeTriangleGeomSizesFilterTest

using namespace ComputeTriangleGeomSizesFilterTest;

TEST_CASE("SimplnxCore::ComputeTriangleGeomSizes", "[SimplnxCore][ComputeTriangleGeomSizes]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "12_IN625_GBCD.tar.gz", "12_IN625_GBCD");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    ComputeTriangleGeomVolumesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeTriangleGeomVolumesFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(ComputeTriangleGeomVolumesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(ComputeTriangleGeomVolumesFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAttributeMatrixPath));
    // Output Path
    args.insertOrAssign(ComputeTriangleGeomVolumesFilter::k_VolumesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_VolumesArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  // Compare the results
  {
    const std::string kExemplarArrayName = "Volumes";
    const DataPath kExemplarArrayPath = k_FeatureAttributeMatrixPath.createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_FeatureAttributeMatrixPath.createChildPath(k_VolumesArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);

    // The corrected version of the filter ensures there are no negative values. instead of
    // uploading a new test file, we can just safely ensure the same applies to the
    // exemplar data.
    auto& exemplarData = dataStructure.getDataRefAs<Float32Array>(kExemplarArrayPath);
    auto& resultStore = exemplarData.getIDataStoreRefAs<AbstractDataStore<float32>>();
    for(auto& value : resultStore)
    {
      if(value < 0)
      {
        value = std::abs(value);
      }
    }
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);
    CompareDataArrays<float32>(kExemplarsArray, kNxArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_triangle_geom_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
