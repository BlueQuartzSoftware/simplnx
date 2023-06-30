#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CopyDataObjectFilter.hpp"

#include "complex/Parameters/MultiPathSelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;

TEST_CASE("ComplexCore::CopyDataObjectFilter(Valid Execution)", "[ComplexCore][CopyDataObjectFilter]")
{
  static const DataPath k_DataPath1({Constants::k_SmallIN100, "Phase Data"});
  static const DataPath k_DataPath2({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"});

  CopyDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  SECTION("Copy to Same Parent")
  {
    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({k_DataPath1, k_DataPath2}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);

    const auto* path1Copy = dataStructure.getDataAs<DataGroup>(DataPath({Constants::k_SmallIN100, "Phase Data_COPY"}));
    const auto* path2Copy = dataStructure.getDataAs<Int32Array>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases_COPY"}));
    REQUIRE(path1Copy != nullptr);
    REQUIRE(path2Copy != nullptr);
  }
  SECTION("Copy to New Parent")
  {
    static const DataPath k_CopyPath({Constants::k_SmallIN100});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({k_DataPath1, k_DataPath2}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(true));
    args.insert(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(k_CopyPath));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);

    DataGroup* copiedDataGroup = dataStructure.getDataAs<DataGroup>(k_CopyPath.createChildPath(k_DataPath1.getTargetName() + "_COPY"));
    Int32Array* copiedArray = dataStructure.getDataAs<Int32Array>(k_CopyPath.createChildPath(k_DataPath2.getTargetName() + "_COPY"));
    REQUIRE(copiedDataGroup != nullptr);
    REQUIRE(copiedArray != nullptr);
  }
}

TEST_CASE("ComplexCore::CopyDataObjectFilter(Invalid Parameters)", "[ComplexCore][CopyDataObjectFilter]")
{
  CopyDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  SECTION("Data to be copied does not exist")
  {
    const DataPath dataPath({Constants::k_SmallIN100, "Bad Data Foo"});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));

    auto result = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("Same parent copy data suffix is empty")
  {
    const DataPath dataPath({Constants::k_SmallIN100, "Phase Data"});
    const DataPath copyPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>(""));

    auto result = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("Copy data new parent tuple mismatch")
  {
    auto* attributeMatrix = AttributeMatrix::Create(dataStructure, "TestAttributeMatrix", {10, 5, 1});

    const DataPath dataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"});
    const DataPath copyPath({"TestAttributeMatrix"});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(true));
    args.insert(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(copyPath));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto result = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_INVALID(result.result);
  }
}
