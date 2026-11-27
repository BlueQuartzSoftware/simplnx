#include "SimplnxCore/Filters/RemoveFlaggedEdgesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
fs::path KBaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_elements_data/remove_flagged_edges_data.dream3d", nx::core::unit_test::k_TestFilesDir));

constexpr StringLiteralType k_RegionIdsName = "Region Ids";
constexpr StringLiteralType k_SliceIdsName = "Slice Ids";
constexpr StringLiteralType k_VertexListName = "SharedVertexList";
constexpr StringLiteralType k_EdgeListName = "SharedEdgeList";

const DataPath k_EdgeGeomPath({"Edge Geometry"});
const DataPath k_MaskPath = k_EdgeGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(constants::k_Mask);
const DataPath k_RegionIdsPath = k_EdgeGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(k_RegionIdsName);
const DataPath k_SliceIdsPath = k_EdgeGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(k_SliceIdsName);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});
const DataPath k_ExemplarReducedGeomPath({"ExemplarReducedGeometry"});
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedEdgesFilter: Test Algorithm", "[SimplnxCore][RemoveFlaggedEdgesFilter]")
{
  unit_test::LoadPlugins();

  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "remove_flagged_elements_data.tar.gz", "remove_flagged_elements_data");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = unit_test::LoadDataStructure(KBaseDataFilePath);

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(::k_RegionIdsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(::k_RegionIdsName);

    unit_test::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(::k_SliceIdsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(constants::k_EdgeDataWithSpaces).createChildPath(::k_SliceIdsName);

    unit_test::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_VertexListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_VertexListName);

    unit_test::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_EdgeListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_EdgeListName);

    unit_test::CompareDataArrays<uint64>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
