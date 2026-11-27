#include "SimplnxCore/Filters/Algorithms/SplitDataArrayByTuple.hpp"
#include "SimplnxCore/Filters/SplitDataArrayByTupleFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch.hpp>
#include <memory>
#include <nonstd/span.hpp>

#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
inline const std::string k_AttributeMatrixName = "AttributeMatrix";
inline const std::string k_DataArrayName = "MultiComponent Array";
inline const std::string k_NewDGName = "SplitDG";
inline const std::string k_ExistingDGName = "ExistingGroup";
inline const std::string k_NewAMName = "SplitAM";
inline const std::string k_ExistingAMName = "ExistingAM";
inline const std::string k_BadDGName = "BadDG";
inline const std::string k_BadAMName = "BadAM";
inline const std::string k_BadExistingAMName = "BadExistingAM";
const DataPath k_InputArrayPath({k_AttributeMatrixName, k_DataArrayName});

template <class K>
void CompareSplitArray(const K& original, const K& split, const std::vector<usize>& srcStart)
{
  const auto& srcShape = original.getTupleShape();
  const auto& destShape = split.getTupleShape();

  const usize rank = srcShape.size();
  const usize comps = original.getNumberOfComponents();

  std::vector<usize> currentIdx(rank, 0); // odometer inside block
  const usize totalDestTuples = std::accumulate(destShape.begin(), destShape.end(), usize{1}, std::multiplies<>());

  for(usize destIdx = 0; destIdx < totalDestTuples; ++destIdx)
  {
    usize srcIdx = indexing::Flatten(srcStart, srcShape) + indexing::Flatten(currentIdx, srcShape);

    for(usize c = 0; c < comps; ++c)
    {
      REQUIRE(original[srcIdx * comps + c] == split[destIdx * comps + c]);
    }

    // Advance currentIdx as if it were an odometer
    indexing::IncrementLikeOdometer(currentIdx, destShape);
  }
}

std::string MakeArrayName(const std::string& targetName, int32 index, int32 totalSize)
{
  return targetName + "_" + string_utilities::GenerateIndexString(index, totalSize);
}

template <typename T>
void UpdateValue(T& value)
{
  value++;
}

template <>
void UpdateValue(bool& value)
{
  value = !value;
}

template <typename T>
void FillDataArray(DataArray<T>& inputArray)
{
  const usize numComps = inputArray.getNumberOfComponents();
  T value = 0;
  for(int32 i = 0; i < inputArray.getNumberOfTuples(); i++)
  {
    for(int c = 0; c < numComps; ++c)
    {
      inputArray[i * numComps + c] = value;
      UpdateValue(value);
    }
  }
}

