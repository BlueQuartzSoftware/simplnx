#include "SimplnxCore/Filters/ComputeFeaturePhasesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// Cell
const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath({k_CellAMName});
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);
const std::string k_CellPhasesName = "CellPhases";
const DataPath k_CellPhasesPath = k_CellAMPath.createChildPath(k_CellPhasesName);

// Feature
const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath({k_FeatureAMName});
const std::string k_PhasesName = "Phases";
const DataPath k_FeaturePhasesPath = k_FeatureAMPath.createChildPath(k_PhasesName);

// Mirrors k_MaxListedFeatures in ComputeFeaturePhases.cpp
constexpr usize k_MaxListedFeatures = 15;

/**
 * @brief Extracts the comma separated list of Feature Ids from a mixed phase warning message. The
 * list is everything following the final ':' in the message, with the optional ", and <N> more ..."
 * truncation suffix removed. Parsing the list instead of matching a literal substring keeps the
 * assertions valid if the descriptive text preceding the list is reworded.
 * @param message The warning message emitted by the algorithm
 * @return The Feature Ids listed in the message, in the order they appear
 */
std::vector<int32> ExtractWarningFeatureIds(const std::string& message)
{
  std::vector<int32> featureIds;

  const usize colonPos = message.rfind(':');
  if(colonPos == std::string::npos)
  {
    return featureIds;
  }

  std::string listStr = message.substr(colonPos + 1);
  const usize truncationPos = listStr.find(", and ");
  if(truncationPos != std::string::npos)
  {
    listStr = listStr.substr(0, truncationPos);
  }

  usize tokenStart = 0;
  while(tokenStart < listStr.size())
  {
    usize tokenEnd = listStr.find(',', tokenStart);
    if(tokenEnd == std::string::npos)
    {
      tokenEnd = listStr.size();
    }
    const std::string token = listStr.substr(tokenStart, tokenEnd - tokenStart);
    const usize firstDigitPos = token.find_first_of("0123456789");
    if(firstDigitPos != std::string::npos)
    {
      featureIds.push_back(static_cast<int32>(std::stoi(token.substr(firstDigitPos))));
    }
    tokenStart = tokenEnd + 1;
  }

  return featureIds;
}
} // namespace

