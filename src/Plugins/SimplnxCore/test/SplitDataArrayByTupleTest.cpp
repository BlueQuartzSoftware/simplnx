
#include "SimplnxCore/Filters/Algorithms/SplitDataArrayByTuple.hpp"
#include "SimplnxCore/Filters/SplitDataArrayByTupleFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

/**
 * Helpers to generate a simple DataStructure and fill some arrays
 * ---------------------------------------------------------------
 */
namespace
{
// ===== Constants used throughout the tests =====
inline const std::string k_AttributeMatrixName = "AttributeMatrix";
inline const std::string k_DataArrayName = "MultiComponent Array";
inline const std::string k_NewDGName = "SplitDG";
inline const std::string k_ExistingDGName = "ExistingGroup";
inline const std::string k_NewAMName = "SplitAM";
inline const std::string k_ExistingAMName = "ExistingAM";
inline const std::string k_BadDGName = "BadDG";
inline const std::string k_BadAMName = "BadAM";
inline const std::string k_BadExistingAMName = "BadExistingAM";

template <class K>
void compareSplitArray(const K& original, const K& split, const std::vector<usize>& srcStart)
{
  const auto& srcShape = original.getTupleShape();
  const auto& blkShape = split.getTupleShape(); // == extent

  const usize rank = srcShape.size();
  const usize comps = original.getNumberOfComponents();

  // ----- build strides (row-major: last dim fastest) -----
  std::vector<usize> stride(rank, 1);
  for(usize d = rank; d-- > 1;)
    stride[d - 1] = stride[d] * srcShape[d];

  auto flatten = [&](const std::vector<usize>& idx) -> usize {
    usize off = 0;
    for(usize d = 0; d < rank; ++d)
      off += idx[d] * stride[d];
    return off;
  };

  std::vector<usize> rel(rank, 0); // odometer inside block
  const usize tuplesToCmp = std::accumulate(blkShape.begin(), blkShape.end(), usize{1}, std::multiplies<>());

  for(usize n = 0; n < tuplesToCmp; ++n)
  {
    usize srcLin = flatten(srcStart) + flatten(rel);
    usize dstLin = n; // split array is contiguous

    for(usize c = 0; c < comps; ++c)
    {
      REQUIRE(original[srcLin * comps + c] == split[dstLin * comps + c]);
    }

    // advance odometer
    for(usize d = rank; d-- > 0;)
    {
      if(++rel[d] < blkShape[d])
      {
        break;
      }
      rel[d] = 0;
    }
  }
}

// Helper: format "<base>_<NN>" with optional width (default 2)
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
DataStructure createDataStructure()
{
  DataStructure ds;

  auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10});

  auto* array = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, k_DataArrayName, std::vector<usize>{10}, std::vector<usize>{2}, am->getId());
  fillDataArray<T>(*array);

  return ds;
}

template <typename T>
DataStructure createDataStructure2D()
{
  DataStructure ds;

  auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10, 15});
  auto* array = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, k_DataArrayName, std::vector<usize>{10, 15}, std::vector<usize>{1}, am->getId());
  fillDataArray<T>(*array);

  return ds;
}

template <typename T>
DataStructure createDataStructure3D()
{
  DataStructure ds;

  auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10, 10, 10});

  auto* array = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, k_DataArrayName, std::vector<usize>{10, 10, 10}, std::vector<usize>{2}, am->getId());
  fillDataArray<T>(*array);

  return ds;
}

