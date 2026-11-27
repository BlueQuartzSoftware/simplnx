#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <utility>

using namespace nx::core;

IFilter::PreflightResult nx::core::PreflightColorToGrayScale(const DataStructure& dataStructure, const ConvertColorToGrayScaleInputValues& inputValues)
{
  Result<OutputActions> resultOutputActions;
  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  if(inputValues.conversion_algorithm == to_underlying(ConvertColorToGrayScale::ConversionType::Luminosity))
  {
    if(inputValues.color_weights[0] < 0.0F || inputValues.color_weights[1] < 0.0F || inputValues.color_weights[2] < 0.0F)
    {
      return {.output_actions = MakeErrorResult<OutputActions>(-10704, "One or more of the Color Weight values is negative. All weights must be zero or positive.")};
    }

    const float colorWeightSum = inputValues.color_weights[0] + inputValues.color_weights[1] + inputValues.color_weights[2];
    if(colorWeightSum < .9800 || colorWeightSum > 1.02)
    {
      return {.output_actions = MakeErrorResult<OutputActions>(-10704, fmt::format("Color Weight values should sum up to 1.0. Current sum is {}", colorWeightSum))};
    }
  }

  if(inputValues.input_data_array_paths.empty())
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-10705, fmt::format("No input arrays selected for conversion."))};
  }

  for(const auto& inputDataArrayPath : inputValues.input_data_array_paths)
  {
    const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);
    const usize numComponents = inputArray.getNumberOfComponents();
    if(inputValues.conversion_algorithm == to_underlying(ConvertColorToGrayScale::ConversionType::SingleChannel))
    {
      if(numComponents == 0)
      {
        return {.output_actions =
                    MakeErrorResult<OutputActions>(-10711, fmt::format("Color channel {} is invalid for array '{}', which has no components. Select an array with at least one component.",
                                                                       inputValues.color_channel, inputDataArrayPath.toString()))};
      }
      if(inputValues.color_channel < 0 || std::cmp_greater_equal(inputValues.color_channel, numComponents))
      {
        return {.output_actions = MakeErrorResult<OutputActions>(-10711, fmt::format("Color channel {} is invalid for array '{}' with {} components. Select a channel from 0 to {}.",
                                                                                     inputValues.color_channel, inputDataArrayPath.toString(), numComponents, numComponents - 1))};
      }
    }
    else if(inputValues.conversion_algorithm <= to_underlying(ConvertColorToGrayScale::ConversionType::Lightness) && numComponents < 3)
    {
      return {.output_actions = MakeErrorResult<OutputActions>(-10710, fmt::format("Grayscale conversion requires at least 3 components. Array '{}' has {} components. Select an RGB or RGBA array.",
                                                                                   inputDataArrayPath.toString(), numComponents))};
    }
    std::vector<std::string> inputPathVector = inputDataArrayPath.getPathVector();
    const std::string inputArrayName = inputDataArrayPath.getTargetName();
    const std::string outputArrayName = fmt::format("{}{}", inputValues.output_array_prefix, inputArrayName);
    inputPathVector.back() = outputArrayName;
    const DataPath outputDataArrayPath(inputPathVector);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, inputArray.getIDataStoreRef().getTupleShape(), std::vector<usize>(1, 1), outputDataArrayPath));
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

namespace
{
// RGB input and grayscale output buffers stay fixed at 65,536 tuples.
constexpr usize k_ChunkTuples = 65536;

/**
 * @class LuminosityImpl
 * @brief Converts RGB tuples with selected color weights.
 * @tparam BoundsCheckV Selects checked component access.
 *
 * This abstract-store worker requires in-memory stores. Its parallel access has
 * no general DataStore thread-safety guarantee.
 */
template <bool BoundsCheckV>
class LuminosityImpl
{
public:
  LuminosityImpl(const UInt8AbstractDataStoreType& data, UInt8AbstractDataStoreType& outputData, const FloatVec3Type& colorWeights, size_t numComp)
  : m_ImageData(data)
  , m_FlatImageData(outputData)
  , m_ColorWeights(colorWeights)
  , m_NumComp(numComp)
  {
  }
  LuminosityImpl(const LuminosityImpl&) = default;
  LuminosityImpl(LuminosityImpl&&) = default;
  LuminosityImpl& operator=(const LuminosityImpl&) = delete;
  LuminosityImpl& operator=(LuminosityImpl&&) = delete;
  ~LuminosityImpl() = default;

