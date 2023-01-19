#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateAttributeMatrixFilter.hpp"

#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;

TEST_CASE("ComplexCore::CreateAttributeMatrixFilter(Instantiate)", "[ComplexCore][CreateAttributeMatrixFilter]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};
  static const DataPath k_DataPath({"foo"});

  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insert(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(k_DataPath));
  args.insert(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CreateAttributeMatrixFilter(Invalid Parameters)", "[ComplexCore][CreateAttributeMatrixFilter]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};

  CreateAttributeMatrixFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Section 1")
  {
    args.insert(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath{}));
    args.insert(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
  }

  SECTION("Section 2")
  {
    AttributeMatrix* attMat1 = AttributeMatrix::Create(dataStructure, "AttributeMatrix1");
    args.insert(CreateAttributeMatrixFilter::k_DataObjectPath, std::make_any<DataPath>(DataPath({"AttributeMatrix1", "AttributeMatrix2"})));
    args.insert(CreateAttributeMatrixFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
  }

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(result.result);
}