template <typename T>
DataStructure CreateDataStructure(const ShapeType& tupleShape, const ShapeType& compShape)
{
  DataStructure dataStructure;

  auto* amPtr = AttributeMatrix::Create(dataStructure, k_AttributeMatrixName, tupleShape);

  auto* arrayPtr = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_DataArrayName, tupleShape, compShape, amPtr->getId());
  FillDataArray<T>(*arrayPtr);

  return dataStructure;
}
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::SplitDataArrayByTupleFilter - Valid Execution", "[SimplnxCore][SplitDataArrayByTupleFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32,
                   float64)
{
  using T = TestType;

  unit_test::LoadPlugins();
  SplitDataArrayByTupleFilter filter;

  Arguments args;
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));

  SECTION("DataArray")
  {
    const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
    CAPTURE(scenario);
    unit_test::AlgorithmTestScope scope(scenario);

    SECTION("New Data Group")
    {
      DataStructure dataStructure = CreateDataStructure<T>({10}, {2});

      args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                          std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
      DynamicTableParameter::ValueType splitCounts = {{5}, {2}, {3}};
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

      auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

      auto exec = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

      // Verify output arrays
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
      const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 1, 3)})));
      const auto& arr1 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 1, 3)}));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 2, 3)})));
      const auto& arr2 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 2, 3)}));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 3, 3)})));
      const auto& arr3 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 3, 3)}));

      REQUIRE(arr1.getNumberOfTuples() == 5);
      REQUIRE(arr1.getNumberOfComponents() == 2);
      REQUIRE(arr2.getNumberOfTuples() == 2);
      REQUIRE(arr2.getNumberOfComponents() == 2);
      REQUIRE(arr3.getNumberOfTuples() == 3);
      REQUIRE(arr3.getNumberOfComponents() == 2);

      CompareSplitArray(inputArray, arr1, {0});
      CompareSplitArray(inputArray, arr2, {5});
      CompareSplitArray(inputArray, arr3, {7});

      // Original array still present
      REQUIRE(dataStructure.getDataAs<IDataArray>(k_InputArrayPath) != nullptr);

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }

    SECTION("Existing Data Group")
    {
      DataStructure dataStructure = CreateDataStructure<T>({10}, {2});
      DataGroup::Create(dataStructure, k_ExistingDGName);

      args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(true));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                          std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup)));
      DynamicTableParameter::ValueType splitCounts = {{5}, {2}, {3}};
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingDataGroupPath, std::make_any<DataPath>(DataPath({k_ExistingDGName})));

      auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

      // Copy input array so that we can check result after execute is over (inputArray is getting deleted)
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
      const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);
      const auto& inputDataStore = inputArray.getDataStoreRef();
      std::vector<T> inputArrayVals;
      std::transform(inputArray.begin(), inputArray.end(), std::back_inserter(inputArrayVals), [](auto val) { return val; });

      auto exec = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

      // Verify output arrays
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, MakeArrayName(k_DataArrayName, 1, 3)})));
      const auto& arr1 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, MakeArrayName(k_DataArrayName, 1, 3)}));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, MakeArrayName(k_DataArrayName, 2, 3)})));
      const auto& arr2 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, MakeArrayName(k_DataArrayName, 2, 3)}));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, MakeArrayName(k_DataArrayName, 3, 3)})));
      const auto& arr3 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, MakeArrayName(k_DataArrayName, 3, 3)}));

      REQUIRE(arr1.getNumberOfTuples() == 5);
      REQUIRE(arr1.getNumberOfComponents() == 2);
      REQUIRE(arr2.getNumberOfTuples() == 2);
      REQUIRE(arr2.getNumberOfComponents() == 2);
      REQUIRE(arr3.getNumberOfTuples() == 3);
      REQUIRE(arr3.getNumberOfComponents() == 2);

      REQUIRE(arr1[0] == inputArrayVals[0]);
      REQUIRE(arr1[1] == inputArrayVals[1]);
      REQUIRE(arr1[2] == inputArrayVals[2]);
      REQUIRE(arr1[3] == inputArrayVals[3]);
      REQUIRE(arr1[4] == inputArrayVals[4]);
      REQUIRE(arr1[5] == inputArrayVals[5]);
      REQUIRE(arr1[6] == inputArrayVals[6]);
      REQUIRE(arr1[7] == inputArrayVals[7]);
      REQUIRE(arr1[8] == inputArrayVals[8]);
      REQUIRE(arr1[9] == inputArrayVals[9]);

      REQUIRE(arr2[0] == inputArrayVals[10]);
      REQUIRE(arr2[1] == inputArrayVals[11]);
      REQUIRE(arr2[2] == inputArrayVals[12]);
      REQUIRE(arr2[3] == inputArrayVals[13]);

      REQUIRE(arr3[0] == inputArrayVals[14]);
      REQUIRE(arr3[1] == inputArrayVals[15]);
      REQUIRE(arr3[2] == inputArrayVals[16]);
      REQUIRE(arr3[3] == inputArrayVals[17]);
      REQUIRE(arr3[4] == inputArrayVals[18]);
      REQUIRE(arr3[5] == inputArrayVals[19]);

      REQUIRE(dataStructure.getDataAs<IDataArray>(k_InputArrayPath) == nullptr); // deleted

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }

    SECTION("New Attribute Matrix")
    {
      DataStructure dataStructure = CreateDataStructure<T>({4, 8, 4}, {2});

      args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                          std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
      DynamicTableParameter::ValueType amTupleShape = {{4, 8, 2}};
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(amTupleShape));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_NewAMName})));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<uint64>(2));

      // Preflight + execute
      auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

      auto exec = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_NewAMName})));
      const auto& splitAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_NewAMName}));
      REQUIRE(splitAM.getShape() == std::vector<usize>{4, 8, 2});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
      const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, MakeArrayName(k_DataArrayName, 1, 2)})));
      const auto& arr1 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, MakeArrayName(k_DataArrayName, 1, 2)}));
      REQUIRE(arr1.getTupleShape() == std::vector<usize>{4, 8, 2});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, MakeArrayName(k_DataArrayName, 2, 2)})));
      const auto& arr2 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, MakeArrayName(k_DataArrayName, 2, 2)}));
      REQUIRE(arr2.getTupleShape() == std::vector<usize>{4, 8, 2});

      CompareSplitArray(inputArray, arr1, {0, 0, 0});
      CompareSplitArray(inputArray, arr2, {0, 0, 2});

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }

    SECTION("Existing Attribute Matrix")
    {
      DataStructure dataStructure = CreateDataStructure<T>({10, 15, 20}, {2});
      AttributeMatrix::Create(dataStructure, k_ExistingAMName, {10, 3, 20});

      args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                          std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_ExistingAMName})));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<uint64>(1));

      auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

      auto exec = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_ExistingAMName})));
      const auto& splitAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_ExistingAMName}));
      REQUIRE(splitAM.getShape() == std::vector<usize>{10, 3, 20});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
      const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 1, 5)})));
      const auto& arr1 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 1, 5)}));
      REQUIRE(arr1.getTupleShape() == std::vector<usize>{10, 3, 20});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 2, 5)})));
      const auto& arr2 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 2, 5)}));
      REQUIRE(arr2.getTupleShape() == std::vector<usize>{10, 3, 20});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 3, 5)})));
      const auto& arr3 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 3, 5)}));
      REQUIRE(arr3.getTupleShape() == std::vector<usize>{10, 3, 20});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 4, 5)})));
      const auto& arr4 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 4, 5)}));
      REQUIRE(arr4.getTupleShape() == std::vector<usize>{10, 3, 20});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 5, 5)})));
      const auto& arr5 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, MakeArrayName(k_DataArrayName, 5, 5)}));
      REQUIRE(arr5.getTupleShape() == std::vector<usize>{10, 3, 20});

      CompareSplitArray(inputArray, arr1, {0, 0, 0});
      CompareSplitArray(inputArray, arr2, {0, 3, 0});
      CompareSplitArray(inputArray, arr3, {0, 6, 0});
      CompareSplitArray(inputArray, arr4, {0, 9, 0});
      CompareSplitArray(inputArray, arr5, {0, 12, 0});

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }

    SECTION("2D Tuple Shape")
    {
      DataStructure dataStructure = CreateDataStructure<T>({10, 15}, {2});

      args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                          std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

      DynamicTableParameter::ValueType splitCounts = {{5}, {6}, {4}};
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(1));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

      auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

      auto exec = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
      const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 1, 3)})));
      const auto& arr1 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 1, 3)}));
      REQUIRE(arr1.getTupleShape() == std::vector<usize>{10, 5});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 2, 3)})));
      const auto& arr2 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 2, 3)}));
      REQUIRE(arr2.getTupleShape() == std::vector<usize>{10, 6});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 3, 3)})));
      const auto& arr3 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 3, 3)}));
      REQUIRE(arr3.getTupleShape() == std::vector<usize>{10, 4});

      CompareSplitArray(inputArray, arr1, {0, 0});
      CompareSplitArray(inputArray, arr2, {0, 5});
      CompareSplitArray(inputArray, arr3, {0, 11});

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }

    SECTION("3D Tuple Shape")
    {
      DataStructure dataStructure = CreateDataStructure<T>({10, 10, 10}, {2});

      args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                          std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

      DynamicTableParameter::ValueType splitCounts = {{7}, {2}, {1}};
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(2));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

      auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

      auto exec = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath));
      const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 1, 3)})));
      const auto& arr1 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 1, 3)}));
      REQUIRE(arr1.getTupleShape() == std::vector<usize>{10, 10, 7});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 2, 3)})));
      const auto& arr2 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 2, 3)}));
      REQUIRE(arr2.getTupleShape() == std::vector<usize>{10, 10, 2});

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 3, 3)})));
      const auto& arr3 = dataStructure.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, MakeArrayName(k_DataArrayName, 3, 3)}));
      REQUIRE(arr3.getTupleShape() == std::vector<usize>{10, 10, 1});

      CompareSplitArray(inputArray, arr1, {0, 0, 0});
      CompareSplitArray(inputArray, arr2, {0, 0, 7});
      CompareSplitArray(inputArray, arr3, {0, 0, 9});

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }
  }

  SECTION("NeighborList")
  {
    DataStructure dataStructure;
    auto* amPtr = AttributeMatrix::Create(dataStructure, k_AttributeMatrixName, {10});
    auto* nlPtr = NeighborList<T>::Create(dataStructure, "Input NL", amPtr->getShape(), amPtr->getId());
    for(usize i = 0; i < 10; ++i)
    {
      nlPtr->setList(i, std::vector<T>{static_cast<T>(i), static_cast<T>(i + 1)});
    }

    // Execute filter
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath({k_AttributeMatrixName, "Input NL"})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    DynamicTableParameter::ValueType splitCounts = {{4}, {6}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

    auto exec = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify results
    auto* outNL1Ptr = dataStructure.getDataAs<NeighborList<T>>(DataPath({k_NewDGName, "Input NL_1"}));
    auto* outNL2Ptr = dataStructure.getDataAs<NeighborList<T>>(DataPath({k_NewDGName, "Input NL_2"}));

    REQUIRE(outNL1Ptr != nullptr);
    REQUIRE(outNL2Ptr != nullptr);
    REQUIRE(outNL1Ptr->getNumberOfLists() == 4);
    REQUIRE(outNL2Ptr->getNumberOfLists() == 6);

    for(usize i = 0; i < 4; ++i)
    {
      REQUIRE(outNL1Ptr->getList(i) == std::vector<T>{static_cast<T>(i), static_cast<T>(i + 1)});
    }
    for(usize i = 0; i < 6; ++i)
    {
      REQUIRE(outNL2Ptr->getList(i) == std::vector<T>{static_cast<T>(i + 4), static_cast<T>(i + 5)});
    }

    // Original NeighborList still exists
    REQUIRE(dataStructure.getDataAs<NeighborList<T>>(DataPath({k_AttributeMatrixName, "Input NL"})) != nullptr);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("String Array")
  {
    DataStructure dataStructure;
    auto* amPtr = AttributeMatrix::Create(dataStructure, k_AttributeMatrixName, {10});
    auto strings = StringArray::CollectionType();
    strings.reserve(10);
    for(usize i = 0; i < 10; ++i)
    {
      strings.push_back(fmt::format("str{}", i));
    }
    auto* strArrayPtr = StringArray::CreateWithValues(dataStructure, "Input Strings", amPtr->getShape(), strings, amPtr->getId());

    // Execute filter
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath({k_AttributeMatrixName, "Input Strings"})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    DynamicTableParameter::ValueType splitCounts = {{4}, {6}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);

    auto exec = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify results
    auto* outSA1Ptr = dataStructure.getDataAs<StringArray>(DataPath({k_NewDGName, "Input Strings_1"}));
    auto* outSA2Ptr = dataStructure.getDataAs<StringArray>(DataPath({k_NewDGName, "Input Strings_2"}));

    REQUIRE(outSA1Ptr != nullptr);
    REQUIRE(outSA2Ptr != nullptr);
    REQUIRE(outSA1Ptr->getNumberOfTuples() == 4);
    REQUIRE(outSA2Ptr->getNumberOfTuples() == 6);

    for(usize i = 0; i < 4; ++i)
    {
      REQUIRE((*outSA1Ptr)[i] == (*strArrayPtr)[i]);
    }
    for(usize i = 0; i < 6; ++i)
    {
      REQUIRE((*outSA2Ptr)[i] == (*strArrayPtr)[i + 4]);
    }

    // Original StringArray still exists
    REQUIRE(dataStructure.getDataAs<StringArray>(DataPath({k_AttributeMatrixName, "Input Strings"})) != nullptr);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::SplitDataArrayByTupleFilter - Invalid Execution", "[SimplnxCore][SplitDataArrayByTupleFilter]")
{
  unit_test::LoadPlugins();
  SplitDataArrayByTupleFilter filter;

  Arguments args;
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));

  SECTION("Tuple shape contains non‑positive value")
  {
    DataStructure dataStructure = CreateDataStructure<uint32>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType badSplitCounts = {{5}, {0}, {5}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(badSplitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    REQUIRE(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitCountLessThanZero));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Tuple shapes do not sum to input tuple shape")
  {
    DataStructure dataStructure = CreateDataStructure<uint32>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType badSplitCounts = {{5}, {2}, {4}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(badSplitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    REQUIRE(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitCountSumNotEqual));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("New Attribute Matrix tuple shape contains non‑positive value")
  {
    DataStructure dataStructure = CreateDataStructure<uint32>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{0}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    REQUIRE(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNegative));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("New Attribute Matrix tuple shape does not divide input in only one dimension")
  {
    DataStructure dataStructure = CreateDataStructure<uint32>({10, 15}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{2, 3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    REQUIRE(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Existing Attribute Matrix tuple shape does not divide input")
  {
    DataStructure dataStructure = CreateDataStructure<uint32>({10}, {2});
    AttributeMatrix::Create(dataStructure, k_BadExistingAMName, {3});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadExistingAMName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    REQUIRE(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("New Attribute Matrix tuple shape has non-positive value")
  {
    DataStructure dataStructure = CreateDataStructure<uint32>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{-1}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    REQUIRE(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNegative));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::SplitDataArrayByTuple: Dimension RED fractional tables", "[SimplnxCore][SplitDataArrayByTuple][DimensionRed]")
{
  const bool matrix = GENERATE(false, true);
  SplitDataArrayByTupleFilter filter;
  auto ds = CreateDataStructure<int32>({matrix ? usize{4} : usize{2}}, {1});
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer, std::make_any<uint64>(matrix ? 2 : 0));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({"Split"})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({"Split"})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.5}, {1.5}}));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2.5}}));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors()[0].code == -79000);
}

TEST_CASE("SimplnxCore::SplitDataArrayByTuple: Dimension fixed-only boundaries and sum overflow", "[SimplnxCore][SplitDataArrayByTuple]")
{
  SplitDataArrayByTupleFilter filter;
  auto ds = CreateDataStructure<int32>({4}, {1});
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({"Split"})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({"Split"})));
  SECTION("Scalar validation")
  {
    const bool matrix = GENERATE(false, true);
    const float64 value = GENERATE(-1.0, 0.0, -std::numeric_limits<float64>::infinity(), 1.5, std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), std::ldexp(1.0, 64));
    CAPTURE(matrix, value);
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer, std::make_any<uint64>(matrix ? 2 : 0));
    args.insertOrAssign(matrix ? SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key : SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key,
                        std::make_any<DynamicTableParameter::ValueType>(matrix ? DynamicTableParameter::ValueType{{value}} : DynamicTableParameter::ValueType{{value}, {1}}));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors()[0].code == (value <= 0 ? (matrix ? -65405 : -65403) : -79000));
    REQUIRE(result.output_actions.errors()[0].message.find("index 0") != std::string::npos);
  }
  SECTION("Sum, not product, overflows")
  {
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer, std::make_any<uint64>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key,
                        std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{std::ldexp(1.0, 63)}, {std::ldexp(1.0, 63)}}));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors()[0].code == -79005);
    REQUIRE(result.output_actions.errors()[0].message.find("plus preceding count") != std::string::npos);
  }
  SECTION("Attribute matrix product")
  {
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer, std::make_any<uint64>(2));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{4294967296.0, 4294967296.0}}));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors()[0].code == -79001);
  }
}

