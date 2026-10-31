#include "SimplnxCore/Filters/ComputeFeaturePhasesBinaryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <type_traits>

using namespace nx::core;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter][BackwardsCompatibility]")
{
  UnitTest::LoadPlugins();
  auto app = Application::GetOrCreateInstance();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeaturePhasesBinaryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeaturePhasesBinaryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeaturePhasesBinaryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeaturePhasesBinaryFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key) == "TestArray");
      CHECK(args.value<DataPath>(ComputeFeaturePhasesBinaryFilter::k_CellFeatureAMPath_Key) == DataPath({"DataContainer", "CellFeatureData"}));
      CHECK(args.value<bool>(ComputeFeaturePhasesBinaryFilter::k_CreateCellEnsembleAM_Key));
      CHECK(args.value<std::string>(ComputeFeaturePhasesBinaryFilter::k_CellEnsembleAMName_Key) == "Legacy Ensembles");
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Cell Feature Ids requires 1 component", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", {2});
  REQUIRE(cellData != nullptr);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Ids", {2}, {2}, cellData->getId());
  (*featureIds)[0] = 1;
  (*featureIds)[1] = 0;
  (*featureIds)[2] = 2;
  (*featureIds)[3] = 0;
  auto* mask = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", {2}, {1}, cellData->getId());
  mask->fill(true);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", {3}) != nullptr);

  ComputeFeaturePhasesBinaryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Cell Data", "Feature Ids"})));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Cell Data", "Mask"})));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CellFeatureAMPath_Key, std::make_any<DataPath>(DataPath({"Feature Data"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(std::any_of(preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end(), [](const Error& error) { return error.code == -208; }));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
Arguments BinaryPhaseArguments(const ComputeFeaturePhasesBinaryFilter& filter, const DataPath& maskPath)
{
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Container", "Cells", "Ids"})));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CellFeatureAMPath_Key, std::make_any<DataPath>(DataPath({"Container", "Features"})));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>("Phases"));
  return args;
}

template <typename T>
void CreateBinaryPhaseInputs(DataStructure& ds, bool featureMask, usize maskTuples = 6)
{
  auto* container = DataGroup::Create(ds, "Container");
  REQUIRE(container != nullptr);
  auto* cells = AttributeMatrix::Create(ds, "Cells", {6}, container->getId());
  REQUIRE(cells != nullptr);
  auto* features = AttributeMatrix::Create(ds, "Features", {2, 2}, container->getId());
  REQUIRE(features != nullptr);
  const std::array<int32, 6> ids = {0, 1, 2, 1, 2, 1};
  auto* idArray = UnitTest::CreateTestDataArray<int32>(ds, "Ids", {6}, {1}, cells->getId());
  auto copyIdsResult = idArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(ids.data(), ids.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIdsResult);
  if(featureMask)
  {
    const std::array<T, 4> values = {static_cast<T>(0), static_cast<T>(1), static_cast<T>(1), static_cast<T>(0)};
    auto* mask = UnitTest::CreateTestDataArray<T>(ds, "Mask", {2, 2}, {1}, features->getId());
    auto copyMaskResult = mask->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyMaskResult);
    // Invalid values and a different tuple count must be ignored in Feature-mask mode.
    idArray->fill(-7);
  }
  else
  {
    auto* maskAM = AttributeMatrix::Create(ds, "Mask Cells", {maskTuples}, container->getId());
    REQUIRE(maskAM != nullptr);
    auto* mask = UnitTest::CreateTestDataArray<T>(ds, "Mask", {maskTuples}, {1}, maskAM->getId());
    if(maskTuples == 6)
    {
      const std::array<T, 6> values = {static_cast<T>(1), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)};
      auto copyMaskResult = mask->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyMaskResult);
    }
  }
}