  // Convert through int32 before uint8. C 6.3.1.4 defines the floating-point
  // conversion step.
  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if constexpr(BoundsCheckV)
      {
        auto temp = static_cast<int32>(roundf((m_ImageData.at(m_NumComp * i) * m_ColorWeights.getX()) + (m_ImageData.at(m_NumComp * i + 1) * m_ColorWeights.getY()) +
                                              (m_ImageData.at(m_NumComp * i + 2) * m_ColorWeights.getZ())));
        m_FlatImageData.setValue(i, static_cast<uint8>(temp));
      }
      else
      {
        auto temp = static_cast<int32>(
            roundf((m_ImageData[m_NumComp * i] * m_ColorWeights.getX()) + (m_ImageData[m_NumComp * i + 1] * m_ColorWeights.getY()) + (m_ImageData[m_NumComp * i + 2] * m_ColorWeights.getZ())));
        m_FlatImageData.setValue(i, static_cast<uint8>(temp));
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const UInt8AbstractDataStoreType& m_ImageData;
  UInt8AbstractDataStoreType& m_FlatImageData;
  const FloatVec3Type& m_ColorWeights;
  size_t m_NumComp;
};

/**
 * @class LightnessImpl
 * @brief Converts RGB tuples from their minimum and maximum components.
 *
 * This abstract-store worker requires in-memory stores. Its parallel access has
 * no general DataStore thread-safety guarantee.
 */
class LightnessImpl
{
public:
  LightnessImpl(const UInt8AbstractDataStoreType& data, UInt8AbstractDataStoreType& outputData, size_t numComp)
  : m_ImageData(data)
  , m_FlatImageData(outputData)
  , m_NumComp(numComp)
  {
  }
  LightnessImpl(const LightnessImpl&) = default;
  LightnessImpl(LightnessImpl&&) = default;
  LightnessImpl& operator=(const LightnessImpl&) = delete;
  LightnessImpl& operator=(LightnessImpl&&) = delete;
  ~LightnessImpl() = default;

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      auto minmax = std::minmax_element(m_ImageData.begin() + (i * m_NumComp), m_ImageData.begin() + (i * m_NumComp + 3));
      m_FlatImageData.setValue(i, static_cast<uint8_t>(roundf(static_cast<float>(static_cast<int16_t>(*(minmax.first)) + static_cast<int16_t>(*(minmax.second))) / 2.0f)));
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const UInt8AbstractDataStoreType& m_ImageData;
  UInt8AbstractDataStoreType& m_FlatImageData;
  size_t m_NumComp;
};

/**
 * @class SingleChannelImpl
 * @brief Copies one selected RGB component.
 * @tparam BoundsCheckV Selects checked component access.
 *
 * This abstract-store worker requires in-memory stores. Its parallel access has
 * no general DataStore thread-safety guarantee.
 */
template <bool BoundsCheckV>
class SingleChannelImpl
{
public:
  SingleChannelImpl(const UInt8AbstractDataStoreType& data, UInt8AbstractDataStoreType& outputData, size_t numComp, int32_t channel)
  : m_ImageData(data)
  , m_FlatImageData(outputData)
  , m_NumComp(numComp)
  , m_Channel(channel)
  {
  }
  SingleChannelImpl(const SingleChannelImpl&) = default;
  SingleChannelImpl(SingleChannelImpl&&) = default;
  SingleChannelImpl& operator=(const SingleChannelImpl&) = delete;
  SingleChannelImpl& operator=(SingleChannelImpl&&) = delete;
  ~SingleChannelImpl() = default;

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if constexpr(BoundsCheckV)
      {
        m_FlatImageData.setValue(i, m_ImageData.at(m_NumComp * i + static_cast<size_t>(m_Channel)));
      }
      else
      {
        m_FlatImageData.setValue(i, m_ImageData[m_NumComp * i + static_cast<size_t>(m_Channel)]);
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const UInt8AbstractDataStoreType& m_ImageData;
  UInt8AbstractDataStoreType& m_FlatImageData;
  size_t m_NumComp;
  int32_t m_Channel;
};

/**
 * @class ContiguousConversionImpl
 * @brief Parallel conversion worker for contiguous in-memory stores.
 * @tparam ConversionV Specifies the grayscale conversion mode.
 *
 * Raw pointers avoid DataStore access in parallel. Each range writes disjoint
 * output tuples.
 */
template <ConvertColorToGrayScale::ConversionType ConversionV>
class ContiguousConversionImpl
{
public:
  ContiguousConversionImpl(const uint8* inputData, uint8* outputData, FloatVec3Type colorWeights, usize numComponents, int32 colorChannel)
  : m_InputData(inputData)
  , m_OutputData(outputData)
  , m_ColorWeights(std::move(colorWeights))
  , m_NumComponents(numComponents)
  , m_ColorChannel(colorChannel)
  {
  }