template <typename T>
void checkArray(const DataArray<T>& inputArr, const std::array<usize, 3> inputStrides, const DataArray<T>& outArr, const std::vector<usize>& outShape, const std::vector<usize>& startOffset)
{
  const std::array<usize, 3> outStrides = {1, outShape[0], outShape[0] * outShape[1]};
  const usize numComps = outArr.getNumberOfComponents();

  for(usize z = 0; z < outShape[2]; ++z)
  {
    for(usize y = 0; y < outShape[1]; ++y)
    {
      for(usize x = 0; x < outShape[0]; ++x)
      {
        usize localTupleIdx = x * outStrides[0] + y * outStrides[1] + z * outStrides[2];
        usize gX = startOffset[0] + x;
        usize gY = startOffset[1] + y;
        usize gZ = startOffset[2] + z;
        usize globalTupleIdx = gX * inputStrides[0] + gY * inputStrides[1] + gZ * inputStrides[2];

        for(usize c = 0; c < numComps; ++c)
        {
          REQUIRE(outArr[localTupleIdx * numComps + c] == inputArr[globalTupleIdx * numComps + c]);
        }
      }
    }
  }
};

// Convenience constants
const DataPath k_InputArrayPath({k_AttributeMatrixName, k_DataArrayName});
} // namespace

/**
 * Comprehensive unit‑tests exercising all success and failure modes
 * ----------------------------------------------------------------
 */
