#include "SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_CellFeatureIdsArrayName("FeatureIds");
const std::string k_FeatureTemperatureName("Feature Temperature");
const std::string k_FeatureDataArrayName("Feature Data Array");
const std::string k_CellTempArraySuffix("_ToCell");
const DataPath k_CellTempArrayPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
const DataPath k_CellFeatureArrayPath({k_FeatureDataArrayName + k_CellTempArraySuffix});

// Class 1 (Analytical) fixture: hand-built input whose expected output is derived by hand.
// See src/Plugins/SimplnxCore/vv/CopyFeatureArrayToElementArrayFilter.md (Oracle section).
namespace AnalyticalFixtures
{
const std::string k_ImageGeometryName("Image Geometry");
const std::string k_CellDataName("Cell Data");
const std::string k_CellFeatureDataName("Cell Feature Data");
const std::string k_AvgTempName("AvgTemp");
const std::string k_RGBName("RGB");
const std::string k_ActiveName("Active");
const std::string k_Suffix("_Cell");

constexpr usize k_RGBComponentCount = 3;

const DataPath k_FeatureIdsPath({k_ImageGeometryName, k_CellDataName, k_CellFeatureIdsArrayName});
const DataPath k_FeatureAMPath({k_ImageGeometryName, k_CellFeatureDataName});
const DataPath k_AvgTempPath({k_ImageGeometryName, k_CellFeatureDataName, k_AvgTempName});
const DataPath k_RGBPath({k_ImageGeometryName, k_CellFeatureDataName, k_RGBName});
const DataPath k_ActivePath({k_ImageGeometryName, k_CellFeatureDataName, k_ActiveName});
const DataPath k_AvgTempCellPath({k_ImageGeometryName, k_CellDataName, k_AvgTempName + k_Suffix});
const DataPath k_RGBCellPath({k_ImageGeometryName, k_CellDataName, k_RGBName + k_Suffix});
const DataPath k_ActiveCellPath({k_ImageGeometryName, k_CellDataName, k_ActiveName + k_Suffix});

// 4 x 3 x 1 (X,Y,Z) image geometry: 12 cells, 4 features (ids 0-3).
const std::vector<int32> k_FeatureIds = {0, 1, 1, 2, 2, 0, 3, 1, 3, 3, 0, 2};

// Feature-level source values (4 tuples).
const std::vector<float32> k_AvgTemp = {10.5F, 20.25F, -30.75F, 40.125F};
const std::vector<int32> k_RGB = {1, 2, 3, 40, 50, 60, -7, 8, -9, 100, 200, 127};
const std::vector<bool> k_Active = {false, true, true, false};

// Hand-derived expected outputs: out[i*C + c] = source[FeatureIds[i]*C + c].
// AvgTemp_Cell[i] = AvgTemp[FeatureIds[i]]:
//   fid  = [0,     1,     1,     2,      2,      0,    3,      1,     3,      3,      0,    2     ]
const std::vector<float32> k_ExpectedAvgTempCell = {10.5F, 20.25F, 20.25F, -30.75F, -30.75F, 10.5F, 40.125F, 20.25F, 40.125F, 40.125F, 10.5F, -30.75F};

// RGB_Cell tuple i = RGB tuple FeatureIds[i] (3 components per tuple):
const std::vector<int32> k_ExpectedRGBCell = {
    1,   2,   3,   // cell 0,  fid 0
    40,  50,  60,  // cell 1,  fid 1
    40,  50,  60,  // cell 2,  fid 1
    -7,  8,   -9,  // cell 3,  fid 2
    -7,  8,   -9,  // cell 4,  fid 2
    1,   2,   3,   // cell 5,  fid 0
    100, 200, 127, // cell 6,  fid 3
    40,  50,  60,  // cell 7,  fid 1
    100, 200, 127, // cell 8,  fid 3
    100, 200, 127, // cell 9,  fid 3
    1,   2,   3,   // cell 10, fid 0
    -7,  8,   -9,  // cell 11, fid 2
};

// Active_Cell[i] = Active[FeatureIds[i]]:
const std::vector<bool> k_ExpectedActiveCell = {false, true, true, true, true, false, false, true, false, false, false, true};

// Builds the 4x3x1 ImageGeom fixture with a Cell AttributeMatrix (FeatureIds) and a
// Feature AttributeMatrix (AvgTemp float32/1-comp, RGB int32/3-comp, Active bool/1-comp).
DataStructure CreateFixture()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeometryName);
  imageGeomPtr->setDimensions(SizeVec3{4, 3, 1}); // X, Y, Z
  imageGeomPtr->setSpacing(FloatVec3{1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin(FloatVec3{0.0F, 0.0F, 0.0F});

  // AttributeMatrix tuple shape is slowest-to-fastest (Z, Y, X)
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, k_CellDataName, std::vector<usize>{1, 3, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);

  auto* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {1, 3, 4}, {1}, cellAMPtr->getId());
  auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();
  for(usize i = 0; i < k_FeatureIds.size(); i++)
  {
    featureIdsStoreRef[i] = k_FeatureIds[i];
  }

  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, k_CellFeatureDataName, std::vector<usize>{4}, imageGeomPtr->getId());

  auto* avgTempPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_AvgTempName, {4}, {1}, featureAMPtr->getId());
  auto& avgTempStoreRef = avgTempPtr->getDataStoreRef();
  for(usize i = 0; i < k_AvgTemp.size(); i++)
  {
    avgTempStoreRef[i] = k_AvgTemp[i];
  }

  auto* rgbPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_RGBName, {4}, {k_RGBComponentCount}, featureAMPtr->getId());
  auto& rgbStoreRef = rgbPtr->getDataStoreRef();
  for(usize i = 0; i < k_RGB.size(); i++)
  {
    rgbStoreRef[i] = k_RGB[i];
  }

  auto* activePtr = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_ActiveName, {4}, {1}, featureAMPtr->getId());
  auto& activeStoreRef = activePtr->getDataStoreRef();
  for(usize i = 0; i < k_Active.size(); i++)
  {
    activeStoreRef[i] = k_Active[i];
  }

  return dataStructure;
}