  void operator()(const Range& range) const
  {
    for(usize tupleIndex = range.min(); tupleIndex < range.max(); tupleIndex++)
    {
      const usize componentOffset = tupleIndex * m_NumComponents;
      if constexpr(ConversionV == ConvertColorToGrayScale::ConversionType::Luminosity || ConversionV == ConvertColorToGrayScale::ConversionType::Average)
      {
        const auto value = static_cast<int32>(
            roundf((m_InputData[componentOffset] * m_ColorWeights.getX()) + (m_InputData[componentOffset + 1] * m_ColorWeights.getY()) + (m_InputData[componentOffset + 2] * m_ColorWeights.getZ())));
        m_OutputData[tupleIndex] = static_cast<uint8>(value);
      }
      else if constexpr(ConversionV == ConvertColorToGrayScale::ConversionType::Lightness)
      {
        const auto minMax = std::minmax_element(m_InputData + componentOffset, m_InputData + componentOffset + 3);
        m_OutputData[tupleIndex] = static_cast<uint8>(roundf(static_cast<float>(static_cast<int16>(*minMax.first) + static_cast<int16>(*minMax.second)) / 2.0F));
      }
      else
      {
        m_OutputData[tupleIndex] = m_InputData[componentOffset + static_cast<usize>(m_ColorChannel)];
      }
    }
  }

private:
  const uint8* m_InputData = nullptr;
  uint8* m_OutputData = nullptr;
  FloatVec3Type m_ColorWeights;
  usize m_NumComponents = 0;
  int32 m_ColorChannel = 0;
};

/**
 * @class ParallelWrapper
 * @brief Runs conversion workers over in-memory stores or raw pointers.
 *
 * requireStoresInMemory() excludes disk-backed stores. It does not make generic
 * DataStore parallel access safe.
 */
class ParallelWrapper
{
public:
  ~ParallelWrapper() = default;
  ParallelWrapper(const ParallelWrapper&) = delete;
  ParallelWrapper(ParallelWrapper&&) = delete;
  ParallelWrapper& operator=(const ParallelWrapper&) = delete;
  ParallelWrapper& operator=(ParallelWrapper&&) = delete;

  template <typename T>
  static void Run(T impl, size_t totalPoints, const typename IParallelAlgorithm::AlgorithmStoresType& algStores)
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, totalPoints);
    dataAlg.requireStoresInMemory(algStores);
    dataAlg.execute(impl);
  }

  template <typename T>
  static void RunContiguous(const T& impl, size_t totalPoints)
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, totalPoints);
    dataAlg.execute(impl);
  }

protected:
  ParallelWrapper() = default;
};

/**
 * @class ConvertColorToGrayScaleDirect
 * @brief Converts grayscale values through in-memory stores.
 *
 * Concrete stores use raw-pointer workers. The abstract fallback retains the
 * existing parallel DataStore limitation. This path does not inspect
 * cancellation after dispatch.
 */
class ConvertColorToGrayScaleDirect
{
public:
  ConvertColorToGrayScaleDirect(const UInt8AbstractDataStoreType& inputColorData, UInt8AbstractDataStoreType& outputGrayData, ConvertColorToGrayScale::ConversionType conversionType,
                                FloatVec3Type colorWeights, int32 colorChannel, const std::atomic_bool& /*unused*/)
  : m_InputColorData(inputColorData)
  , m_OutputGrayData(outputGrayData)
  , m_ConversionType(conversionType)
  , m_ColorWeights(std::move(colorWeights))
  , m_ColorChannel(colorChannel)
  {
  }

