#include "SimplnxCore/Filters/RemoveFlaggedEdgesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_elements_data/remove_flagged_edges_data.dream3d", nx::core::unit_test::k_TestFilesDir));

const std::string k_RegionIdsName = "Region Ids";
const std::string k_SliceIdsName = "Slice Ids";
const std::string k_VertexListName = "SharedVertexList";
const std::string k_EdgeListName = "SharedEdgeList";

const DataPath k_EdgeGeomPath({"Edge Geometry"});
const DataPath k_MaskPath = k_EdgeGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(Constants::k_Mask);
const DataPath k_RegionIdsPath = k_EdgeGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(k_RegionIdsName);
const DataPath k_SliceIdsPath = k_EdgeGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(k_SliceIdsName);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});
const DataPath k_ExemplarReducedGeomPath({"ExemplarReducedGeometry"});
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedEdgesFilter: Test Algorithm", "[SimplnxCore][RemoveFlaggedEdgesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "remove_flagged_elements_data.tar.gz", "remove_flagged_elements_data");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedEdgesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_InputEdgeGeometryPath_Key, std::make_any<DataPath>(::k_EdgeGeomPath));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_EdgeDataHandling_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(RemoveFlaggedEdgesFilter::k_EdgeDataSelectedArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{::k_SliceIdsPath, ::k_RegionIdsPath}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(::k_RegionIdsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(::k_RegionIdsName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(::k_SliceIdsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Edge_Data).createChildPath(::k_SliceIdsName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_VertexListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_VertexListName);

    UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_EdgeListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_EdgeListName);

    UnitTest::CompareDataArrays<uint64>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }
}