Arguments CreateArguments()
{
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_AvgTempPath, k_RGBPath, k_ActivePath}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_Suffix));
  return args;
}
} // namespace AnalyticalFixtures
} // namespace

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Empty selection (filter guard)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  CopyFeatureArrayToElementArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // The FeatureIds path must be VALID so that parameter validation passes and the
  // filter's own empty-selection guard in preflightImpl()/executeImpl() is what fires.
  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {30}, {1});

  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(""));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  for(const Error& err : preflightResult.outputActions.errors())
  {
    REQUIRE(err.code == nx::core::FilterParameter::Constants::k_Validate_Empty_Value);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors().size() == 1);
  for(const Error& err : executeResult.result.errors())
  {
    REQUIRE(err.code == nx::core::FilterParameter::Constants::k_Validate_Empty_Value);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Feature array tuple count mismatch (-3020)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  // Cell-level FeatureIds must exist (validated selection parameter) but is not part of the tuple-count check.
  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {30}, {1});

  // Two feature-level arrays with deliberately different tuple counts (3 != 4) so the validateNumberOfTuples()
  // guard over the selected feature arrays fails and emits error -3020.
  Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureTemperatureName, {3}, {1});
  Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureDataArrayName, {4}, {1});

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName}), DataPath({k_FeatureDataArrayName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3020);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Non-DataArray selection rejected", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // A NeighborList is an IArray but NOT an IDataArray. The parameter restricts selections to
  // ArrayType::DataArray, so this must fail parameter validation with a clean error instead of
  // reaching preflightImpl() and throwing std::bad_cast from getDataRefAs<IDataArray>().
  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureAMPath));
  auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureAMPath);
  auto* neighborListPtr = NeighborList<float32>::Create(dataStructure, "NeighborList", featureAM.getShape(), featureAM.getId());
  REQUIRE(neighborListPtr != nullptr);

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{AnalyticalFixtures::k_FeatureAMPath.createChildPath("NeighborList")}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Suffix contains '/' (-3021)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>("/Cell"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3021);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Execute Error - Created name collides with existing array (-266)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // Selecting the FeatureIds array itself with an empty suffix derives a created path identical
  // to the source path. IFilter::preflight() does not apply output actions, so the collision is
  // reported when the CreateArrayAction is applied at execute (-266) — never a silent overwrite.
  // (In a pipeline, the pipeline-level preflight applies actions and catches this before execute.)
  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{AnalyticalFixtures::k_FeatureIdsPath}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(""));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == -266);

  // The original FeatureIds array must be untouched.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath));
  const auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath).getDataStoreRef();
  for(usize i = 0; i < AnalyticalFixtures::k_FeatureIds.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(featureIdsStoreRef[i] == AnalyticalFixtures::k_FeatureIds[i]);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Analytical Oracle (Class 1)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const usize numCells = AnalyticalFixtures::k_FeatureIds.size();

  // ------------------------------------------------------------------------
  // Class 1 (Analytical): compare against the hand-derived expected constants.
  // out[i*C + c] = source[FeatureIds[i]*C + c] — derivation in the V&V report.
  // ------------------------------------------------------------------------
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(AnalyticalFixtures::k_AvgTempCellPath));
  const auto& avgTempCellRef = dataStructure.getDataRefAs<Float32Array>(AnalyticalFixtures::k_AvgTempCellPath).getDataStoreRef();
  REQUIRE(avgTempCellRef.getNumberOfTuples() == numCells);
  REQUIRE(avgTempCellRef.getNumberOfComponents() == 1);
  for(usize i = 0; i < AnalyticalFixtures::k_ExpectedAvgTempCell.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(avgTempCellRef[i] == AnalyticalFixtures::k_ExpectedAvgTempCell[i]);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_RGBCellPath));
  const auto& rgbCellRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_RGBCellPath).getDataStoreRef();
  REQUIRE(rgbCellRef.getNumberOfTuples() == numCells);
  REQUIRE(rgbCellRef.getNumberOfComponents() == AnalyticalFixtures::k_RGBComponentCount);
  for(usize i = 0; i < AnalyticalFixtures::k_ExpectedRGBCell.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(rgbCellRef[i] == AnalyticalFixtures::k_ExpectedRGBCell[i]);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(AnalyticalFixtures::k_ActiveCellPath));
  const auto& activeCellRef = dataStructure.getDataRefAs<BoolArray>(AnalyticalFixtures::k_ActiveCellPath).getDataStoreRef();
  REQUIRE(activeCellRef.getNumberOfTuples() == numCells);
  REQUIRE(activeCellRef.getNumberOfComponents() == 1);
  for(usize i = 0; i < AnalyticalFixtures::k_ExpectedActiveCell.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(activeCellRef[i] == AnalyticalFixtures::k_ExpectedActiveCell[i]);
  }

  // ------------------------------------------------------------------------
  // Class 4 (Invariant): piecewise constancy — every pair of cells sharing a
  // feature id must have identical output tuples (checked on the 3-comp array).
  // ------------------------------------------------------------------------
  for(usize i = 0; i < numCells; i++)
  {
    for(usize j = i + 1; j < numCells; j++)
    {
      if(AnalyticalFixtures::k_FeatureIds[i] == AnalyticalFixtures::k_FeatureIds[j])
      {
        for(usize c = 0; c < AnalyticalFixtures::k_RGBComponentCount; c++)
        {
          CAPTURE(i, j, c);
          REQUIRE(rgbCellRef[i * AnalyticalFixtures::k_RGBComponentCount + c] == rgbCellRef[j * AnalyticalFixtures::k_RGBComponentCount + c]);
        }
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Execute Error - Negative FeatureIds (-5355)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  // Corrupt one feature id to a negative value. Preflight cannot see array values,
  // so this must pass preflight and fail in execute via
  // ValidateFeatureIdsToFeatureAttributeMatrixIndexing (ignoreNegativeValues = false).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath));
  auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath).getDataStoreRef();
  featureIdsStoreRef[5] = -1;

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == -5355);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Execute Error - FeatureId exceeds Feature tuple count (-5351)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  // Corrupt one feature id to 4: the feature arrays have 4 tuples (valid ids 0-3),
  // so id 4 would read past the end. Must pass preflight and fail in execute with -5351.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath));
  auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath).getDataStoreRef();
  featureIdsStoreRef[5] = 4;

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == -5351);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Over-provisioned Feature array accepted", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // Pins deviation CopyFeatureArrayToElementArrayFilter-D2: DREAM3D 6.5.171 errors (-5555) when the
  // feature array has more tuples than largestFeatureId + 1; SIMPLNX deliberately accepts it.
  DataStructure dataStructure;

  // 6 cells referencing features 0-2; feature array over-provisioned with 8 tuples.
  auto* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {6}, {1});
  auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();
  const std::vector<int32> featureIds = {0, 2, 1, 1, 0, 2};
  for(usize i = 0; i < featureIds.size(); i++)
  {
    featureIdsStoreRef[i] = featureIds[i];
  }

  auto* featureValuesPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureTemperatureName, {8}, {1});
  auto& featureValuesStoreRef = featureValuesPtr->getDataStoreRef();
  for(usize i = 0; i < featureValuesStoreRef.getNumberOfTuples(); i++)
  {
    featureValuesStoreRef[i] = static_cast<float32>(i) * 2.0F + 1.0F; // [1, 3, 5, 7, 9, 11, 13, 15]
  }

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Hand-derived: out[i] = featureValues[featureIds[i]] = [1, 5, 3, 3, 1, 5]
  const DataPath createdPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(createdPath));
  const auto& createdRef = dataStructure.getDataRefAs<Float32Array>(createdPath).getDataStoreRef();
  const std::vector<float32> expected = {1.0F, 5.0F, 3.0F, 3.0F, 1.0F, 5.0F};
  for(usize i = 0; i < expected.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(createdRef[i] == expected[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Zero-tuple FeatureIds accepted", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // A FeatureIds array with zero tuples is degenerate but legal: there is nothing to copy,
  // and the filter must succeed with empty output arrays (not crash in range validation).
  DataStructure dataStructure;

  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {0}, {1});
  Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureTemperatureName, {4}, {1});

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath createdPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(createdPath));
  const auto& createdRef = dataStructure.getDataRefAs<Float32Array>(createdPath).getDataStoreRef();
  REQUIRE(createdRef.getNumberOfTuples() == 0);
}

