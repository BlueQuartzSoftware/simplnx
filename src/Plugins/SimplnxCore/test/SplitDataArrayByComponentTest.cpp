#include "SimplnxCore/Filters/SplitDataArrayByComponentFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{

void FillDataArray(BoolArrayType* inputArray)
{
  usize numComps = inputArray->getNumberOfComponents();
  bool num = false;
  for(int i = 0; i < inputArray->getNumberOfTuples(); i++)
  {
    (*inputArray)[i * numComps] = num;
    (*inputArray)[i * numComps + 1] = num;
    (*inputArray)[i * numComps + 2] = num;
    num = true;
    (*inputArray)[i * numComps + 3] = num;
    num = true;
    (*inputArray)[i * numComps + 4] = num;
    num = true;
  }
}

template <typename T>
void FillDataArray(DataArray<T>* inputArray)
{
  usize numComps = inputArray->getNumberOfComponents();
  T num = 0;
  for(int i = 0; i < inputArray->getNumberOfTuples(); i++)
  {
    (*inputArray)[i * numComps] = num;
    num++;
    (*inputArray)[i * numComps + 1] = num;
    num++;
    (*inputArray)[i * numComps + 2] = num;
    num++;
    (*inputArray)[i * numComps + 3] = num;
    num++;
    (*inputArray)[i * numComps + 4] = num;
    num++;
  }
}

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  AttributeMatrix* am1Ptr = AttributeMatrix::Create(dataStructure, "AttributeMatrix", {10});

  UInt32ArrayType* mcArray1Ptr =
      UInt32ArrayType::CreateWithStore<DataStore<uint32>>(dataStructure, "MultiComponent Array uint32", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<uint32>(mcArray1Ptr);

  BoolArrayType* mcArray2Ptr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "MultiComponent Array bool", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray(mcArray2Ptr);

  Int8ArrayType* mcArray4Ptr = Int8ArrayType::CreateWithStore<DataStore<int8>>(dataStructure, "MultiComponent Array int8", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<int8>(mcArray4Ptr);

  UInt8ArrayType* mcArray5Ptr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "MultiComponent Array uint8", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<uint8>(mcArray5Ptr);

  Int16ArrayType* mcArray6Ptr = Int16ArrayType::CreateWithStore<DataStore<int16>>(dataStructure, "MultiComponent Array int16", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<int16>(mcArray6Ptr);

  UInt16ArrayType* mcArray7Ptr =
      UInt16ArrayType::CreateWithStore<DataStore<uint16>>(dataStructure, "MultiComponent Array uint16", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<uint16>(mcArray7Ptr);

  Int32ArrayType* mcArray8Ptr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "MultiComponent Array int32", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<int32>(mcArray8Ptr);

  Int64ArrayType* mcArray9Ptr = Int64ArrayType::CreateWithStore<DataStore<int64>>(dataStructure, "MultiComponent Array int64", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<int64>(mcArray9Ptr);

  UInt64ArrayType* mcArray10Ptr =
      UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "MultiComponent Array uint64", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<uint64>(mcArray10Ptr);

  Float32ArrayType* mcArray11Ptr =
      Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, "MultiComponent Array float", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<float32>(mcArray11Ptr);

  Float64ArrayType* mcArray12Ptr =
      Float64ArrayType::CreateWithStore<DataStore<float64>>(dataStructure, "MultiComponent Array double", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1Ptr->getId());
  FillDataArray<float64>(mcArray12Ptr);

  return dataStructure;
}

template <typename T>
void TestSplitByType(DataStructure& dataStructure, const std::string& dataType, const DynamicTableInfo::RowType& extractComps = {})
{
  SplitDataArrayByComponentFilter filter;

  DataPath arrayPath({"AttributeMatrix", "MultiComponent Array " + dataType});
  ShapeType compsToCheck;

  Arguments args;
  // Load the exemplar shift data file.
  args.insertOrAssign(SplitDataArrayByComponentFilter::k_MultiCompArrayPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(SplitDataArrayByComponentFilter::k_Postfix_Key, std::make_any<std::string>("Component"));
  args.insertOrAssign(SplitDataArrayByComponentFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
  if(!extractComps.empty())
  {
    args.insertOrAssign(SplitDataArrayByComponentFilter::k_SelectComponents_Key, std::make_any<bool>(true));
    args.insertOrAssign(SplitDataArrayByComponentFilter::k_ComponentsToExtract_Key, std::make_any<DynamicTableParameter::ValueType>({extractComps}));
    for(const auto& comp : extractComps)
    {
      compsToCheck.push_back(static_cast<usize>(comp));
    }
  }
  else
  {
    args.insertOrAssign(SplitDataArrayByComponentFilter::k_SelectComponents_Key, std::make_any<bool>(false));
    for(usize i = 0; i < 5; ++i)
    {
      compsToCheck.push_back(i);
    }
  }

  auto executeResults = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResults.result);

  using DataArrayPtrType = DataArray<T>*;

  DataArray<T>& mcArrayOriginal = dataStructure.getDataRefAs<DataArray<T>>(DataPath({"AttributeMatrix", "MultiComponent Array " + dataType}));
  std::map<usize, DataArrayPtrType> mcArraysAfterSplit;
  for(const auto& comp : compsToCheck)
  {
    DataArrayPtrType mcArrayPtr = dataStructure.getDataAs<DataArray<T>>(DataPath({"AttributeMatrix", "MultiComponent Array " + dataType + "Component" + string_utilities::Number(comp)}));
    mcArraysAfterSplit[comp] = mcArrayPtr;
  }

  usize numTuples = mcArrayOriginal.getNumberOfTuples();
  usize numComps = mcArrayOriginal.getNumberOfComponents();
  for(int i = 0; i < numTuples; i++)
  {
    for(const auto& j : compsToCheck)
    {
      T originalValue = mcArrayOriginal[i * numComps + j];
      T afterSplitValue = (*mcArraysAfterSplit[j])[i];
      REQUIRE(originalValue == afterSplitValue);
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::SplitDataArrayByComponent", "[SimplnxCore][SplitDataArrayByComponentFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  unit_test::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure();

  scope.execute([&] { TestSplitByType<uint32>(dataStructure, "uint32"); });
  scope.execute([&] { TestSplitByType<bool>(dataStructure, "bool"); });
  scope.execute([&] { TestSplitByType<int8>(dataStructure, "int8"); });
  scope.execute([&] { TestSplitByType<uint8>(dataStructure, "uint8", {1, 3}); });
  scope.execute([&] { TestSplitByType<int16>(dataStructure, "int16"); });
  scope.execute([&] { TestSplitByType<uint16>(dataStructure, "uint16"); });
  scope.execute([&] { TestSplitByType<int32>(dataStructure, "int32"); });
  scope.execute([&] { TestSplitByType<int64>(dataStructure, "int64"); });
  scope.execute([&] { TestSplitByType<uint64>(dataStructure, "uint64"); });
  scope.execute([&] { TestSplitByType<float>(dataStructure, "float"); });
  scope.execute([&] { TestSplitByType<double>(dataStructure, "double"); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SplitDataArrayByComponentFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SplitDataArrayByComponentFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SplitDataArrayByComponentFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SplitDataArrayByComponentFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<SplitDataArrayByComponentFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<DataPath>(SplitDataArrayByComponentFilter::k_MultiCompArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(SplitDataArrayByComponentFilter::k_Postfix_Key) == "TestName");
    }
  }
}
