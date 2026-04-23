#include "SimplnxCore/Filters/MoveDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <set>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr StringLiteral k_Group1Name = "Group1";
constexpr StringLiteral k_Group2Name = "Group2";
constexpr StringLiteral k_Group3Name = "Group3";
constexpr StringLiteral k_Group4Name = "Group4";

const int32 k_TupleDimMismatchWarningCode = -69250;

constexpr StringLiteral k_AM1Name = "Matrix1";
constexpr StringLiteral k_DataArray1Name = "Array1";

DataStructure createDataStructure()
{
  DataStructure data;
  auto group1 = DataGroup::Create(data, k_Group1Name);
  auto group2 = DataGroup::Create(data, k_Group2Name);
  auto group3 = DataGroup::Create(data, k_Group3Name, group2->getId());
  auto group4 = DataGroup::Create(data, k_Group4Name, group2->getId());

  auto am1 = AttributeMatrix::Create(data, k_AM1Name, std::vector<usize>{2});

  auto dataStore = std::make_unique<DataStore<uint8>>(std::vector<usize>{20}, std::vector<usize>{3}, 0);
  auto dataArray1 = DataArray<uint8>::Create(data, k_DataArray1Name, std::move(dataStore));

  data.setAdditionalParent(group4->getId(), group3->getId());
  return data;
}
} // namespace

TEST_CASE("SimplnxCore::MoveDataFilter Successful", "[Simplnx::Core][MoveDataFilter]")
{
  MoveDataFilter filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_Group1Path({k_Group1Name});
  const DataPath k_Group3Path({k_Group2Name, k_Group3Name});
  const DataPath k_Group4Path({k_Group2Name, k_Group4Name});

  args.insertOrAssign(MoveDataFilter::k_SourceDataPaths_Key, std::make_any<std::vector<DataPath>>({k_Group3Path, k_Group4Path}));
  args.insertOrAssign(MoveDataFilter::k_DestinationParentPath_Key, std::make_any<DataPath>(k_Group1Path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* group1 = dataStructure.getDataAs<DataGroup>(k_Group1Path);
  REQUIRE(group1->getDataMap().getSize() == 2);

  const auto* group2 = dataStructure.getDataAs<DataGroup>(DataPath({k_Group2Name}));
  REQUIRE(group2->getDataMap().getSize() == 0);

  REQUIRE(dataStructure.getDataAs<DataGroup>(k_Group3Path) == nullptr);

  const DataPath newGroup3Path = k_Group1Path.createChildPath(k_Group3Name);
  REQUIRE(dataStructure.getDataAs<DataGroup>(newGroup3Path) != nullptr);

  REQUIRE(dataStructure.getDataAs<DataGroup>(k_Group4Path) == nullptr);

  const DataPath newGroup4Path = k_Group1Path.createChildPath(k_Group4Name);
  REQUIRE(dataStructure.getDataAs<DataGroup>(newGroup4Path) != nullptr);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MoveDataFilter Unsuccessful", "[Simplnx::Core][MoveDataFilter]")
{
  MoveDataFilter filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_Group2Path({k_Group2Name});
  const DataPath k_Group3Path({k_Group3Name});

  SECTION("Object already exists in new data path")
  {
    args.insertOrAssign(MoveDataFilter::k_SourceDataPaths_Key, std::make_any<std::vector<DataPath>>({k_Group3Path}));
    args.insertOrAssign(MoveDataFilter::k_DestinationParentPath_Key, std::make_any<DataPath>(k_Group2Path));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  SECTION("Cannot reparent object to itself")
  {
    args.insertOrAssign(MoveDataFilter::k_SourceDataPaths_Key, std::make_any<std::vector<DataPath>>({k_Group3Path}));
    args.insertOrAssign(MoveDataFilter::k_DestinationParentPath_Key, std::make_any<DataPath>(k_Group3Path));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  SECTION("Cannot reparent object to a child object")
  {
    args.insertOrAssign(MoveDataFilter::k_SourceDataPaths_Key, std::make_any<std::vector<DataPath>>({k_Group2Path}));
    args.insertOrAssign(MoveDataFilter::k_DestinationParentPath_Key, std::make_any<DataPath>(k_Group3Path));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MoveDataFilter Tuple Size Mismatches Warning and Failure", "[Simplnx::Core][MoveDataFilter]")
{
  MoveDataFilter filter;
  Arguments args;
  DataStructure dataStructure = createDataStructure();

  const DataPath k_DataArrayPath({k_DataArray1Name});
  const DataPath k_AMPath({k_AM1Name});

  args.insertOrAssign(MoveDataFilter::k_SourceDataPaths_Key, std::make_any<std::vector<DataPath>>({k_DataArrayPath}));
  args.insertOrAssign(MoveDataFilter::k_DestinationParentPath_Key, std::make_any<DataPath>(k_AMPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);

  REQUIRE_FALSE(preflightResult.outputActions.warnings().empty());

  bool found = false;
  for(const auto& warning : preflightResult.outputActions.warnings())
  {
    if(warning.code == ::k_TupleDimMismatchWarningCode)
    {
      found = true;
    }
  }

  REQUIRE(found);

  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MoveDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][MoveDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "MoveDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "MoveDataFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<MoveDataFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      // Fixture has WhatToMove=0 (move attribute matrix): source is the AttributeMatrixSource path,
      // destination is the DataContainerDestination path.
      CHECK(args.value<DataPath>(MoveDataFilter::k_DestinationParentPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::vector<DataPath>>(MoveDataFilter::k_SourceDataPaths_Key) == std::vector<DataPath>{DataPath({"DataContainer", "CellData"})});
    }
  }
}
