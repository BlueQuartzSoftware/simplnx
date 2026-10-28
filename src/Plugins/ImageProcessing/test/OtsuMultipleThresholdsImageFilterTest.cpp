#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/OtsuMultipleThresholdsImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/OtsuImageEngine.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Builds an ImageGeom "Image Geometry" with cell AttributeMatrix "CellData" and a float32 cell
 *        array "Input" holding a deterministic MULTIMODAL distribution: the voxels are split into four
 *        equal blocks whose intensities cluster around 10, 70, 140 and 210 (with a small in-block spread),
 *        so multi-threshold Otsu results are meaningful. The value is a pure function of the flat index,
 *        so two DataStructures built with the same dimension hold byte-identical input.
 */
DataPath BuildMultimodalImage(DataStructure& ds, usize dim)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const std::array<float32, 4> clusterBase = {10.0f, 70.0f, 140.0f, 210.0f};
  const usize total = ref.getSize();
  const usize quarter = (total >= 4) ? (total / 4) : 1;
  for(usize i = 0; i < total; ++i)
  {
    const usize cluster = std::min<usize>(3, i / quarter);
    ref.setValue(i, clusterBase.at(cluster) + static_cast<float32>(i % 8)); // clusters at ~10-17, 70-77, 140-147, 210-217
  }
  return inputPath;
}

DataPath BuildCompactInt16Image(DataStructure& ds, usize dim)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<int16>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<int16>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < ref.getSize(); ++i)
  {
    const int32 cluster = static_cast<int32>((i / 37) % 4);
    ref.setValue(i, static_cast<int16>(-360 + cluster * 240 + static_cast<int32>(i % 23)));
  }
  return inputPath;
}

void RunCompactInt16Otsu(DataStructure& ds, const DataPath& inputPath)
{
  OtsuMultipleThresholdsImageFilter filter;
  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{3}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{7}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(true));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: nonfinite policy is explicit and defaults to Error", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter][OtsuSafeRed]")
{
  UnitTest::LoadPlugins();
  const OtsuMultipleThresholdsImageFilter filter;
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "nonfinite_input_handling_index";

  // Inspect the public schema before lookup. Do not enter the baseline histogram with NaN or infinity.
  REQUIRE(parameters.contains(policyKey));
  const auto* policyPtr = dynamic_cast<const ChoicesParameter*>(parameters.at(policyKey).get());
  REQUIRE(policyPtr != nullptr);
  REQUIRE(policyPtr->humanName() == "Nonfinite Input Handling");
  REQUIRE(policyPtr->choices() == ChoicesParameter::Choices{"Error", "Warn and Continue"});
  REQUIRE(policyPtr->defaultIndex() == 0);
  const Arguments defaultArguments = filter.getDefaultArguments();
  REQUIRE(defaultArguments.value<ChoicesParameter::ValueType>(policyKey) == 0);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: explicit mask selection is optional and linked", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter][OtsuSafeRed]")
{
  UnitTest::LoadPlugins();
  const OtsuMultipleThresholdsImageFilter filter;
  const Parameters parameters = filter.parameters();
  const std::string useMaskKey = "use_mask_array";
  const std::string maskPathKey = "mask_array_path";

  REQUIRE(parameters.contains(useMaskKey));
  REQUIRE(parameters.contains(maskPathKey));
  const auto* useMaskPtr = dynamic_cast<const BoolParameter*>(parameters.at(useMaskKey).get());
  const auto* maskPtr = dynamic_cast<const ArraySelectionParameter*>(parameters.at(maskPathKey).get());
  REQUIRE(useMaskPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  REQUIRE(useMaskPtr->humanName() == "Use Mask Array");
  REQUIRE_FALSE(std::any_cast<bool>(useMaskPtr->defaultValue()));
  REQUIRE(maskPtr->allowedTypes() == ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8});
  REQUIRE(maskPtr->requiredComponentShapes() == ArraySelectionParameter::AllowedComponentShapes{{1}});
  REQUIRE(maskPtr->allowsInMemory());
  REQUIRE(maskPtr->allowsOutOfCore());

  REQUIRE(parameters.containsGroup(useMaskKey));
  const std::vector<std::string> linkedKeys = parameters.getKeysInGroup(useMaskKey);
  REQUIRE(std::find(linkedKeys.begin(), linkedKeys.end(), maskPathKey) != linkedKeys.end());
  // A Masked Label control may also belong to this group. Its presence must not break this test.
  Arguments arguments = filter.getDefaultArguments();
  REQUIRE_FALSE(arguments.value<bool>(useMaskKey));
  REQUIRE_FALSE(parameters.isParameterActive(maskPathKey, arguments));
  arguments.insertOrAssign(useMaskKey, std::make_any<bool>(true));
  REQUIRE(parameters.isParameterActive(maskPathKey, arguments));
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: masked label has a separate uint8 control", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter][OtsuSafeRed]")
{
  UnitTest::LoadPlugins();
  const OtsuMultipleThresholdsImageFilter filter;
  const Parameters parameters = filter.parameters();
  const std::string maskedLabelKey = "masked_label";

  REQUIRE(parameters.contains(maskedLabelKey));
  const auto* maskedLabelPtr = dynamic_cast<const UInt8Parameter*>(parameters.at(maskedLabelKey).get());
  REQUIRE(maskedLabelPtr != nullptr);
  REQUIRE(maskedLabelPtr->humanName() == "Masked Label");
  REQUIRE(std::any_cast<uint8>(maskedLabelPtr->defaultValue()) == uint8{255});
  const Arguments defaultArguments = filter.getDefaultArguments();
  REQUIRE(defaultArguments.value<uint8>(maskedLabelKey) == uint8{255});
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: preflight checks the complete configured uint8 class interval",
          "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter][OtsuSafeRed]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = BuildMultimodalImage(dataStructure, 2);
  const OtsuMultipleThresholdsImageFilter filter;
  Arguments arguments = filter.getDefaultArguments();
  arguments.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  arguments.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  arguments.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  arguments.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(256));

  uint8 labelOffset = 0;
  uint8 thresholdCount = 1;
  bool expectValid = false;
  SECTION("offset 254 plus two thresholds exceeds 255")
  {
    labelOffset = 254;
    thresholdCount = 2;
  }
  SECTION("offset 255 plus one threshold exceeds 255")
  {
    labelOffset = 255;
    thresholdCount = 1;
  }
  SECTION("offset 253 plus two thresholds reaches 255")
  {
    labelOffset = 253;
    thresholdCount = 2;
    expectValid = true;
  }
  SECTION("255 thresholds fit when the offset is zero and no mask is used")
  {
    labelOffset = 0;
    thresholdCount = 255;
    expectValid = true;
  }
  arguments.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(labelOffset));
  arguments.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(thresholdCount));

  // This regression is preflight-only. Never execute overflowing labels or the 255-threshold solver case.
  const auto preflightResult = filter.preflight(dataStructure, arguments);
  if(expectValid)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  else
  {
    REQUIRE(preflightResult.outputActions.invalid());
    REQUIRE_FALSE(preflightResult.outputActions.errors().empty());
    REQUIRE(preflightResult.outputActions.errors().front().code == -8312);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
Arguments otsuArguments(const DataPath& inputPath)
{
  Arguments args = OtsuMultipleThresholdsImageFilter().getDefaultArguments();
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(2));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(7));
  return args;
}

