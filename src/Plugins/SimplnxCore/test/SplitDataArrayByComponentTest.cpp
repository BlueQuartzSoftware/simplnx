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

void fillDataArray(BoolArray* inputArray)
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
void fillDataArray(DataArray<T>* inputArray)
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

DataStructure createDataStructure()
{
  DataStructure dataStructure;
  AttributeMatrix* am1 = AttributeMatrix::Create(dataStructure, "AttributeMatrix", {10});

  UInt32Array* mcArray1 = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, "MultiComponent Array uint32", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<uint32>(mcArray1);

  BoolArray* mcArray2 = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "MultiComponent Array bool", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray(mcArray2);

  Int8Array* mcArray4 = Int8Array::CreateWithStore<DataStore<int8>>(dataStructure, "MultiComponent Array int8", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<int8>(mcArray4);

  UInt8Array* mcArray5 = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "MultiComponent Array uint8", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<uint8>(mcArray5);

  Int16Array* mcArray6 = Int16Array::CreateWithStore<DataStore<int16>>(dataStructure, "MultiComponent Array int16", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<int16>(mcArray6);

  UInt16Array* mcArray7 = UInt16Array::CreateWithStore<DataStore<uint16>>(dataStructure, "MultiComponent Array uint16", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<uint16>(mcArray7);

  Int32Array* mcArray8 = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "MultiComponent Array int32", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<int32>(mcArray8);

  Int64Array* mcArray9 = Int64Array::CreateWithStore<DataStore<int64>>(dataStructure, "MultiComponent Array int64", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<int64>(mcArray9);

  UInt64Array* mcArray10 = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "MultiComponent Array uint64", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<uint64>(mcArray10);

  Float32Array* mcArray11 = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "MultiComponent Array float", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<float32>(mcArray11);

  Float64Array* mcArray12 = Float64Array::CreateWithStore<DataStore<float64>>(dataStructure, "MultiComponent Array double", std::vector<size_t>(1, 10), std::vector<size_t>(1, 5), am1->getId());
  fillDataArray<float64>(mcArray12);

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

  DataArray<T>& mcArray_original = dataStructure.getDataRefAs<DataArray<T>>(DataPath({"AttributeMatrix", "MultiComponent Array " + dataType}));
  std::map<usize, DataArrayPtrType> mcArraysAfterSplit;
  for(const auto& comp : compsToCheck)
  {
    DataArrayPtrType mcArray = dataStructure.getDataAs<DataArray<T>>(DataPath({"AttributeMatrix", "MultiComponent Array " + dataType + "Component" + StringUtilities::number(comp)}));
    mcArraysAfterSplit[comp] = mcArray;
  }

  usize numTuples = mcArray_original.getNumberOfTuples();
  usize numComps = mcArray_original.getNumberOfComponents();
  for(int i = 0; i < numTuples; i++)
  {
    for(const auto& j : compsToCheck)
    {
      T originalValue = mcArray_original[i * numComps + j];
      T afterSplitValue = (*mcArraysAfterSplit[j])[i];
      REQUIRE(originalValue == afterSplitValue);
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::SplitDataArrayByComponent", "[SimplnxCore][SplitDataArrayByComponentFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = createDataStructure();

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SplitDataArrayByComponentFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SplitDataArrayByComponentFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SplitDataArrayByComponentFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SplitDataArrayByComponentFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<SplitDataArrayByComponentFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(SplitDataArrayByComponentFilter::k_MultiCompArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(SplitDataArrayByComponentFilter::k_Postfix_Key) == "TestName");
    }
  }
}