TEST_CASE("SimplnxCore::SplitDataArrayByTuple: Dimension inactive tables and exact output", "[SimplnxCore][SplitDataArrayByTuple]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const bool matrix = GENERATE(false, true);
  SplitDataArrayByTupleFilter filter;
  auto ds = CreateDataStructure<int32>({4}, {1});
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer, std::make_any<uint64>(matrix ? 2 : 0));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({"Split"})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({"Split"})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key,
                      std::make_any<DynamicTableParameter::ValueType>(matrix ? DynamicTableParameter::ValueType{{-1}} : DynamicTableParameter::ValueType{{2}, {2}}));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{matrix ? 2.0 : -1.0}}));
  auto result = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  for(usize outputIndex = 0; outputIndex < 2; ++outputIndex)
  {
    const DataPath path({"Split", MakeArrayName(k_DataArrayName, static_cast<int32>(outputIndex + 1), 2)});
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32ArrayType>(path));
    const auto& array = ds.getDataRefAs<Int32ArrayType>(path);
    REQUIRE(array.getTupleShape() == ShapeType{2});
    REQUIRE(array[0] == static_cast<int32>(outputIndex * 2));
    REQUIRE(array[1] == static_cast<int32>(outputIndex * 2 + 1));
  }
  unit_test::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::SplitDataArrayByTupleFilter - tuple rank validation", "[SimplnxCore][SplitDataArrayByTupleFilter]")
{
  const auto container = GENERATE(SplitDataArrayByTuple::OutputContainer::NewDataGroup, SplitDataArrayByTuple::OutputContainer::ExistingDataGroup,
                                  SplitDataArrayByTuple::OutputContainer::NewAttrMatrix, SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix);
  CAPTURE(container);
  SplitDataArrayByTupleFilter filter;
  Arguments args;
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(container)));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingDataGroupPath, std::make_any<DataPath>(DataPath({k_ExistingDGName})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_NewAMName})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_ExistingAMName})));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1}, {1}}));
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1}}));

  SECTION("Split dimension equals input rank")
  {
    auto ds = CreateDataStructure<uint8>({1}, {1});
    DataGroup::Create(ds, k_ExistingDGName);
    AttributeMatrix::Create(ds, k_ExistingAMName, {1});
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(1));
    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    CHECK(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitDimOutOfRange));
    CHECK(preflight.output_actions.errors()[0].message ==
          fmt::format("The chosen Split Dimension (1) is not a dimension of the input array '{}', which has tuple shape (1). Enter a Split Dimension from 0 to 0.", k_InputArrayPath.toString()));
    CHECK(std::any_of(preflight.output_values.cbegin(), preflight.output_values.cend(), [](const IFilter::PreflightValue& value) { return value.name == "Input Array"; }));
    auto execution = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(execution.result);
    REQUIRE(execution.result.errors().size() == 1);
    CHECK(execution.result.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitDimOutOfRange));
  }

  SECTION("Input has no tuple dimensions")
  {
    auto ds = CreateDataStructure<uint8>({}, {1});
    DataGroup::Create(ds, k_ExistingDGName);
    AttributeMatrix::Create(ds, k_ExistingAMName, {1});
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    const auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().size() == 1);
    CHECK(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitDimOutOfRange));
    CHECK(preflight.output_actions.errors()[0].message ==
          fmt::format("The chosen Split Dimension (0) is not a dimension of the input array '{}', which has tuple shape (). Select an input array with at least one tuple dimension.",
                      k_InputArrayPath.toString()));
  }

  SECTION("Output rank is smaller than the valid input split dimension")
  {
    if(container == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix || container == SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)
    {
      auto ds = CreateDataStructure<uint8>({1, 1}, {1});
      AttributeMatrix::Create(ds, k_ExistingAMName, {1});
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(1));
      auto preflight = filter.preflight(ds, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
      REQUIRE(preflight.output_actions.errors().size() == 1);
      CHECK(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier));
      INFO(preflight.output_actions.errors()[0].message);
      CHECK(preflight.output_actions.errors()[0].message.find("The selected tuple shape has 1 and the input array tuple shape has 2.") != std::string::npos);
    }
  }

  SECTION("Selected value does not divide the input value")
  {
    if(container == SplitDataArrayByTuple::OutputContainer::NewAttrMatrix || container == SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)
    {
      auto ds = CreateDataStructure<uint8>({3}, {1});
      AttributeMatrix::Create(ds, k_ExistingAMName, {2});
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{2}}));
      args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
      auto preflight = filter.preflight(ds, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
      REQUIRE(preflight.output_actions.errors().size() == 1);
      CHECK(preflight.output_actions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier));
      INFO(preflight.output_actions.errors()[0].message);
      CHECK(preflight.output_actions.errors()[0].message.find("The input array tuple shape has the value 3 at dimension 0. This value cannot be divided evenly by the selected tuple shape value 2.") !=
            std::string::npos);
    }
  }
}