void CheckBinaryPhases(const DataStructure& ds, const std::array<int32, 4>& expected)
{
  const auto* output = ds.getDataAs<Int32Array>(DataPath({"Container", "Features", "Phases"}));
  REQUIRE(output != nullptr);
  CHECK(output->getTupleShape() == ShapeType{2, 2});
  CHECK(output->getNumberOfComponents() == 1);
  std::array<int32, 4> actual{};
  auto readResult = output->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  CHECK(actual == expected);
  CHECK(ds.getData(DataPath({"Container", "Cells", "Phases"})) == nullptr);
}
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Cell mask hand oracle", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]", bool, uint8)
{
  UnitTest::LoadPlugins();
  DataStructure ds;
  CreateBinaryPhaseInputs<TestType>(ds, false);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  auto preflightResult = filter.preflight(ds, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  auto executeResult = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  // Feature 0 is true. Feature 1 ends false. Feature 2 ends true. Feature 3 is unreferenced.
  CheckBinaryPhases(ds, {1, 0, 1, 0});
  CHECK(ds.getData(DataPath({"Container", "Cell Ensemble Data"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEMPLATE_TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Feature mask hand oracle", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]", bool, uint8)
{
  UnitTest::LoadPlugins();
  DataStructure ds;
  CreateBinaryPhaseInputs<TestType>(ds, true);
  if constexpr(std::is_same_v<TestType, uint8>)
  {
    auto* mask = ds.getDataAs<UInt8Array>(DataPath({"Container", "Features", "Mask"}));
    REQUIRE(mask != nullptr);
    const std::array<uint8, 4> values = {0, 2, 255, 0};
    auto copyMaskResult = mask->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyMaskResult);
  }
  ComputeFeaturePhasesBinaryFilter filter;
  const bool alias = GENERATE(false, true);
  DataPath maskPath({"Container", "Features", "Mask"});
  if(alias)
  {
    auto* aliasGroup = DataGroup::Create(ds, "Alias");
    REQUIRE(aliasGroup != nullptr);
    auto* features = ds.getDataAs<AttributeMatrix>(DataPath({"Container", "Features"}));
    REQUIRE(features != nullptr);
    REQUIRE(ds.setAdditionalParent(features->getId(), aliasGroup->getId()));
    maskPath = DataPath({"Alias", "Features", "Mask"});
  }
  auto args = BinaryPhaseArguments(filter, maskPath);
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CreateCellEnsembleAM_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CellEnsembleAMName_Key, std::make_any<std::string>("Ensembles"));
  auto preflightResult = filter.preflight(ds, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  auto executeResult = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  CheckBinaryPhases(ds, {0, 1, 1, 0});
  const auto* ensemble = ds.getDataAs<AttributeMatrix>(DataPath({"Container", "Ensembles"}));
  REQUIRE(ensemble != nullptr);
  CHECK(ensemble->getShape() == ShapeType{2});
  CHECK(ds.getData(DataPath({"Ensembles"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Cell mask tuple mismatch", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  const usize maskTuples = GENERATE(5, 7);
  DataStructure ds;
  CreateBinaryPhaseInputs<uint8>(ds, false, maskTuples);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -53802);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Feature Id range", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  const int32 invalidId = GENERATE(-1, 4);
  DataStructure ds;
  CreateBinaryPhaseInputs<bool>(ds, false);
  auto* ids = ds.getDataAs<Int32Array>(DataPath({"Container", "Cells", "Ids"}));
  REQUIRE(ids != nullptr);
  ids->setValue(5, invalidId);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  auto preflightResult = filter.preflight(ds, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == -53801);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Equal counts in different matrices use Cell mode", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure ds;
  CreateBinaryPhaseInputs<uint8>(ds, false);
  auto* cells = ds.getDataAs<AttributeMatrix>(DataPath({"Container", "Cells"}));
  auto* maskCells = ds.getDataAs<AttributeMatrix>(DataPath({"Container", "Mask Cells"}));
  REQUIRE(cells != nullptr);
  REQUIRE(maskCells != nullptr);
  auto resizeResult = cells->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  resizeResult = maskCells->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  auto* ids = ds.getDataAs<Int32Array>(DataPath({"Container", "Cells", "Ids"}));
  auto* mask = ds.getDataAs<UInt8Array>(DataPath({"Container", "Mask Cells", "Mask"}));
  REQUIRE(ids != nullptr);
  REQUIRE(mask != nullptr);
  const std::array<int32, 4> idValues = {2, 1, 2, 0};
  const std::array<uint8, 4> maskValues = {255, 2, 0, 255};
  auto copyResult = ids->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(idValues.data(), idValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  copyResult = mask->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(maskValues.data(), maskValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CreateCellEnsembleAM_Key, std::make_any<bool>(true));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckBinaryPhases(ds, {1, 1, 0, 0});
  const auto* ensemble = ds.getDataAs<AttributeMatrix>(DataPath({"Container", "Cell Ensemble Data"}));
  REQUIRE(ensemble != nullptr);
  CHECK(ensemble->getShape() == ShapeType{2});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Empty and chunk-boundary masks", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  const bool featureMask = GENERATE(false, true);
  const usize tupleCount = GENERATE(usize(0), usize(1'000'001));
  DataStructure ds;
  CreateBinaryPhaseInputs<uint8>(ds, featureMask);
  const DataPath maskAMPath = featureMask ? DataPath({"Container", "Features"}) : DataPath({"Container", "Mask Cells"});
  auto* maskAM = ds.getDataAs<AttributeMatrix>(maskAMPath);
  REQUIRE(maskAM != nullptr);
  auto resizeResult = maskAM->resizeTuples({tupleCount});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  auto* mask = ds.getDataAs<UInt8Array>(maskAMPath.createChildPath("Mask"));
  REQUIRE(mask != nullptr);
  mask->fill(255);
  if(tupleCount > 0)
  {
    mask->setValue(tupleCount - 1, 0);
  }
  if(!featureMask)
  {
    auto* cells = ds.getDataAs<AttributeMatrix>(DataPath({"Container", "Cells"}));
    REQUIRE(cells != nullptr);
    resizeResult = cells->resizeTuples({tupleCount});
    SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
    auto* ids = ds.getDataAs<Int32Array>(DataPath({"Container", "Cells", "Ids"}));
    REQUIRE(ids != nullptr);
    ids->fill(1);
    if(tupleCount > 0)
    {
      ids->setValue(0, 0);
    }
  }
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, maskAMPath.createChildPath("Mask"));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(!featureMask)
  {
    CheckBinaryPhases(ds, tupleCount == 0 ? std::array<int32, 4>{0, 0, 0, 0} : std::array<int32, 4>{1, 0, 0, 0});
  }
  else
  {
    const auto* output = ds.getDataAs<Int32Array>(DataPath({"Container", "Features", "Phases"}));
    REQUIRE(output != nullptr);
    CHECK(output->getNumberOfTuples() == tupleCount);
    if(tupleCount > 0)
    {
      std::array<int32, 2> boundary{};
      auto readResult = output->getDataStoreRef().copyIntoBuffer(tupleCount - 2, nonstd::span<int32>(boundary.data(), boundary.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(readResult);
      CHECK(boundary == std::array<int32, 2>{1, 0});
    }
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}
