#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

#include "complex_plugins/Utilities/TestUtilities.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindTriangleGeomSizesFilter.hpp"

using namespace complex;
using namespace complex::UnitTest;

namespace FindTriangleGeomSizesFilterTest
{
const std::string k_TriangleGeometryName = "TriangleDataContainer";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_FaceFeatureName = "FaceFeatureData";
const std::string k_FaceDataName = "FaceData";
const std::string k_VolumesArrayName = "Volumes [NX Computed]";

const DataPath k_GeometryPath = DataPath({k_TriangleGeometryName});
const DataPath k_FeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceDataName).createChildPath(k_FaceLabelsName);

} // namespace FindTriangleGeomSizesFilterTest

using namespace FindTriangleGeomSizesFilterTest;

TEST_CASE("Core::FindTriangleGeomSizes", "[Core][FindTriangleGeomSizes]")
{
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/TestFiles/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    FindTriangleGeomSizesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindTriangleGeomSizesFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(FindTriangleGeomSizesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(FindTriangleGeomSizesFilter::k_FeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_FeatureAttributeMatrixPath));
    // Output Path
    args.insertOrAssign(FindTriangleGeomSizesFilter::k_VolumesArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_VolumesArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  // Compare the results
  {
    const std::string kExemplarArrayName = "Volumes";
    const DataPath kExemplarArrayPath = k_FeatureAttributeMatrixPath.createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_FeatureAttributeMatrixPath.createChildPath(k_VolumesArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    CompareDataArrays<float32>(kExemplarsArray, kNxArray);
  }

  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_triangle_geom_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
}