template <class MaskT, usize N>
DataPath createOtsuMask(DataStructure& dataStructure, const std::array<MaskT, N>& values, const ShapeType& componentShape = {1})
{
  const DataPath maskPath({"Mask"});
  auto storePtr = DataStoreUtilities::CreateDataStore<MaskT>(dataStructure, maskPath, {N / componentShape.front()}, componentShape);
  const auto writeResult = storePtr->copyFromBuffer(0, nonstd::span<const MaskT>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  auto* arrayPtr = DataArray<MaskT>::Create(dataStructure, "Mask", storePtr);
  REQUIRE(arrayPtr != nullptr);
  return maskPath;
}

void requireOtsuLabels(const DataStructure& dataStructure, const DataPath& inputPath, const std::vector<uint8>& expected)
{
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* arrayPtr = nullptr;
  REQUIRE_NOTHROW(arrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(arrayPtr->getDataType() == DataType::uint8);
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  REQUIRE(arrayPtr->getNumberOfTuples() == expected.size());
  const UInt8Array* labelsPtr = nullptr;
  REQUIRE_NOTHROW(labelsPtr = &dataStructure.getDataRefAs<UInt8Array>(outputPath));
  std::vector<uint8> actual(expected.size());
  const auto readResult = labelsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  REQUIRE(actual == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

template <class T>
class OtsuTransferStore : public DataStore<T>
{
public:
  explicit OtsuTransferStore(usize size)
  : DataStore<T>({size}, {1}, T{})
  {
  }
  ~OtsuTransferStore() noexcept override = default;
  OtsuTransferStore(const OtsuTransferStore&) = delete;
  OtsuTransferStore(OtsuTransferStore&&) = delete;
  OtsuTransferStore& operator=(const OtsuTransferStore&) = delete;
  OtsuTransferStore& operator=(OtsuTransferStore&&) = delete;

  IDataStore::StoreType getStoreType() const override
  {
    // An abstract-store route witness only. This test store has no disk backing.
    return IDataStore::StoreType::Empty;
  }
  Result<> copyIntoBuffer(usize start, nonstd::span<T> buffer) const override
  {
    ++m_Reads;
    m_MaximumRead = std::max(m_MaximumRead, buffer.size());
    if(m_FailRead || (m_FailReadAt != 0 && m_Reads == m_FailReadAt))
    {
      return MakeErrorResult(-28620, "Injected Otsu bulk-read failure");
    }
    auto result = DataStore<T>::copyIntoBuffer(start, buffer);
    if(m_WarnOnFirstRead && m_Reads == 1)
    {
      result.warnings().push_back(Warning{.code = -28622, .message = "Injected Otsu bulk-read warning"});
    }
    if(m_CancelPtr != nullptr)
    {
      m_CancelPtr->store(true);
    }
    return result;
  }
  Result<> copyFromBuffer(usize start, nonstd::span<const T> buffer) override
  {
    ++m_Writes;
    if(m_FailWrite)
    {
      return MakeErrorResult(-28621, "Injected Otsu bulk-write failure");
    }
    return DataStore<T>::copyFromBuffer(start, buffer);
  }
  void failRead() noexcept
  {
    m_FailRead = true;
  }
  void failWrite() noexcept
  {
    m_FailWrite = true;
  }
  void failReadAt(usize read) noexcept
  {
    m_FailReadAt = read;
  }
  void warnOnFirstRead() noexcept
  {
    m_WarnOnFirstRead = true;
  }
  void cancelOnRead(std::atomic_bool& shouldCancel) noexcept
  {
    m_CancelPtr = &shouldCancel;
  }
  usize reads() const noexcept
  {
    return m_Reads;
  }
  usize writes() const noexcept
  {
    return m_Writes;
  }
  usize maximumRead() const noexcept
  {
    return m_MaximumRead;
  }

private:
  bool m_FailRead = false;
  bool m_WarnOnFirstRead = false;
  usize m_FailReadAt = 0;
  bool m_FailWrite = false;
  std::atomic_bool* m_CancelPtr = nullptr;
  mutable usize m_Reads = 0;
  mutable usize m_Writes = 0;
  mutable usize m_MaximumRead = 0;
};
} // namespace

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: Warn retains literal endpoint frequencies and labels", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize rotation = GENERATE(usize{0}, usize{1}, usize{4});
  std::vector<float64> values{0, 0, 4, 4, std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity()};
  std::vector<uint8> expected{7, 7, 8, 8, 7, 7, 8};
  std::rotate(values.begin(), values.begin() + static_cast<std::vector<float64>::difference_type>(rotation), values.end());
  std::rotate(expected.begin(), expected.begin() + static_cast<std::vector<uint8>::difference_type>(rotation), expected.end());
  DataStructure dataStructure;
  const DataPath inputPath = recon_test::BuildImageFromPattern<float64>(dataStructure, values.size(), 1, 1, values);
  const Float64Array* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Float64Array>(inputPath));
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2, .LabelOffset = 7, .WarnOnNonfinite = true};
  const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(inputPtr->getDataStoreRef(), static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
  REQUIRE(histogramResult.value().HistogramData.min == 0);
  REQUIRE(histogramResult.value().HistogramData.max == 4);
  REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{4, 3});
  REQUIRE(histogramResult.value().NaNCount == 1);
  REQUIRE(histogramResult.value().NegativeInfinityCount == 1);
  REQUIRE(histogramResult.value().PositiveInfinityCount == 1);
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NonfiniteInputHandling_Key, std::make_any<ChoicesParameter::ValueType>(1));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -8319);
  REQUIRE(result.result.warnings().front().message.find("1 NaN, 1 negative infinity and 1 positive infinity") != std::string::npos);
  REQUIRE(result.result.warnings().front().message.find("unreliable") != std::string::npos);
  requireOtsuLabels(dataStructure, inputPath, expected);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: Error rejects each selected nonfinite kind", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const float64 value = GENERATE(std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity());
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(dataStructure, 3, 1, 1, {0, value, 4});
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, otsuArguments(inputPath));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == -8324);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: mask precedes Error and accepts flat tuple count across parents",
                   "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]", bool, uint8)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(
      dataStructure, 7, 1, 1, {0, 0, 4, 4, std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity()});
  constexpr TestType selected = static_cast<TestType>(255);
  const auto maskPath = createOtsuMask(dataStructure, std::array<TestType, 7>{selected, selected, selected, selected, false, false, false});
  const uint8 maskedLabel = GENERATE(uint8{0}, uint8{255});
  const DataArray<TestType>* maskPtr = nullptr;
  const Float64Array* inputPtr = nullptr;
  REQUIRE_NOTHROW(maskPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(maskPath));
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Float64Array>(inputPath));
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2};
  const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(inputPtr->getDataStoreRef(), &maskPtr->getDataStoreRef(), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
  REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{2, 2});
  REQUIRE(histogramResult.value().FiniteCount == 4);
  REQUIRE(histogramResult.value().NaNCount == 0);
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_UseMaskArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskedLabel_Key, std::make_any<uint8>(maskedLabel));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  requireOtsuLabels(dataStructure, inputPath, {7, 7, 8, 8, maskedLabel, maskedLabel, maskedLabel});
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: selected finite data is required in both modes", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const uint64 policy = GENERATE(uint64{0}, uint64{1});
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(dataStructure, 2, 1, 1, {0, 4});
  const auto maskPath = createOtsuMask(dataStructure, std::array<uint8, 2>{0, 0});
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NonfiniteInputHandling_Key, std::make_any<uint64>(policy));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_UseMaskArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == -8325);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: configured mask collision and tuple validation", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float32>(dataStructure, 4, 1, 1, {0, 0, 4, 4});
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_UseMaskArray_Key, std::make_any<bool>(true));
  int32 expectedCode = -8313;
  SECTION("unused interior class is still reserved")
  {
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(3));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(2));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskedLabel_Key, std::make_any<uint8>(8));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<uint8, 4>{1, 1, 1, 1})));
  }
  SECTION("interval lower endpoint")
  {
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskedLabel_Key, std::make_any<uint8>(7));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<uint8, 4>{1, 1, 1, 1})));
  }
  SECTION("interval upper endpoint")
  {
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskedLabel_Key, std::make_any<uint8>(8));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<uint8, 4>{1, 1, 1, 1})));
  }
  SECTION("different tuple count")
  {
    expectedCode = -8314;
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<uint8, 3>{1, 1, 1})));
  }
  SECTION("full unmasked interval leaves no masked label")
  {
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(256));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(255));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(0));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<uint8, 4>{1, 1, 1, 1})));
  }
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == expectedCode);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: full native integer spans retain a valid two-bin split", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]",
                   int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool midpoint = GENERATE(false, true);
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<TestType>(dataStructure, 2, 1, 1, {std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::max()});
  const DataArray<TestType>* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(inputPath));
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2};
  const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(inputPtr->getDataStoreRef(), static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
  REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{1, 1});
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(0));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(midpoint));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  requireOtsuLabels(dataStructure, inputPath, {0, 1});
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: true constants and detectable numeric collapse are distinct", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2};
  SECTION("true tiny constant is valid")
  {
    const DataStore<float64> input({2}, {1}, 1e-200);
    DataStore<uint8> output({2}, {1}, uint8{99});
    const auto result = ImageProcessing::ApplyOtsuImage(input, output, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(output.getValue(0) == 0);
    REQUIRE(output.getValue(1) == 0);
  }
  SECTION("rounded-away marginal extension retains usable bins")
  {
    DataStore<float64> input({2}, {1}, 1.0);
    input.setValue(1, 1.0 + (4.0 * std::numeric_limits<float64>::epsilon()));
    DataStore<uint8> output({2}, {1}, uint8{99});
    const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
    REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{1, 1});
    const auto result = ImageProcessing::ApplyOtsuImage(input, output, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(output.getValue(0) == 0);
    REQUIRE(output.getValue(1) == 1);
  }
  SECTION("adjacent uint64 extrema collapse only after conversion")
  {
    DataStore<uint64> input({2}, {1}, std::numeric_limits<uint64>::max());
    input.setValue(0, std::numeric_limits<uint64>::max() - 1);
    const auto result = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8326);
  }
  SECTION("opposite float64 extrema overflow the range")
  {
    DataStore<float64> input({2}, {1}, std::numeric_limits<float64>::max());
    input.setValue(0, -std::numeric_limits<float64>::max());
    const auto result = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8326);
  }
  SECTION("nonzero means square to zero")
  {
    const ImageProcessing::Histogram histogram{.counts = {1, 1, 100}, .min = 0, .max = 3e-200, .binWidth = 1e-200};
    const auto result = ImageProcessing::ComputeCheckedOtsuThresholds(histogram, options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8320);
  }
  SECTION("finite means square to infinity")
  {
    const ImageProcessing::Histogram histogram{.counts = {1, 1, 100}, .min = 1e200, .max = 4e200, .binWidth = 1e200};
    const auto result = ImageProcessing::ComputeCheckedOtsuThresholds(histogram, options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8320);
  }
  SECTION("unique rational optimum stays unchanged at ordinary scale")
  {
    // Doubled bin centers are [1,3,5]. Split 1 scores 2508; split 0 scores 253110/101.
    // The positive exact margin is 198/101, so the unique optimum is after bin 1.
    const ImageProcessing::Histogram histogram{.counts = {1, 1, 100}, .min = 0, .max = 3, .binWidth = 1};
    const auto result = ImageProcessing::ComputeCheckedOtsuThresholds(histogram, options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value() == std::vector<float64>{2});
  }
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: bounded transfers preserve failures and cancellation", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  std::atomic_bool shouldCancel{false};
  OtsuTransferStore<int32> input(8);
  OtsuTransferStore<bool> mask(8);
  OtsuTransferStore<uint8> output(8);
  for(usize i = 0; i < 8; ++i)
  {
    input.setValue(i, i < 4 ? 0 : 4);
    mask.setValue(i, true);
  }
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2, .TargetBytes = 8};
  int32 expectedCode = 0;
  bool expectWarning = false;
  bool wideRange = false;
  SECTION("bounded compact route and mask output")
  {
  }
  SECTION("input read failure")
  {
    input.failRead();
    expectedCode = -28620;
  }
  SECTION("mask read failure")
  {
    mask.failRead();
    expectedCode = -28620;
  }
  SECTION("classification read failure")
  {
    input.failReadAt(9);
    expectedCode = -28620;
  }
  SECTION("wide native range uses a second bounded histogram scan")
  {
    wideRange = true;
    for(usize i = 4; i < 8; ++i)
    {
      input.setValue(i, 70000);
    }
  }
  SECTION("read warning survives a later output failure")
  {
    input.warnOnFirstRead();
    output.failWrite();
    expectedCode = -28621;
    expectWarning = true;
  }
  SECTION("output write failure")
  {
    output.failWrite();
    expectedCode = -28621;
  }
  SECTION("cancellation during selected scan does not classify partial histogram")
  {
    input.cancelOnRead(shouldCancel);
  }
  const auto result = ImageProcessing::ApplyOtsuImage(input, output, static_cast<const AbstractDataStore<bool>*>(&mask), options, shouldCancel, {});
  if(expectedCode != 0)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == expectedCode);
    if(expectWarning)
    {
      REQUIRE(result.warnings().size() == 1);
      REQUIRE(result.warnings().front().code == -28622);
    }
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    if(shouldCancel)
    {
      REQUIRE(output.writes() == 0);
    }
    else
    {
      REQUIRE(input.reads() > 1);
      REQUIRE(mask.reads() > 1);
      REQUIRE(input.maximumRead() <= 2);
      REQUIRE(input.reads() == (wideRange ? 24 : 16));
      REQUIRE(output.getValue(0) == 0);
      REQUIRE(output.getValue(7) == 1);
    }
  }
}

