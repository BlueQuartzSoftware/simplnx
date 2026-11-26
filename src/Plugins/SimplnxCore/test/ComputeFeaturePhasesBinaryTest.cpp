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
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeaturePhasesBinaryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeaturePhasesBinaryFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeFeaturePhasesBinaryFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2});
  REQUIRE(cellDataPtr != nullptr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Ids", {2}, {2}, cellDataPtr->getId());
  (*featureIdsPtr)[0] = 1;
  (*featureIdsPtr)[1] = 0;
  (*featureIdsPtr)[2] = 2;
  (*featureIdsPtr)[3] = 0;
  auto* maskPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", {2}, {1}, cellDataPtr->getId());
  maskPtr->fill(true);
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
void CreateBinaryPhaseInputs(DataStructure& dataStructure, bool featureMask, usize maskTuples = 6)
{
  auto* containerPtr = DataGroup::Create(dataStructure, "Container");
  REQUIRE(containerPtr != nullptr);
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", {6}, containerPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {2, 2}, containerPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  const std::array<int32, 6> ids = {0, 1, 2, 1, 2, 1};
  auto* idArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", {6}, {1}, cellsPtr->getId());
  auto copyIdsResult = idArrayPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(ids.data(), ids.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIdsResult);
  if(featureMask)
  {
    const std::array<T, 4> values = {static_cast<T>(0), static_cast<T>(1), static_cast<T>(1), static_cast<T>(0)};
    auto* maskPtr = UnitTest::CreateTestDataArray<T>(dataStructure, "Mask", {2, 2}, {1}, featuresPtr->getId());
    auto copyMaskResult = maskPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyMaskResult);
    // Invalid values and a different tuple count must be ignored in Feature-mask mode.
    idArrayPtr->fill(-7);
  }
  else
  {
    auto* maskAmPtr = AttributeMatrix::Create(dataStructure, "Mask Cells", {maskTuples}, containerPtr->getId());
    REQUIRE(maskAmPtr != nullptr);
    auto* maskPtr = UnitTest::CreateTestDataArray<T>(dataStructure, "Mask", {maskTuples}, {1}, maskAmPtr->getId());
    if(maskTuples == 6)
    {
      const std::array<T, 6> values = {static_cast<T>(1), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)};
      auto copyMaskResult = maskPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyMaskResult);
    }
  }
}

