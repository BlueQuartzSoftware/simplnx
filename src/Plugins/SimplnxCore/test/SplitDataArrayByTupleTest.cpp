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
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

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
void compareSplitArray(const K& original, const K& split, const std::vector<usize>& srcStart)
{
  const auto& srcShape = original.getTupleShape();
  const auto& destShape = split.getTupleShape();

  const usize rank = srcShape.size();
  const usize comps = original.getNumberOfComponents();

  std::vector<usize> currentIdx(rank, 0); // odometer inside block
  const usize totalDestTuples = std::accumulate(destShape.begin(), destShape.end(), usize{1}, std::multiplies<>());

  for(usize destIdx = 0; destIdx < totalDestTuples; ++destIdx)
  {
    usize srcIdx = Indexing::Flatten(srcStart, srcShape) + Indexing::Flatten(currentIdx, srcShape);

    for(usize c = 0; c < comps; ++c)
    {
      REQUIRE(original[srcIdx * comps + c] == split[destIdx * comps + c]);
    }

    // Advance currentIdx as if it were an odometer
    Indexing::IncrementLikeOdometer(currentIdx, destShape);
  }
}

std::string makeArrayName(const std::string& targetName, int32 index, int32 totalSize)
{
  return targetName + "_" + StringUtilities::GenerateIndexString(index, totalSize);
}

template <typename T>
void updateValue(T& value)
{
  value++;
}

template <>
void updateValue(bool& value)
{
  value = !value;
}

template <typename T>
void fillDataArray(DataArray<T>& inputArray)
{
  const usize numComps = inputArray.getNumberOfComponents();
  T value = 0;
  for(int32 i = 0; i < inputArray.getNumberOfTuples(); i++)
  {
    for(int c = 0; c < numComps; ++c)
    {
      inputArray[i * numComps + c] = value;
      updateValue(value);
    }
  }
}