// -----------------------------------------------------------------------------
// (3) Validation: a data-independent guard must reject a threshold count that is 0 or >= the bin count
//     (which would otherwise make the backend return an empty threshold set and silently collapse the
//     image into a single class). preflight() must return an INVALID result with error code -8311.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: Invalid threshold count (-8311)", "[ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = BuildMultimodalImage(ds, 8);

  uint8 numThresholds = 1;
  uint32 numBins = 128;
  SECTION("thresholds >= bins")
  {
    numThresholds = 200;
    numBins = 128;
  }
  SECTION("thresholds == 0")
  {
    numThresholds = 0;
    numBins = 128;
  }

  OtsuMultipleThresholdsImageFilter filter;
  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(numThresholds));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{0}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(numBins));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors()[0].code == -8311);
}

// -----------------------------------------------------------------------------
// (4) Classify tie rule (regression for the upper_bound -> lower_bound fix). On a CONSTANT image the single
//     Otsu threshold equals the constant value, so every voxel's value is exactly equal to the threshold. ITK's
//     ThresholdLabeler puts a value == threshold in the LOWER class, so every voxel must be labeled labelOffset
//     (0). The pre-fix std::upper_bound put them one class too high (all labeled 1).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: constant image -> lower class at the tie", "[ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({4, 4, 4});
  const ShapeType cellShape = {4, 4, 4};
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < ref.getSize(); ++i)
  {
    ref.setValue(i, 42.0f); // constant
  }

  OtsuMultipleThresholdsImageFilter filter;
  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{1}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{0}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(outStore.getValue(i) == uint8{0}); // value == threshold -> lower class (NOT 1)
  }
}