using ListOfTypes = std::tuple<int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64>;
TEMPLATE_LIST_TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Valid filter execution", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]", ListOfTypes)
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  // Create Cell FeatureIds array
  Int32Array* cellFeatureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {{10, 3}}, {1});
  REQUIRE(cellFeatureIdsPtr != nullptr);
  Int32Array& cellFeatureIds = *cellFeatureIdsPtr;

  for(usize y = 0; y < 3; y++)
  {
    for(usize x = 0; x < 10; x++)
    {
      usize index = (10 * y) + x;
      cellFeatureIds[index] = static_cast<int32>(y);
    }
  }

  // Create two feature data arrays with 3 tuples each, filled with distinct per-feature
  // values so that an indexing mistake in the filter cannot go undetected.
  DataArray<TestType>* avgTempValuePtr = DataArray<TestType>::template CreateWithStore<DataStore<TestType>>(dataStructure, k_FeatureTemperatureName, {3}, {1});
  REQUIRE(avgTempValuePtr != nullptr);
  DataArray<TestType>& avgTempValue = *avgTempValuePtr;
  DataArray<TestType>* featureDataPtr = DataArray<TestType>::template CreateWithStore<DataStore<TestType>>(dataStructure, k_FeatureDataArrayName, {3}, {1});
  REQUIRE(featureDataPtr != nullptr);
  DataArray<TestType>& featureDataValue = *featureDataPtr;

  for(usize i = 0; i < 3; i++)
  {
    avgTempValue[i] = static_cast<TestType>(i * 10 + 5);    // [5, 15, 25]
    featureDataValue[i] = static_cast<TestType>(i * 3 + 1); // [1, 4, 7]
  }

  // Create filter and set arguments
  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;

  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName}), DataPath({k_FeatureDataArrayName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Check the filter results
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(k_CellTempArrayPath));
  const auto& createdElementTempArray = dataStructure.getDataRefAs<DataArray<TestType>>(k_CellTempArrayPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(k_CellFeatureArrayPath));
  const auto& createdElementFeatureArray = dataStructure.getDataRefAs<DataArray<TestType>>(k_CellFeatureArrayPath);
  REQUIRE(createdElementTempArray.getNumberOfTuples() == createdElementFeatureArray.getNumberOfTuples());
  for(usize i = 0; i < createdElementTempArray.getNumberOfTuples(); i++)
  {
    CAPTURE(i);
    int32 featureId = cellFeatureIds[i];
    TestType value1 = createdElementTempArray[i];
    TestType value2 = createdElementFeatureArray[i];
    TestType featureValue1 = avgTempValue[featureId];
    TestType featureValue2 = featureDataValue[featureId];
    REQUIRE(value1 == featureValue1);
    REQUIRE(value2 == featureValue2);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CopyFeatureArrayToElementArrayFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CopyFeatureArrayToElementArrayFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CopyFeatureArrayToElementArrayFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::vector<DataPath>>(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key) == std::vector<DataPath>{DataPath({"DataContainer", "CellData", "TestArray"})});
      CHECK(args.value<DataPath>(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // The legacy CreatedArrayName is intentionally NOT mapped onto the suffix; the copied array keeps
      // the input array's name, so the suffix stays at its default (empty).
      CHECK(args.value<std::string>(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key).empty());
    }
  }
}
