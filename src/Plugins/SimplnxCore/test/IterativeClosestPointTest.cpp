#include "SimplnxCore/Filters/IterativeClosestPointFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <limits>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::constants;

TEST_CASE("SimplnxCore::IterativeClosestPointFilter: Create Filter", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  unit_test::LoadPlugins();

  IterativeClosestPointFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.output_actions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::IterativeClosestPointFilter: Test Algorithm", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  unit_test::LoadPlugins();

  IterativeClosestPointFilter filter;
  DataStructure dataStructure = unit_test::CreateDataStructure();
  Arguments args;

  DataPath movingVertexPath({constants::k_SmallIN100, constants::k_EbsdScanData, constants::k_VertexGeometry});
  auto* movingVertexGeomPtr = dataStructure.getDataAs<VertexGeom>(movingVertexPath);

  uint64 numIterations = 1;
  bool applyTransformation = true;
  DataPath transformArrayPath({constants::k_SmallIN100, "Transform Array"});

  auto* targetVertexGeomPtr = VertexGeom::Create(dataStructure, "Vertex Geometry 2", movingVertexGeomPtr->getParentIds().front());
  auto* eulerData2Ptr =
      Float32ArrayType::CreateWithStore<DataStore<float>>(dataStructure, "Euler 2", {movingVertexGeomPtr->getVertices()->getNumberOfTuples()}, {3}, targetVertexGeomPtr->getParentIds().front());
  targetVertexGeomPtr->setVertices(*eulerData2Ptr);

  DataPath targetVertexPath({constants::k_SmallIN100, constants::k_EbsdScanData, targetVertexGeomPtr->getName()});

  auto* vertices1Ptr = movingVertexGeomPtr->getVertices();
  auto* vertices2Ptr = targetVertexGeomPtr->getVertices();
  usize numVertices = vertices1Ptr->getSize();
  for(usize i = 0; i < numVertices; i++)
  {
    (*vertices1Ptr)[i] = i;
    (*vertices2Ptr)[i] = i;
  }

  args.insertOrAssign(IterativeClosestPointFilter::k_MovingVertexPath_Key, std::make_any<DataPath>(movingVertexPath));
  args.insertOrAssign(IterativeClosestPointFilter::k_TargetVertexPath_Key, std::make_any<DataPath>(targetVertexPath));
  args.insertOrAssign(IterativeClosestPointFilter::k_NumIterations_Key, std::make_any<uint64>(numIterations));
  args.insertOrAssign(IterativeClosestPointFilter::k_ApplyTransformation_Key, std::make_any<bool>(applyTransformation));
  args.insertOrAssign(IterativeClosestPointFilter::k_TransformArrayPath_Key, std::make_any<DataPath>(transformArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::IterativeClosestPointFilter: SIMPL Backwards Compatibility", "[SimplnxCore][IterativeClosestPointFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "IterativeClosestPointFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<IterativeClosestPointFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<DataPath>(IterativeClosestPointFilter::k_MovingVertexPath_Key) == DataPath({"MovingGeometry"}));
      CHECK(args.value<DataPath>(IterativeClosestPointFilter::k_TargetVertexPath_Key) == DataPath({"TargetGeometry"}));
      CHECK(args.value<uint64>(IterativeClosestPointFilter::k_NumIterations_Key) == 5);
      CHECK(args.value<bool>(IterativeClosestPointFilter::k_ApplyTransformation_Key) == true);
      CHECK(args.value<DataPath>(IterativeClosestPointFilter::k_TransformArrayPath_Key) == DataPath({"TransformArray"}));
    }
  }
}