  /**
   * @brief Converts all input tuples.
   * @return Success after conversion.
   */
  Result<> operator()() const
  {
    const usize numComponents = m_InputColorData.getNumberOfComponents();
    const usize totalPoints = m_InputColorData.getNumberOfTuples();

    const auto* contiguousInputStorePtr = dynamic_cast<const UInt8DataStoreType*>(&m_InputColorData);
    auto* contiguousOutputStorePtr = dynamic_cast<UInt8DataStoreType*>(&m_OutputGrayData);
    if(contiguousInputStorePtr != nullptr && contiguousOutputStorePtr != nullptr)
    {
      const uint8* inputDataPtr = contiguousInputStorePtr->data();
      uint8* outputDataPtr = contiguousOutputStorePtr->data();
      switch(m_ConversionType)
      {
      case ConvertColorToGrayScale::ConversionType::Luminosity:
        if(numComponents >= 3)
        {
          ParallelWrapper::RunContiguous(ContiguousConversionImpl<ConvertColorToGrayScale::ConversionType::Luminosity>(inputDataPtr, outputDataPtr, m_ColorWeights, numComponents, m_ColorChannel),
                                         totalPoints);
          return {};
        }
        break;
      case ConvertColorToGrayScale::ConversionType::Average:
        if(numComponents >= 3)
        {
          ParallelWrapper::RunContiguous(
              ContiguousConversionImpl<ConvertColorToGrayScale::ConversionType::Average>(inputDataPtr, outputDataPtr, {0.3333F, 0.3333F, 0.3333F}, numComponents, m_ColorChannel), totalPoints);
          return {};
        }
        break;
      case ConvertColorToGrayScale::ConversionType::Lightness:
        if(numComponents >= 3)
        {
          ParallelWrapper::RunContiguous(ContiguousConversionImpl<ConvertColorToGrayScale::ConversionType::Lightness>(inputDataPtr, outputDataPtr, m_ColorWeights, numComponents, m_ColorChannel),
                                         totalPoints);
          return {};
        }
        break;
      case ConvertColorToGrayScale::ConversionType::SingleChannel:
        if(totalPoints > 0 && numComponents * (totalPoints - 1) + m_ColorChannel < m_InputColorData.getSize())
        {
          ParallelWrapper::RunContiguous(ContiguousConversionImpl<ConvertColorToGrayScale::ConversionType::SingleChannel>(inputDataPtr, outputDataPtr, m_ColorWeights, numComponents, m_ColorChannel),
                                         totalPoints);
          return {};
        }
        break;
      }
    }

    typename IParallelAlgorithm::AlgorithmStoresType algorithmStores;
    algorithmStores.push_back(&m_InputColorData);
    algorithmStores.push_back(&m_OutputGrayData);

    switch(m_ConversionType)
    {
    case ConvertColorToGrayScale::ConversionType::Luminosity:
      if(numComponents < 3)
      {
        ParallelWrapper::Run<LuminosityImpl<true>>(LuminosityImpl<true>(m_InputColorData, m_OutputGrayData, m_ColorWeights, numComponents), totalPoints, algorithmStores);
      }
      else
      {
        ParallelWrapper::Run<LuminosityImpl<false>>(LuminosityImpl<false>(m_InputColorData, m_OutputGrayData, m_ColorWeights, numComponents), totalPoints, algorithmStores);
      }
      break;
    case ConvertColorToGrayScale::ConversionType::Average:
      if(numComponents < 3)
      {
        ParallelWrapper::Run<LuminosityImpl<true>>(LuminosityImpl<true>(m_InputColorData, m_OutputGrayData, {0.3333F, 0.3333F, 0.3333F}, numComponents), totalPoints, algorithmStores);
      }
      else
      {
        ParallelWrapper::Run<LuminosityImpl<false>>(LuminosityImpl<false>(m_InputColorData, m_OutputGrayData, {0.3333F, 0.3333F, 0.3333F}, numComponents), totalPoints, algorithmStores);
      }
      break;
    case ConvertColorToGrayScale::ConversionType::Lightness:
      ParallelWrapper::Run<LightnessImpl>(LightnessImpl(m_InputColorData, m_OutputGrayData, numComponents), totalPoints, algorithmStores);
      break;
    case ConvertColorToGrayScale::ConversionType::SingleChannel:
      if(numComponents * (totalPoints - 1) + m_ColorChannel < m_InputColorData.getSize())
      {
        ParallelWrapper::Run<SingleChannelImpl<false>>(SingleChannelImpl<false>(m_InputColorData, m_OutputGrayData, numComponents, m_ColorChannel), totalPoints, algorithmStores);
      }
      else
      {
        ParallelWrapper::Run<SingleChannelImpl<true>>(SingleChannelImpl<true>(m_InputColorData, m_OutputGrayData, numComponents, m_ColorChannel), totalPoints, algorithmStores);
      }
      break;
    }

    return {};
  }

private:
  const UInt8AbstractDataStoreType& m_InputColorData;
  UInt8AbstractDataStoreType& m_OutputGrayData;
  ConvertColorToGrayScale::ConversionType m_ConversionType;
  FloatVec3Type m_ColorWeights;
  int32 m_ColorChannel;
};