void CheckBinaryPhases(const DataStructure& dataStructure, const std::array<int32, 4>& expected)
{
  const auto* outputPtr = dataStructure.getDataAs<Int32Array>(DataPath({"Container", "Features", "Phases"}));
  REQUIRE(outputPtr != nullptr);
  CHECK(outputPtr->getTupleShape() == ShapeType{2, 2});
  CHECK(outputPtr->getNumberOfComponents() == 1);
  std::array<int32, 4> actual{};
  auto readResult = outputPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  CHECK(actual == expected);
  CHECK(dataStructure.getData(DataPath({"Container", "Cells", "Phases"})) == nullptr);
}
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Cell mask hand oracle", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]", bool, uint8)
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreateBinaryPhaseInputs<TestType>(dataStructure, false);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  auto preflightResult = filter.preflight(dataStructure, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  auto executeResult = filter.execute(dataStructure, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  // Feature 0 is true. Feature 1 ends false. Feature 2 ends true. Feature 3 is unreferenced.
  CheckBinaryPhases(dataStructure, {1, 0, 1, 0});
  CHECK(dataStructure.getData(DataPath({"Container", "Cell Ensemble Data"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Feature mask hand oracle", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]", bool, uint8)
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreateBinaryPhaseInputs<TestType>(dataStructure, true);
  if constexpr(std::is_same_v<TestType, uint8>)
  {
    auto* maskPtr = dataStructure.getDataAs<UInt8Array>(DataPath({"Container", "Features", "Mask"}));
    REQUIRE(maskPtr != nullptr);
    const std::array<uint8, 4> values = {0, 2, 255, 0};
    auto copyMaskResult = maskPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyMaskResult);
  }
  ComputeFeaturePhasesBinaryFilter filter;
  const bool alias = GENERATE(false, true);
  DataPath maskPath({"Container", "Features", "Mask"});
  if(alias)
  {
    auto* aliasGroupPtr = DataGroup::Create(dataStructure, "Alias");
    REQUIRE(aliasGroupPtr != nullptr);
    auto* featuresPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Container", "Features"}));
    REQUIRE(featuresPtr != nullptr);
    REQUIRE(dataStructure.setAdditionalParent(featuresPtr->getId(), aliasGroupPtr->getId()));
    maskPath = DataPath({"Alias", "Features", "Mask"});
  }
  auto args = BinaryPhaseArguments(filter, maskPath);
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CreateCellEnsembleAM_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CellEnsembleAMName_Key, std::make_any<std::string>("Ensembles"));
  auto preflightResult = filter.preflight(dataStructure, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  auto executeResult = filter.execute(dataStructure, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  CheckBinaryPhases(dataStructure, {0, 1, 1, 0});
  const auto* ensemblePtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Container", "Ensembles"}));
  REQUIRE(ensemblePtr != nullptr);
  CHECK(ensemblePtr->getShape() == ShapeType{2});
  CHECK(dataStructure.getData(DataPath({"Ensembles"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Cell mask tuple mismatch", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  const usize maskTuples = GENERATE(5, 7);
  DataStructure dataStructure;
  CreateBinaryPhaseInputs<uint8>(dataStructure, false, maskTuples);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -53802);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Feature Id range", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  const int32 invalidId = GENERATE(-1, 4);
  DataStructure dataStructure;
  CreateBinaryPhaseInputs<bool>(dataStructure, false);
  auto* idsPtr = dataStructure.getDataAs<Int32Array>(DataPath({"Container", "Cells", "Ids"}));
  REQUIRE(idsPtr != nullptr);
  idsPtr->setValue(5, invalidId);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  auto preflightResult = filter.preflight(dataStructure, args).outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == -53801);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Equal counts in different matrices use Cell mode", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreateBinaryPhaseInputs<uint8>(dataStructure, false);
  auto* cellsPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Container", "Cells"}));
  auto* maskCellsPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Container", "Mask Cells"}));
  REQUIRE(cellsPtr != nullptr);
  REQUIRE(maskCellsPtr != nullptr);
  auto resizeResult = cellsPtr->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  resizeResult = maskCellsPtr->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  auto* idsPtr = dataStructure.getDataAs<Int32Array>(DataPath({"Container", "Cells", "Ids"}));
  auto* maskPtr = dataStructure.getDataAs<UInt8Array>(DataPath({"Container", "Mask Cells", "Mask"}));
  REQUIRE(idsPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  const std::array<int32, 4> idValues = {2, 1, 2, 0};
  const std::array<uint8, 4> maskValues = {255, 2, 0, 255};
  auto copyResult = idsPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(idValues.data(), idValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  copyResult = maskPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(maskValues.data(), maskValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, DataPath({"Container", "Mask Cells", "Mask"}));
  args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CreateCellEnsembleAM_Key, std::make_any<bool>(true));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckBinaryPhases(dataStructure, {1, 1, 0, 0});
  const auto* ensemblePtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Container", "Cell Ensemble Data"}));
  REQUIRE(ensemblePtr != nullptr);
  CHECK(ensemblePtr->getShape() == ShapeType{2});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Empty and chunk-boundary masks", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();
  const bool featureMask = GENERATE(false, true);
  const usize tupleCount = GENERATE(usize(0), usize(1'000'001));
  DataStructure dataStructure;
  CreateBinaryPhaseInputs<uint8>(dataStructure, featureMask);
  const DataPath maskAMPath = featureMask ? DataPath({"Container", "Features"}) : DataPath({"Container", "Mask Cells"});
  auto* maskAmPtr = dataStructure.getDataAs<AttributeMatrix>(maskAMPath);
  REQUIRE(maskAmPtr != nullptr);
  auto resizeResult = maskAmPtr->resizeTuples({tupleCount});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  auto* maskPtr = dataStructure.getDataAs<UInt8Array>(maskAMPath.createChildPath("Mask"));
  REQUIRE(maskPtr != nullptr);
  maskPtr->fill(255);
  if(tupleCount > 0)
  {
    maskPtr->setValue(tupleCount - 1, 0);
  }
  if(!featureMask)
  {
    auto* cellsPtr = dataStructure.getDataAs<AttributeMatrix>(DataPath({"Container", "Cells"}));
    REQUIRE(cellsPtr != nullptr);
    resizeResult = cellsPtr->resizeTuples({tupleCount});
    SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
    auto* idsPtr = dataStructure.getDataAs<Int32Array>(DataPath({"Container", "Cells", "Ids"}));
    REQUIRE(idsPtr != nullptr);
    idsPtr->fill(1);
    if(tupleCount > 0)
    {
      idsPtr->setValue(0, 0);
    }
  }
  ComputeFeaturePhasesBinaryFilter filter;
  auto args = BinaryPhaseArguments(filter, maskAMPath.createChildPath("Mask"));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(!featureMask)
  {
    CheckBinaryPhases(dataStructure, tupleCount == 0 ? std::array<int32, 4>{0, 0, 0, 0} : std::array<int32, 4>{1, 0, 0, 0});
  }
  else
  {
    const auto* outputPtr = dataStructure.getDataAs<Int32Array>(DataPath({"Container", "Features", "Phases"}));
    REQUIRE(outputPtr != nullptr);
    CHECK(outputPtr->getNumberOfTuples() == tupleCount);
    if(tupleCount > 0)
    {
      std::array<int32, 2> boundary{};
      auto readResult = outputPtr->getDataStoreRef().copyIntoBuffer(tupleCount - 2, nonstd::span<int32>(boundary.data(), boundary.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(readResult);
      CHECK(boundary == std::array<int32, 2>{1, 0});
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