// Case 1: 7 cells, 3 features, uniform phases throughout
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Valid: Uniform Phases", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [1, 1, 2, 2, 2, 3, 3]
  // cellPhases  = [1, 1, 2, 2, 2, 1, 1]
  // Expected: featurePhases = [0, 1, 2, 1]; 0 warnings.

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{7});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    const std::array<int32, 7> fids = {1, 1, 2, 2, 2, 3, 3};
    const std::array<int32, 7> cphases = {1, 1, 2, 2, 2, 1, 1};
    for(usize i = 0; i < 7; i++)
    {
      (*featureIds)[i] = fids[i];
      (*cellPhases)[i] = cphases[i];
    }
  }

  // Execution
  {
    ComputeFeaturePhasesFilter filter;
    Arguments args;
    args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty());
  }

  // Validation
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath));
  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath);
  REQUIRE(featurePhases.getNumberOfTuples() == 4);

  const std::array<int32, 4> expected = {0, 1, 2, 1};
  for(usize i = 0; i < 4; i++)
  {
    REQUIRE(featurePhases[i] == expected[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2: empty feature in cells - ignore feature 0
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Valid: Feature 0 Skip", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [0, 0, 1, 1, 2, 2]
  // cellPhases  = [2, 2, 1, 1, 2, 2]
  // Expected featurePhases = [0, 1, 2]; 0 warnings.

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{6});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{3});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    const std::array<int32, 6> fids = {0, 0, 1, 1, 2, 2};
    const std::array<int32, 6> cphases = {2, 2, 1, 1, 2, 2};
    for(usize i = 0; i < 6; i++)
    {
      (*featureIds)[i] = fids[i];
      (*cellPhases)[i] = cphases[i];
    }
  }

  // Execution
  {
    ComputeFeaturePhasesFilter filter;
    Arguments args;
    args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty());
  }

  // Validation
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath));
  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath);
  REQUIRE(featurePhases.getNumberOfTuples() == 3);

  REQUIRE(featurePhases[0] == 0); // Must be zero since it is ignored
  REQUIRE(featurePhases[1] == 1);
  REQUIRE(featurePhases[2] == 2);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 3: 2 cells, mixed phase warning
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Valid: Mixed Phase Warning", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [1, 1]
  // cellPhases  = [1, 2]  — last-cell-wins: featurePhases[1] == 2
  // Expected: 1 warning (-500), feature 1

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{2});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{2});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    (*featureIds)[0] = 1;
    (*featureIds)[1] = 1;
    (*cellPhases)[0] = 1;
    (*cellPhases)[1] = 2;
  }

  // Execution
  ComputeFeaturePhasesFilter filter;
  Arguments args;
  args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Validation
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings().front().code == -500);

  // Warning message lists exactly one Feature Id: feature 1
  const std::string& warnMsg = executeResult.result.warnings().front().message;
  REQUIRE(ExtractWarningFeatureIds(warnMsg) == std::vector<int32>{1});

  // Tie-break - Last in. featurePhases[1] == 2
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath));
  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath);
  REQUIRE(featurePhases[1] == 2);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 4: Exceed Max Printable Mixed Features
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Valid: Truncated Mixed Feature Warning", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // 32 cells; features 1–16 each have 2 cells with different phases (phase 1 then phase 2).
  // Expected: warning listing feature ids 1–15 + "and 1 more occurrence".

  static constexpr usize k_NumFeatures = k_MaxListedFeatures + 1;
  static constexpr usize k_NumCells = k_NumFeatures * 2;

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{k_NumCells});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{k_NumFeatures + 1});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    // Each feature f (1..16) gets cells at indices (f-1)*2 and (f-1)*2+1 with phases 1 and 2.
    for(usize feature = 1; feature <= k_NumFeatures; feature++)
    {
      const usize base = (feature - 1) * 2;
      (*featureIds)[base] = static_cast<int32>(feature);
      (*featureIds)[base + 1] = static_cast<int32>(feature);
      (*cellPhases)[base] = 1;
      (*cellPhases)[base + 1] = 2;
    }
  }

  // Execution
  ComputeFeaturePhasesFilter filter;
  Arguments args;
  args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Validation
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings().front().code == -500);

  // Truncation: 16 conflicts, exactly the first 15 feature ids listed + "and 1 more occurrence"
  const std::string& warnMsg = executeResult.result.warnings().front().message;
  const std::vector<int32> listedFeatureIds = ExtractWarningFeatureIds(warnMsg);
  REQUIRE(listedFeatureIds.size() == k_MaxListedFeatures);
  for(usize i = 0; i < k_MaxListedFeatures; i++)
  {
    REQUIRE(listedFeatureIds[i] == static_cast<int32>(i + 1));
  }
  REQUIRE(warnMsg.find("and 1 more") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 5: featureId out of bounds
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Invalid: FeatureId Out Of Bounds", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [1, 5]  — max=5 >= numFeatures=3 → out-of-bounds
  // cellPhases  = [1, 1]
  // Expected: error code -5351

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{2});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{3});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    (*featureIds)[0] = 1;
    (*featureIds)[1] = 5;
    (*cellPhases)[0] = 1;
    (*cellPhases)[1] = 1;
  }

  // Execution
  ComputeFeaturePhasesFilter filter;
  Arguments args;
  args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);

  // Validation
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().front().code == -5351);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 6: Different Input Array Sizes
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Cell Array Size Mismatch", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // Expected: error code -61860

  const std::string k_CellFeatIdsAMName = "FeatureIdsAM";
  const DataPath k_CellFeatIdsAMPath({k_CellFeatIdsAMName});
  const std::string k_CellPhasesAMName = "CellPhasesAM";
  const DataPath k_CellPhasesAMPath({k_CellPhasesAMName});

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellFeatIdsAM = AttributeMatrix::Create(dataStructure, k_CellFeatIdsAMName, ShapeType{4});
    auto* cellPhasesAM = AttributeMatrix::Create(dataStructure, k_CellPhasesAMName, ShapeType{5});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{3});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellFeatIdsAM->getShape(), ShapeType{1}, cellFeatIdsAM->getId());
    Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellPhasesAM->getShape(), ShapeType{1}, cellPhasesAM->getId());

    (*featureIds)[0] = 1;
    (*featureIds)[1] = 1;
    (*featureIds)[2] = 1;
    (*featureIds)[3] = 1;
  }

  // Execution
  ComputeFeaturePhasesFilter filter;
  Arguments args;
  args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesAMPath.createChildPath(k_CellPhasesName)));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellFeatIdsAMPath.createChildPath(k_FeatureIdsName)));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

  auto preflightResult = filter.preflight(dataStructure, args);

  // Validation
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -61860);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 7: Negative Cell Phase
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Negative Cell Phase", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [1, 1]
  // cellPhases  = [1, -1]
  // Expected:  error code -61861

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{2});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{2});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    (*featureIds)[0] = 1;
    (*featureIds)[1] = 1;
    (*cellPhases)[0] = 1;
    (*cellPhases)[1] = -1;
  }

  // Execution
  ComputeFeaturePhasesFilter filter;
  Arguments args;
  args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);

  // Validation
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().front().code == -61861);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 8: Negative featureId value
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Negative FeatureId", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [-1, 1]  — negative featureId → validator error -5355
  // cellPhases  = [1, 1]
  // Expected: error code -5355

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{2});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{3});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    (*featureIds)[0] = -1;
    (*featureIds)[1] = 1;
    (*cellPhases)[0] = 1;
    (*cellPhases)[1] = 1;
  }

  // Execution
  ComputeFeaturePhasesFilter filter;
  Arguments args;
  args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);

  // Validation
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().front().code == -5355);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 9: Gap feature - a feature index with no cells stays zero initialized
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Valid: Gap Feature Stays Zero", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [1, 1, 3, 3]  — features 2 and 4 have no cells
  // cellPhases  = [1, 1, 2, 2]
  // Expected: featurePhases = [0, 1, 0, 2, 0]; 0 warnings. Documents the zero-init contract that
  // deviation D1 relies on: any feature index never written by the loop reads back as 0.

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{4});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{5});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    const std::array<int32, 4> fids = {1, 1, 3, 3};
    const std::array<int32, 4> cphases = {1, 1, 2, 2};
    for(usize i = 0; i < 4; i++)
    {
      (*featureIds)[i] = fids[i];
      (*cellPhases)[i] = cphases[i];
    }
  }

  // Execution
  {
    ComputeFeaturePhasesFilter filter;
    Arguments args;
    args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty());
  }

  // Validation
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath));
  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath);
  REQUIRE(featurePhases.getNumberOfTuples() == 5);

  const std::array<int32, 5> expected = {0, 1, 0, 2, 0};
  for(usize i = 0; i < 5; i++)
  {
    REQUIRE(featurePhases[i] == expected[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 10: Negative cell phase on a background cell is skipped, not an error
TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: Valid: Negative Cell Phase On Background Cell", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  // featureIds  = [0, 1, 1]
  // cellPhases  = [-5, 2, 2]
  // Expected: featurePhases = [0, 2]; 0 warnings, no error. The -61861 negative phase guard runs
  // after the `featureId == 0` skip, so a negative phase on a background cell is never inspected.

  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  // Construction
  {
    auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{3});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{2});

    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsName, cellAM->getShape(), ShapeType{1}, cellAM->getId());
    auto* cellPhases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellPhasesName, cellAM->getShape(), ShapeType{1}, cellAM->getId());

    const std::array<int32, 3> fids = {0, 1, 1};
    const std::array<int32, 3> cphases = {-5, 2, 2};
    for(usize i = 0; i < 3; i++)
    {
      (*featureIds)[i] = fids[i];
      (*cellPhases)[i] = cphases[i];
    }
  }

  // Execution
  {
    ComputeFeaturePhasesFilter filter;
    Arguments args;
    args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_PhasesName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty());
  }

  // Validation
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath));
  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(k_FeaturePhasesPath);
  REQUIRE(featurePhases.getNumberOfTuples() == 2);

  REQUIRE(featurePhases[0] == 0); // Background feature never written
  REQUIRE(featurePhases[1] == 2);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeaturePhasesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeaturePhasesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeaturePhasesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeaturePhasesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", k_CellAMName, "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", k_CellAMName, "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key) == "TestArray");
    }
  }
}