/**
 * @class ConvertColorToGrayScaleScanline
 * @brief Converts grayscale values through bounded bulk I/O.
 *
 * Each completed chunk remains written after cancellation.
 */
class ConvertColorToGrayScaleScanline
{
public:
  /**
   * @brief Creates a bulk-I/O grayscale algorithm.
   * @param inputColorData Provides RGB values.
   * @param outputGrayData Receives grayscale values.
   * @param conversionType Specifies the conversion mode.
   * @param colorWeights Specifies luminosity weights.
   * @param colorChannel Specifies the selected RGB component.
   * @param shouldCancel Stops before later chunks when true.
   */
  ConvertColorToGrayScaleScanline(const UInt8AbstractDataStoreType& inputColorData, UInt8AbstractDataStoreType& outputGrayData, ConvertColorToGrayScale::ConversionType conversionType,
                                  FloatVec3Type colorWeights, int32 colorChannel, const std::atomic_bool& shouldCancel)
  : m_InputColorData(inputColorData)
  , m_OutputGrayData(outputGrayData)
  , m_ConversionType(conversionType)
  , m_ColorWeights(std::move(colorWeights))
  , m_ColorChannel(colorChannel)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Converts all input tuples through bulk I/O.
   * @return Error from bulk I/O, or success after cancellation.
   */
  Result<> operator()() const
  {
    const usize numComponents = m_InputColorData.getNumberOfComponents();
    const usize totalPoints = m_InputColorData.getNumberOfTuples();
    if(totalPoints == 0)
    {
      return {};
    }

    auto inputBuffer = std::make_unique<uint8[]>(k_ChunkTuples * numComponents);
    auto outputBuffer = std::make_unique<uint8[]>(k_ChunkTuples);

    for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
      const usize inputValueOffset = tupleOffset * numComponents;
      const usize inputValueCount = tupleCount * numComponents;
      Result<> readResult = m_InputColorData.copyIntoBuffer(inputValueOffset, nonstd::span<uint8>(inputBuffer.get(), inputValueCount));
      if(readResult.invalid())
      {
        return readResult;
      }

      convertChunk(inputBuffer.get(), outputBuffer.get(), tupleCount, numComponents);

      Result<> writeResult = m_OutputGrayData.copyFromBuffer(tupleOffset, nonstd::span<const uint8>(outputBuffer.get(), tupleCount));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }

    return {};
  }