template <typename T>
DataStructure createDataStructure(const std::vector<usize>& tupleShape, const std::vector<usize>& compShape)
{
  DataStructure ds;

  auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, tupleShape);

  auto* array = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, k_DataArrayName, tupleShape, compShape, am->getId());
  fillDataArray<T>(*array);

  return ds;
}
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::SplitDataArrayByTupleFilter", "[SimplnxCore][SplitDataArrayByTupleFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64)
{
  using T = TestType;

  UnitTest::LoadPlugins();
  SplitDataArrayByTupleFilter filter;

  Arguments args;
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));

  SECTION("Valid – New Data Group")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType splitCounts = {{5}, {2}, {3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify output arrays
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)}));
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)}));
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)}));

    REQUIRE(arr1.getNumberOfTuples() == 5);
    REQUIRE(arr1.getNumberOfComponents() == 2);
    REQUIRE(arr2.getNumberOfTuples() == 2);
    REQUIRE(arr2.getNumberOfComponents() == 2);
    REQUIRE(arr3.getNumberOfTuples() == 3);
    REQUIRE(arr3.getNumberOfComponents() == 2);

    compareSplitArray(inputArray, arr1, {0});
    compareSplitArray(inputArray, arr2, {5});
    compareSplitArray(inputArray, arr3, {7});

    // Original array still present
    REQUIRE(ds.getDataAs<IDataArray>(k_InputArrayPath) != nullptr);
  }

  SECTION("Valid – Existing Data Group")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});
    DataGroup::Create(ds, k_ExistingDGName);

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(true));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup)));
    DynamicTableParameter::ValueType splitCounts = {{5}, {2}, {3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingDataGroupPath, std::make_any<DataPath>(DataPath({k_ExistingDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    // Copy input array so that we can check result after execute is over (inputArray is getting deleted)
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);
    const auto& inputDataStore = inputArray.getDataStoreRef();
    std::vector<T> inputArrayVals;
    std::transform(inputArray.begin(), inputArray.end(), std::back_inserter(inputArrayVals), [](auto val) { return val; });

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify output arrays
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 1, 3)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 1, 3)}));
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 2, 3)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 2, 3)}));
    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 3, 3)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 3, 3)}));

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

    REQUIRE(ds.getDataAs<IDataArray>(k_InputArrayPath) == nullptr); // deleted
  }

  SECTION("Valid – New Attribute Matrix")
  {
    DataStructure ds = createDataStructure<T>({4, 8, 4}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType amTupleShape = {{4, 8, 2}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(amTupleShape));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_NewAMName})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<uint64>(2));

    // Preflight + execute
    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({k_NewAMName})));
    const auto& splitAM = ds.getDataRefAs<AttributeMatrix>(DataPath({k_NewAMName}));
    REQUIRE(splitAM.getShape() == std::vector<usize>{4, 8, 2});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, makeArrayName(k_DataArrayName, 1, 2)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, makeArrayName(k_DataArrayName, 1, 2)}));
    REQUIRE(arr1.getTupleShape() == std::vector<usize>{4, 8, 2});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, makeArrayName(k_DataArrayName, 2, 2)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewAMName, makeArrayName(k_DataArrayName, 2, 2)}));
    REQUIRE(arr2.getTupleShape() == std::vector<usize>{4, 8, 2});

    compareSplitArray(inputArray, arr1, {0, 0, 0});
    compareSplitArray(inputArray, arr2, {0, 0, 2});
  }

  SECTION("Valid – Existing Attribute Matrix")
  {
    DataStructure ds = createDataStructure<T>({10, 15, 20}, {2});
    AttributeMatrix::Create(ds, k_ExistingAMName, {10, 3, 20});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_ExistingAMName})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<uint64>(1));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({k_ExistingAMName})));
    const auto& splitAM = ds.getDataRefAs<AttributeMatrix>(DataPath({k_ExistingAMName}));
    REQUIRE(splitAM.getShape() == std::vector<usize>{10, 3, 20});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 1, 5)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 1, 5)}));
    REQUIRE(arr1.getTupleShape() == std::vector<usize>{10, 3, 20});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 2, 5)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 2, 5)}));
    REQUIRE(arr2.getTupleShape() == std::vector<usize>{10, 3, 20});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 3, 5)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 3, 5)}));
    REQUIRE(arr3.getTupleShape() == std::vector<usize>{10, 3, 20});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 4, 5)})));
    const auto& arr4 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 4, 5)}));
    REQUIRE(arr4.getTupleShape() == std::vector<usize>{10, 3, 20});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 5, 5)})));
    const auto& arr5 = ds.getDataRefAs<DataArray<T>>(DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, 5, 5)}));
    REQUIRE(arr5.getTupleShape() == std::vector<usize>{10, 3, 20});

    compareSplitArray(inputArray, arr1, {0, 0, 0});
    compareSplitArray(inputArray, arr2, {0, 3, 0});
    compareSplitArray(inputArray, arr3, {0, 6, 0});
    compareSplitArray(inputArray, arr4, {0, 9, 0});
    compareSplitArray(inputArray, arr5, {0, 12, 0});
  }

  SECTION("Valid – 2D Tuple Shape")
  {
    DataStructure ds = createDataStructure<T>({10, 15}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    DynamicTableParameter::ValueType splitCounts = {{5}, {6}, {4}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(1));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)}));
    REQUIRE(arr1.getTupleShape() == std::vector<usize>{10, 5});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)}));
    REQUIRE(arr2.getTupleShape() == std::vector<usize>{10, 6});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)}));
    REQUIRE(arr3.getTupleShape() == std::vector<usize>{10, 4});

    UnitTest::WriteTestDataStructure(ds, "/tmp/output.dream3d");

    compareSplitArray(inputArray, arr1, {0, 0});
    compareSplitArray(inputArray, arr2, {0, 5});
    compareSplitArray(inputArray, arr3, {0, 11});
  }

  SECTION("Valid – 3D Tuple Shape")
  {
    DataStructure ds = createDataStructure<T>({10, 10, 10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    DynamicTableParameter::ValueType splitCounts = {{7}, {2}, {1}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(2));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)}));
    REQUIRE(arr1.getTupleShape() == std::vector<usize>{10, 10, 7});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)}));
    REQUIRE(arr2.getTupleShape() == std::vector<usize>{10, 10, 2});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)}));
    REQUIRE(arr3.getTupleShape() == std::vector<usize>{10, 10, 1});

    compareSplitArray(inputArray, arr1, {0, 0, 0});
    compareSplitArray(inputArray, arr2, {0, 0, 7});
    compareSplitArray(inputArray, arr3, {0, 0, 9});
  }

  SECTION("Valid – NeighborList")
  {
    DataStructure ds;
    auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10});
    auto* nl = NeighborList<T>::Create(ds, "Input NL", 10, am->getId());
    for(usize i = 0; i < 10; ++i)
    {
      nl->setList(i, std::vector<T>{static_cast<T>(i), static_cast<T>(i + 1)});
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

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify results
    auto* outNL1 = ds.getDataAs<NeighborList<T>>(DataPath({k_NewDGName, "Input NL_1"}));
    auto* outNL2 = ds.getDataAs<NeighborList<T>>(DataPath({k_NewDGName, "Input NL_2"}));

    REQUIRE(outNL1 != nullptr);
    REQUIRE(outNL2 != nullptr);
    REQUIRE(outNL1->getNumberOfLists() == 4);
    REQUIRE(outNL2->getNumberOfLists() == 6);

    for(usize i = 0; i < 4; ++i)
    {
      REQUIRE(outNL1->getList(i) == std::vector<T>{static_cast<T>(i), static_cast<T>(i + 1)});
    }
    for(usize i = 0; i < 6; ++i)
    {
      REQUIRE(outNL2->getList(i) == std::vector<T>{static_cast<T>(i + 4), static_cast<T>(i + 5)});
    }

    // Original NeighborList still exists
    REQUIRE(ds.getDataAs<NeighborList<T>>(DataPath({k_AttributeMatrixName, "Input NL"})) != nullptr);
  }

  SECTION("Valid – String Array")
  {
    DataStructure ds;
    auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10});
    auto strings = StringArray::collection_type();
    strings.reserve(10);
    for(usize i = 0; i < 10; ++i)
    {
      strings.push_back(fmt::format("str{}", i));
    }
    auto* strArray = StringArray::CreateWithValues(ds, "Input Strings", strings, am->getId());

    // Execute filter
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath({k_AttributeMatrixName, "Input Strings"})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    DynamicTableParameter::ValueType splitCounts = {{4}, {6}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(splitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify results
    auto* outSA1 = ds.getDataAs<StringArray>(DataPath({k_NewDGName, "Input Strings_1"}));
    auto* outSA2 = ds.getDataAs<StringArray>(DataPath({k_NewDGName, "Input Strings_2"}));

    REQUIRE(outSA1 != nullptr);
    REQUIRE(outSA2 != nullptr);
    REQUIRE(outSA1->getNumberOfTuples() == 4);
    REQUIRE(outSA2->getNumberOfTuples() == 6);

    for(usize i = 0; i < 4; ++i)
    {
      REQUIRE((*outSA1)[i] == (*strArray)[i]);
    }
    for(usize i = 0; i < 6; ++i)
    {
      REQUIRE((*outSA2)[i] == (*strArray)[i + 4]);
    }

    // Original StringArray still exists
    REQUIRE(ds.getDataAs<StringArray>(DataPath({k_AttributeMatrixName, "Input Strings"})) != nullptr);
  }

  SECTION("Invalid – Tuple shape contains non‑positive value")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType badSplitCounts = {{5}, {0}, {5}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(badSplitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitCountLessThanZero));
  }

  SECTION("Invalid – Tuple shapes do not sum to input tuple shape")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType badSplitCounts = {{5}, {2}, {4}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimension_Key, std::make_any<NumberParameter<uint64>::ValueType>(0));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitDimensionCounts_Key, std::make_any<DynamicTableParameter::ValueType>(badSplitCounts));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::SplitCountSumNotEqual));
  }

  SECTION("Invalid – New Attribute Matrix tuple shape contains non‑positive value")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{0}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNegative));
  }

  SECTION("Invalid – New Attribute Matrix tuple shape does not divide input in only one dimension")
  {
    DataStructure ds = createDataStructure<T>({10, 15}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{2, 3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier));
  }

  SECTION("Invalid – Existing Attribute Matrix tuple shape does not divide input")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});
    AttributeMatrix::Create(ds, k_BadExistingAMName, {3});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadExistingAMName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNoCommonMultiplier));
  }

  SECTION("Invalid – New Attribute Matrix tuple shape has negative value")
  {
    DataStructure ds = createDataStructure<T>({10}, {2});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{-1}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == to_underlying(SplitDataArrayByTuple::ErrorCodes::AttrMatrixTupleShapeNegative));
  }
}