TEMPLATE_TEST_CASE("SimplnxCore::SplitDataArrayByTupleFilter", "[SimplnxCore][SplitDataArrayByTupleFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64)
{
  using T = TestType;

  UnitTest::LoadPlugins();
  SplitDataArrayByTupleFilter filter;

  Arguments args;
  args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));

  SECTION("Valid – New Data Group")
  {
    DataStructure ds = createDataStructure<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    // Tuple shapes that sum to 10 ⇒ 5 + 2 + 3
    DynamicTableParameter::ValueType tupleShapes = {{5}, {2}, {3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(tupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    // Preflight + execute
    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // Verify output arrays
    for(int i = 1; i <= 3; ++i)
    {
      REQUIRE(ds.getDataAs<IDataArray>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, i, 3)})) != nullptr);
    }

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

    REQUIRE(arr1[0] == inputArray[0]);
    REQUIRE(arr1[1] == inputArray[1]);
    REQUIRE(arr1[2] == inputArray[2]);
    REQUIRE(arr1[3] == inputArray[3]);
    REQUIRE(arr1[4] == inputArray[4]);
    REQUIRE(arr1[5] == inputArray[5]);
    REQUIRE(arr1[6] == inputArray[6]);
    REQUIRE(arr1[7] == inputArray[7]);
    REQUIRE(arr1[8] == inputArray[8]);
    REQUIRE(arr1[9] == inputArray[9]);

    REQUIRE(arr2[0] == inputArray[10]);
    REQUIRE(arr2[1] == inputArray[11]);
    REQUIRE(arr2[2] == inputArray[12]);
    REQUIRE(arr2[3] == inputArray[13]);

    REQUIRE(arr3[0] == inputArray[14]);
    REQUIRE(arr3[1] == inputArray[15]);
    REQUIRE(arr3[2] == inputArray[16]);
    REQUIRE(arr3[3] == inputArray[17]);
    REQUIRE(arr3[4] == inputArray[18]);
    REQUIRE(arr3[5] == inputArray[19]);

    // Original array still present
    REQUIRE(ds.getDataAs<IDataArray>(k_InputArrayPath) != nullptr);
  }

  SECTION("Valid – Existing Data Group + delete original")
  {
    DataStructure ds = createDataStructure<T>();

    // Pre‑create a DataGroup to receive the split arrays
    DataGroup::Create(ds, k_ExistingDGName);

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(true));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingDataGroup)));
    DynamicTableParameter::ValueType tupleShapes = {{5}, {2}, {3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(tupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingDataGroupPath, std::make_any<DataPath>(DataPath({k_ExistingDGName})));

    // Preflight + execute
    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE(ds.getDataAs<IDataArray>(DataPath({k_ExistingDGName, makeArrayName(k_DataArrayName, 1, 3)})) != nullptr);
    REQUIRE(ds.getDataAs<IDataArray>(k_InputArrayPath) == nullptr); // deleted
  }

  SECTION("Valid – New Attribute Matrix")
  {
    DataStructure ds = createDataStructure<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType amTupleShape = {{2}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(amTupleShape));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_NewAMName})));

    // Preflight + execute
    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    const auto* splitAM = ds.getDataAs<AttributeMatrix>(DataPath({k_NewAMName}));
    REQUIRE(splitAM != nullptr);
    REQUIRE(splitAM->getShape()[0] == 2);

    // 5 arrays expected (10 / 2)
    for(int i = 1; i <= 5; ++i)
    {
      auto path = DataPath({k_NewAMName, makeArrayName(k_DataArrayName, i, 5)});
      REQUIRE(ds.getDataAs<IDataArray>(path) != nullptr);
    }
  }

  SECTION("Valid – Existing Attribute Matrix")
  {
    DataStructure ds = createDataStructure<T>();
    // Create an AttributeMatrix with tuple shape {2} so it divides 10
    AttributeMatrix* existingAM = AttributeMatrix::Create(ds, k_ExistingAMName, {2});
    (void)existingAM;

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_ExistingAMName})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NumOfAttrMatrixSplitArrays_Key, std::make_any<uint64>(5));

    // Preflight + execute
    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    // 5 arrays expected inside ExistingAM
    for(int i = 1; i <= 5; ++i)
    {
      auto path = DataPath({k_ExistingAMName, makeArrayName(k_DataArrayName, i, 5)});
      REQUIRE(ds.getDataAs<IDataArray>(path) != nullptr);
    }
  }

  SECTION("Valid – 2D Tuple Shape")
  {
    DataStructure ds = createDataStructure2D<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    // Tuple shapes that element‑wise sum to {10,15}  →  (2,5) + (4,6) + (4,4)
    DynamicTableParameter::ValueType tupleShapes = {{2, 5}, {4, 6}, {4, 4}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(tupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)}));
    REQUIRE(arr1.getTupleShape() == std::vector<usize>{2, 5});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)}));
    REQUIRE(arr2.getTupleShape() == std::vector<usize>{4, 6});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)}));
    REQUIRE(arr3.getTupleShape() == std::vector<usize>{4, 4});

    compareSplitArray(inputArray, arr1, {0, 0});
    compareSplitArray(inputArray, arr2, {2, 5});
    compareSplitArray(inputArray, arr3, {6, 11});
  }

  SECTION("Valid – 3D Tuple Shape")
  {
    DataStructure ds = createDataStructure3D<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    // Tuple shapes that element‑wise sum to {10,10,10}  →  (2,5,7) + (4,1,2) + (4,4,1)
    DynamicTableParameter::ValueType tupleShapes = {{2, 5, 7}, {4, 1, 2}, {4, 4, 1}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(tupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_NewDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(k_InputArrayPath));
    const auto& inputArray = ds.getDataRefAs<DataArray<T>>(k_InputArrayPath);

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)})));
    const auto& arr1 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 1, 3)}));
    REQUIRE(arr1.getTupleShape() == std::vector<usize>{2, 5, 7});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)})));
    const auto& arr2 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 2, 3)}));
    REQUIRE(arr2.getTupleShape() == std::vector<usize>{4, 1, 2});

    REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)})));
    const auto& arr3 = ds.getDataRefAs<DataArray<T>>(DataPath({k_NewDGName, makeArrayName(k_DataArrayName, 3, 3)}));
    REQUIRE(arr3.getTupleShape() == std::vector<usize>{4, 4, 1});

    compareSplitArray(inputArray, arr1, {0, 0, 0});
    compareSplitArray(inputArray, arr2, {2, 5, 7});
    compareSplitArray(inputArray, arr3, {6, 6, 9});
  }

  SECTION("Valid – NeighborList")
  {
    // Create test data structure
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

    DynamicTableParameter::ValueType tupleShapes = {{4}, {6}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(tupleShapes));
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

    for(T i = 0; i < 4; ++i)
    {
      REQUIRE(outNL1->getList(i) == std::vector<T>{i, static_cast<T>(i + 1)});
    }
    for(T i = 0; i < 6; ++i)
    {
      REQUIRE(outNL2->getList(i) == std::vector<T>{static_cast<T>(i + 4), static_cast<T>(i + 5)});
    }

    // Original NeighborList remains untouched
    REQUIRE(ds.getDataAs<NeighborList<T>>(DataPath({k_AttributeMatrixName, "Input NL"})) != nullptr);
  }

  SECTION("Valid – String Array")
  {
    // Create test data structure
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

    DynamicTableParameter::ValueType tupleShapes = {{4}, {6}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(tupleShapes));
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

    // Original StringArray still present
    REQUIRE(ds.getDataAs<StringArray>(DataPath({k_AttributeMatrixName, "Input Strings"})) != nullptr);
  }

  SECTION("Invalid – Tuple shape contains non‑positive value")
  {
    DataStructure ds = createDataStructure<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType badTupleShapes = {{5}, {0}, {5}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(badTupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – Tuple shapes do not sum to input tuple shape")
  {
    DataStructure ds = createDataStructure<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));
    DynamicTableParameter::ValueType badTupleShapes = {{5}, {2}, {4}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(badTupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – New Attribute Matrix tuple shape contains non‑positive value")
  {
    DataStructure ds = createDataStructure<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{0}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – Attribute Matrix tuple shape does not divide input")
  {
    DataStructure ds = createDataStructure<T>();

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewAttrMatrix)));
    DynamicTableParameter::ValueType badAMTuple = {{3}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_AttrMatrixTupleShape_Key, std::make_any<DynamicTableParameter::ValueType>(badAMTuple));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadAMName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – Existing Attribute Matrix tuple shape does not divide input")
  {
    DataStructure ds = createDataStructure<T>();
    AttributeMatrix::Create(ds, k_BadExistingAMName, {3});

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::ExistingAttrMatrix)));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_ExistingAttributeMatrixPath, std::make_any<DataPath>(DataPath({k_BadExistingAMName})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NumOfAttrMatrixSplitArrays_Key, std::make_any<uint64>(4));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – StringArray given multi-dimensional tuple shape")
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

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath({k_AttributeMatrixName, "Input Strings MD"})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    // Illegal: rank-2 tuple shape for a rank-1 StringArray
    DynamicTableParameter::ValueType badTupleShapes = {{5, 5}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(badTupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – NeighborList given multi-dimensional tuple shape")
  {
    DataStructure ds;
    auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10});
    auto* nl = NeighborList<T>::Create(ds, "Input NL MD", 10, am->getId());

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath({k_AttributeMatrixName, "Input NL MD"})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    // Illegal rank-2 tuple shape
    DynamicTableParameter::ValueType badTupleShapes = {{4, 6}};
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(badTupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }

  SECTION("Invalid – Tuple shape rank differs from input rank")
  {
    DataStructure ds;
    auto* am = AttributeMatrix::Create(ds, k_AttributeMatrixName, {10, 10});
    auto* arr = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, "Input 2D", std::vector<usize>{10, 10}, std::vector<usize>{1}, am->getId());

    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath({k_AttributeMatrixName, "Input 2D"})));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_OutputContainer,
                        std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(SplitDataArrayByTuple::OutputContainer::NewDataGroup)));

    // Rank-1 tuple-shape list → should fail (rank mismatch)
    DynamicTableParameter::ValueType badTupleShapes = {{25}}; // 25*? not relevant; rank is wrong
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_SplitArraysTupleShapes_Key, std::make_any<DynamicTableParameter::ValueType>(badTupleShapes));
    args.insertOrAssign(SplitDataArrayByTupleFilter::k_NewDataGroupPath, std::make_any<DataPath>(DataPath({k_BadDGName})));

    auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  }
}