private:
  /**
   * @brief Converts one buffered tuple range.
   * @param inputBuffer Provides RGB values.
   * @param outputBuffer Receives grayscale values.
   * @param tupleCount Specifies buffered tuple count.
   * @param numComponents Specifies input components per tuple.
   */
  void convertChunk(const uint8* inputBuffer, uint8* outputBuffer, usize tupleCount, usize numComponents) const
  {
    switch(m_ConversionType)
    {
    case ConvertColorToGrayScale::ConversionType::Luminosity:
      ConvertLuminosity(inputBuffer, outputBuffer, tupleCount, numComponents, m_ColorWeights);
      break;
    case ConvertColorToGrayScale::ConversionType::Average:
      ConvertLuminosity(inputBuffer, outputBuffer, tupleCount, numComponents, {0.3333F, 0.3333F, 0.3333F});
      break;
    case ConvertColorToGrayScale::ConversionType::Lightness:
      for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
      {
        const usize componentOffset = tupleIndex * numComponents;
        const auto minMax = std::minmax_element(inputBuffer + componentOffset, inputBuffer + componentOffset + 3);
        outputBuffer[tupleIndex] = static_cast<uint8>(roundf(static_cast<float>(static_cast<int16>(*minMax.first) + static_cast<int16>(*minMax.second)) / 2.0F));
      }
      break;
    case ConvertColorToGrayScale::ConversionType::SingleChannel:
      for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
      {
        outputBuffer[tupleIndex] = inputBuffer[tupleIndex * numComponents + static_cast<usize>(m_ColorChannel)];
      }
      break;
    }
  }

  /**
   * @brief Converts one buffered range with RGB weights.
   * @param inputBuffer Provides RGB values.
   * @param outputBuffer Receives grayscale values.
   * @param tupleCount Specifies buffered tuple count.
   * @param numComponents Specifies input components per tuple.
   * @param colorWeights Specifies RGB weights.
   */
  static void ConvertLuminosity(const uint8* inputBuffer, uint8* outputBuffer, usize tupleCount, usize numComponents, const FloatVec3Type& colorWeights)
  {
    for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
    {
      const usize componentOffset = tupleIndex * numComponents;
      const auto value = static_cast<int32>(
          roundf((inputBuffer[componentOffset] * colorWeights.getX()) + (inputBuffer[componentOffset + 1] * colorWeights.getY()) + (inputBuffer[componentOffset + 2] * colorWeights.getZ())));
      outputBuffer[tupleIndex] = static_cast<uint8>(value);
    }
  }

  const UInt8AbstractDataStoreType& m_InputColorData;
  UInt8AbstractDataStoreType& m_OutputGrayData;
  ConvertColorToGrayScale::ConversionType m_ConversionType;
  FloatVec3Type m_ColorWeights;
  int32 m_ColorChannel;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

ConvertColorToGrayScale::ConvertColorToGrayScale(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ConvertColorToGrayScaleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& ConvertColorToGrayScale::getCancel()
{
  return m_ShouldCancel;
}

Result<> ConvertColorToGrayScale::operator()()
{
  if(m_InputValues->output_data_array_paths.size() != m_InputValues->input_data_array_paths.size())
  {
    return MakeErrorResult(-10706, fmt::format("Grayscale conversion needs one output path for each input path. Input paths: {}. Output paths: {}.", m_InputValues->input_data_array_paths.size(),
                                               m_InputValues->output_data_array_paths.size()));
  }

  auto outputPathIter = m_InputValues->output_data_array_paths.begin();
  for(const auto& arrayPath : m_InputValues->input_data_array_paths)
  {
    m_MessageHandler.sendInfoMessage(fmt::format("Converting data '{}'", arrayPath.toString()));

    if(getCancel())
    {
      break;
    }
    const auto& inputColorArray = m_DataStructure.getDataRefAs<UInt8ArrayType>(arrayPath);
    auto& outputGrayArray = m_DataStructure.getDataRefAs<UInt8ArrayType>(*outputPathIter);
    const auto& inputColorData = inputColorArray.getDataStoreRef();
    auto& outputGrayData = outputGrayArray.getDataStoreRef();
    const auto conversionType = static_cast<ConversionType>(m_InputValues->conversion_algorithm);
    const FloatVec3Type colorWeights = m_InputValues->color_weights;

    Result<> result = DispatchAlgorithm<ConvertColorToGrayScaleDirect, ConvertColorToGrayScaleScanline>({&inputColorArray, &outputGrayArray}, inputColorData, outputGrayData, conversionType,
                                                                                                        colorWeights, m_InputValues->color_channel, m_ShouldCancel);
    if(result.invalid())
    {
      return result;
    }
    ++outputPathIter;
  }

  return {};
}

Result<> nx::core::ConvertColorToGrayScaleArrays(DataStructure& dataStructure, const ConvertColorToGrayScaleInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel)
{
  ConvertColorToGrayScaleInputValues resolvedValues = inputValues;
  if(resolvedValues.output_data_array_paths.empty())
  {
    for(const auto& inputDataArrayPath : resolvedValues.input_data_array_paths)
    {
      resolvedValues.output_data_array_paths.push_back(inputDataArrayPath.replaceName(resolvedValues.output_array_prefix + inputDataArrayPath.getTargetName()));
    }
  }
  return ConvertColorToGrayScale(dataStructure, messageHandler, shouldCancel, &resolvedValues)();
}