// -----------------------------------------------------------------------------
// (5) Label offset: the classify op is `offset + idx` (OtsuMultipleThresholdsImageFilter.cpp classify lambda,
//     `return static_cast<U>(offset + idx);`). Every other test pins LabelOffset == 0; this asserts a non-zero
//     LabelOffset shifts EVERY output label by exactly that amount. Running the SAME multimodal image with
//     LabelOffset 0 and LabelOffset 10 must yield outputs that differ by a constant 10 at every voxel (labels
//     0,1,2,3 -> 10,11,12,13). A filter that IGNORED LabelOffset would produce identical outputs and fail here.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: LabelOffset shifts labels", "[ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr uint8 k_Offset = 10;
  const auto runWithOffset = [](DataStructure& ds, const DataPath& inputPath, uint8 labelOffset) {
    OtsuMultipleThresholdsImageFilter filter;
    Arguments args;
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{3})); // 4 classes -> labels 0..3
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(labelOffset));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  DataStructure baseDs;
  const DataPath baseInput = BuildMultimodalImage(baseDs, 8);
  runWithOffset(baseDs, baseInput, uint8{0});

  DataStructure offsetDs;
  const DataPath offsetInput = BuildMultimodalImage(offsetDs, 8);
  runWithOffset(offsetDs, offsetInput, k_Offset);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& baseStore = baseDs.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  const auto& offsetStore = offsetDs.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  REQUIRE(baseStore.getSize() == offsetStore.getSize());
  bool allShifted = true;
  bool anyNonZeroBase = false; // guard against a degenerate all-zero base making the shift trivially satisfied
  usize badIndex = 0;
  for(usize i = 0; i < baseStore.getSize(); ++i)
  {
    if(baseStore.getValue(i) != 0)
    {
      anyNonZeroBase = true;
    }
    if(static_cast<uint8>(baseStore.getValue(i) + k_Offset) != offsetStore.getValue(i))
    {
      allShifted = false;
      badIndex = i;
      break;
    }
  }
  if(!allShifted)
  {
    UNSCOPED_INFO(fmt::format("index={} base={} offset={} expected={}", badIndex, static_cast<uint32>(baseStore.getValue(badIndex)), static_cast<uint32>(offsetStore.getValue(badIndex)),
                              static_cast<uint32>(static_cast<uint8>(baseStore.getValue(badIndex) + k_Offset))));
  }
  REQUIRE(allShifted);
  REQUIRE(anyNonZeroBase);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: real OOC int16 compact-range parity", "[.OOC][ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_Dim = 19;
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  DataStructure inCoreDs;
  {
    const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
    const DataPath inputPath = BuildCompactInt16Image(inCoreDs, k_Dim);
    RunCompactInt16Otsu(inCoreDs, inputPath);
  }

  DataStructure oocDs;
  {
    const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceOutOfCore, 0);
    const DataPath inputPath = BuildCompactInt16Image(oocDs, k_Dim);
    REQUIRE(oocDs.getDataRefAs<IDataArray>(inputPath).getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
    RunCompactInt16Otsu(oocDs, inputPath);
    REQUIRE(oocDs.getDataRefAs<IDataArray>(outputPath).getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  }

  const auto& inCoreOutput = inCoreDs.getDataRefAs<IDataArray>(outputPath);
  const auto& oocOutput = oocDs.getDataRefAs<IDataArray>(outputPath);
  UnitTest::CompareDataArrays<uint8>(inCoreOutput, oocOutput);
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the four ITK-sourced real-image md5 golden cases below. Mirrors ITKOtsuMultipleThresholdsImageTest.cpp:
// read the ITK case's input through OUR ITK-free reader, run OUR OtsuMultipleThresholdsImageFilter (AlwaysUInt8 label
// output) with the ITK case's params, then apply the (A) md5-validity-first golden (plan Sec.4). Pinned ForceInCore.
// -----------------------------------------------------------------------------
void RunOtsuItkGolden(const std::string& inputFile, const std::function<void(Arguments&)>& setParams, const std::string& committedMd5)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args);
  OtsuMultipleThresholdsImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("OtsuMultipleThresholdsImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden("RA-Short.nrrd", [](Arguments&) {}, "a9c3b0c0971c5cbda12b29db916451c6");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (two_on_float)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden(
      "Ramp-Zero-One-Float.nrrd",
      [](Arguments& args) {
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(2));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
      },
      "1ab20d3cd9a354b45ac07ec59c0413b3");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (three_on)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden(
      "cthead1.png",
      [](Arguments& args) {
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(3));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(256));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
      },
      "b61c3f4e063fcdd24dba76227129ae34");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (valley_emphasis)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden(
      "cthead1.png",
      [](Arguments& args) {
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(3));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(true));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
      },
      "fb65e730472c8001185f355fb626ca3f");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: compatible pipeline imports receive intentional defaults", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const OtsuMultipleThresholdsImageFilter filter;
  auto json = filter.toJson(filter.getDefaultArguments());
  json.erase("nonfinite_input_handling_index");
  json.erase("use_mask_array");
  json.erase("mask_array_path");
  json.erase("masked_label");
  const auto result = filter.fromJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.warnings().size() == 4);
  REQUIRE(result.value().value<ChoicesParameter::ValueType>("nonfinite_input_handling_index") == 0);
  REQUIRE_FALSE(result.value().value<bool>("use_mask_array"));
  REQUIRE(result.value().value<uint8>("masked_label") == 255);
  const nlohmann::json legacy{{"NumberOfThresholds", 1},
                              {"LabelOffset", 7},
                              {"NumberOfHistogramBins", 128},
                              {"ValleyEmphasis", false},
                              {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                              {"NewCellArrayName", "Output"}};
  const auto legacyResult = OtsuMultipleThresholdsImageFilter::FromSIMPLJson(legacy);
  SIMPLNX_RESULT_REQUIRE_VALID(legacyResult);
  REQUIRE(legacyResult.value().value<ChoicesParameter::ValueType>("nonfinite_input_handling_index") == 0);
  REQUIRE_FALSE(legacyResult.value().value<bool>("use_mask_array"));
  REQUIRE(legacyResult.value().value<uint8>("masked_label") == 255);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: mask selection validates scalar bool or uint8 and ignores disabled paths",
          "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(dataStructure, 2, 1, 1, {0, 4});
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_UseMaskArray_Key, std::make_any<bool>(true));
  int32 expectedCode = 0;
  SECTION("wrong mask type")
  {
    expectedCode = FilterParameter::Constants::k_Validate_AllowedType_Error;
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<float32, 2>{1, 1})));
  }
  SECTION("multiple mask components")
  {
    expectedCode = FilterParameter::Constants::k_Validate_TupleShapeValue;
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(createOtsuMask(dataStructure, std::array<uint8, 4>{1, 1, 1, 1}, {2})));
  }
  SECTION("disabled stale mask path")
  {
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_UseMaskArray_Key, std::make_any<bool>(false));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Does Not Exist"})));
  }
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.preflight(dataStructure, args);
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == expectedCode);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: constants keep explicit Warn endpoints and empty selected data errors",
          "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2, .LabelOffset = 7, .WarnOnNonfinite = true};
  DataStore<float64> input({4}, {1}, 42.0);
  input.setValue(1, std::numeric_limits<float64>::quiet_NaN());
  input.setValue(2, -std::numeric_limits<float64>::infinity());
  input.setValue(3, std::numeric_limits<float64>::infinity());
  const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
  REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{3, 1});
  DataStore<uint8> output({4}, {1}, uint8{99});
  const auto result = ImageProcessing::ApplyOtsuImage(input, output, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(output.getValue(0) == 7);
  REQUIRE(output.getValue(1) == 7);
  REQUIRE(output.getValue(2) == 7);
  REQUIRE(output.getValue(3) == 8);
  input.setValue(0, std::numeric_limits<float64>::quiet_NaN());
  const auto noFiniteResult = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(noFiniteResult);
  REQUIRE(noFiniteResult.errors().front().code == -8325);
  const DataStore<float64> empty({0}, {1}, 0.0);
  const auto emptyResult = ImageProcessing::ComputeOtsuHistogram(empty, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(emptyResult);
  REQUIRE(emptyResult.errors().front().code == -8325);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: checked helper configuration errors are actionable", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const std::atomic_bool shouldCancel{false};
  ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2};
  DataStore<float64> input({2}, {1}, 0.0);
  input.setValue(1, 4.0);
  SECTION("bin count")
  {
    options.NumberOfBins = 1;
    const auto result = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28600);
  }
  SECTION("mask size")
  {
    DataStore<uint8> mask({1}, {1}, uint8{1});
    const auto result = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(&mask), options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8329);
  }
  SECTION("output size")
  {
    DataStore<uint8> output({1}, {1}, uint8{0});
    const auto result = ImageProcessing::ApplyOtsuImage(input, output, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28602);
  }
  SECTION("label interval")
  {
    options.LabelOffset = 255;
    DataStore<uint8> output({2}, {1}, uint8{0});
    const auto result = ImageProcessing::ApplyOtsuImage(input, output, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28603);
  }
  SECTION("threshold count")
  {
    options.NumberOfThresholds = 0;
    const ImageProcessing::Histogram histogram{.counts = {1, 1}, .min = 0, .max = 4, .binWidth = 2};
    const auto result = ImageProcessing::ComputeCheckedOtsuThresholds(histogram, options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28601);
  }
  SECTION("empty histogram")
  {
    const ImageProcessing::Histogram histogram{.counts = {0, 0}, .min = 0, .max = 4, .binWidth = 2};
    const auto result = ImageProcessing::ComputeCheckedOtsuThresholds(histogram, options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28605);
  }
  SECTION("nonfinite histogram metadata")
  {
    const ImageProcessing::Histogram histogram{.counts = {1, 1}, .min = std::numeric_limits<float64>::infinity(), .max = std::numeric_limits<float64>::infinity(), .binWidth = 0};
    const auto result = ImageProcessing::ComputeCheckedOtsuThresholds(histogram, options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28606);
  }
  SECTION("collapsed required bin boundaries")
  {
    options.NumberOfBins = 128;
    input.setValue(0, 1e20);
    input.setValue(1, 1e20 + 32768.0);
    const auto result = ImageProcessing::ComputeOtsuHistogram(input, static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8327);
  }
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: highest legal class labels coexist with masked label zero", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(dataStructure, 3, 1, 1, {0, 4, std::numeric_limits<float64>::quiet_NaN()});
  const auto maskPath = createOtsuMask(dataStructure, std::array<uint8, 3>{1, 255, 0});
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(254));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_UseMaskArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_MaskedLabel_Key, std::make_any<uint8>(0));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  requireOtsuLabels(dataStructure, inputPath, {254, 255, 0});
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: two thresholds keep ordinary Warn endpoint classes", "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(
      dataStructure, 6, 1, 1, {0, 2, 4, std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity()});
  const Float64Array* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Float64Array>(inputPath));
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 3, .NumberOfThresholds = 2, .LabelOffset = 7, .WarnOnNonfinite = true};
  const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(inputPtr->getDataStoreRef(), static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
  REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{3, 1, 2});
  // Three bins and two thresholds have exactly one possible split: each finite intensity gets its own class.
  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(3));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(2));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NonfiniteInputHandling_Key, std::make_any<ChoicesParameter::ValueType>(1));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  requireOtsuLabels(dataStructure, inputPath, {7, 8, 9, 7, 7, 9});
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: first centroid may equal minimum with usable two-bin separation",
          "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter][OtsuCentroidSafeRed]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool midpoint = GENERATE(false, true);
  REQUIRE(std::numeric_limits<float64>::is_iec559);
  REQUIRE(std::numeric_limits<float64>::radix == 2);
  REQUIRE(std::numeric_limits<float64>::digits == 53);
  constexpr float64 epsilon = std::numeric_limits<float64>::epsilon();
  constexpr float64 minimum = 1.0;
  constexpr float64 maximum = 1.0 + (2.0 * epsilon);
  REQUIRE(maximum > minimum);
  REQUIRE(maximum - minimum == 2.0 * epsilon);
  REQUIRE(minimum + epsilon > minimum);
  REQUIRE(minimum + epsilon < maximum);
  REQUIRE(minimum + (0.5 * epsilon) == minimum);
  REQUIRE(maximum + ((maximum - minimum) / 200.0) == maximum);

  DataStructure dataStructure;
  const auto inputPath = recon_test::BuildImageFromPattern<float64>(dataStructure, 2, 1, 1, {minimum, maximum});
  const Float64Array* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Float64Array>(inputPath));
  REQUIRE(inputPtr->getTupleShape() == ShapeType{1, 1, 2});
  REQUIRE(inputPtr->getComponentShape() == ShapeType{1});
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2, .ReturnBinMidpoint = midpoint};
  const auto histogramResult = ImageProcessing::ComputeOtsuHistogram(inputPtr->getDataStoreRef(), static_cast<const AbstractDataStore<uint8>*>(nullptr), options, shouldCancel);
  // Binary64 centroids are [minimum, maximum], with one finite, distinct split. Rejecting the first centroid is incorrect.
  SIMPLNX_RESULT_REQUIRE_VALID(histogramResult);
  REQUIRE(histogramResult.value().HistogramData.counts == std::vector<usize>{1, 1});

  auto args = otsuArguments(inputPath);
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(0));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(midpoint));
  const OtsuMultipleThresholdsImageFilter filter;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const IDataArray* outputPtr = nullptr;
  REQUIRE_NOTHROW(outputPtr = &dataStructure.getDataRefAs<IDataArray>(inputPath.replaceName("Output")));
  REQUIRE(outputPtr->getDataType() == DataType::uint8);
  REQUIRE(outputPtr->getTupleShape() == ShapeType{1, 1, 2});
  REQUIRE(outputPtr->getComponentShape() == ShapeType{1});
  requireOtsuLabels(dataStructure, inputPath, {0, 1});
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: floating summaries preserve literal counts across resident and buffered batches",
          "[ImageProcessing][Issue1874][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const bool buffered = GENERATE(false, true);
  const bool excludeNonfinite = GENERATE(false, true);
  const usize targetBytes = GENERATE(usize{4096}, usize{64 * 1024 * 1024});
  constexpr usize valueCount = 524296; // 65,537 periods; spans multiple 262,144-value reduction blocks plus a short tail.
  std::unique_ptr<AbstractDataStore<float64>> inputPtr;
  if(buffered)
  {
    inputPtr = std::make_unique<OtsuTransferStore<float64>>(valueCount);
  }
  else
  {
    inputPtr = std::make_unique<DataStore<float64>>(ShapeType{valueCount}, ShapeType{1}, 0.0);
  }
  DataStore<uint8> mask({valueCount}, {1}, uint8{0});
  const std::array<float64, 8> values{0,
                                      4,
                                      std::numeric_limits<float64>::quiet_NaN(),
                                      -std::numeric_limits<float64>::infinity(),
                                      std::numeric_limits<float64>::infinity(),
                                      99,
                                      std::numeric_limits<float64>::quiet_NaN(),
                                      -std::numeric_limits<float64>::infinity()};
  for(usize index = 0; index < valueCount; ++index)
  {
    const usize periodIndex = index % values.size();
    inputPtr->setValue(index, values.at(periodIndex));
    mask.setValue(index, periodIndex < (excludeNonfinite ? 2 : 5) ? uint8{255} : uint8{0});
  }
  const ImageProcessing::OtsuImageOptions options{.NumberOfBins = 2, .WarnOnNonfinite = !excludeNonfinite, .TargetBytes = targetBytes};
  const std::atomic_bool shouldCancel{false};
  const auto result = ImageProcessing::ComputeOtsuHistogram(*inputPtr, static_cast<const AbstractDataStore<uint8>*>(&mask), options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value().HistogramData.min == 0);
  REQUIRE(result.value().HistogramData.max == 4);
  REQUIRE(result.value().FiniteCount == 131074);
  REQUIRE(result.value().NaNCount == (excludeNonfinite ? 0 : 65537));
  REQUIRE(result.value().NegativeInfinityCount == (excludeNonfinite ? 0 : 65537));
  REQUIRE(result.value().PositiveInfinityCount == (excludeNonfinite ? 0 : 65537));
  // Per period, Warn contributes [0,NaN,-inf] to bin 0 and [4,+inf] to bin 1. Excluded values, including 99, contribute nothing.
  const std::vector<usize> expected = excludeNonfinite ? std::vector<usize>{65537, 65537} : std::vector<usize>{196611, 131074};
  REQUIRE(result.value().HistogramData.counts == expected);
  if(buffered)
  {
    const auto* transferPtr = dynamic_cast<const OtsuTransferStore<float64>*>(inputPtr.get());
    REQUIRE(transferPtr != nullptr);
    REQUIRE(transferPtr->reads() > 1);
    REQUIRE(transferPtr->maximumRead() <= (targetBytes / (sizeof(float64) + sizeof(uint8))));
  }
  // These are output/transfer-bound assertions. No thread-count or throughput claim follows from them.
}
