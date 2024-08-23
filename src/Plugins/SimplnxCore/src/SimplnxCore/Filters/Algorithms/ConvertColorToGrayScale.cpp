#include "ConvertColorToGrayScale.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <bool BoundsCheckV>
class LuminosityImpl
{
public:
  LuminosityImpl(const UInt8AbstractDataStore& data, UInt8AbstractDataStore& outputData, const FloatVec3& colorWeights, size_t numComp)
  : m_ImageData(data)
  , m_FlatImageData(outputData)
  , m_ColorWeights(colorWeights)
  , m_NumComp(numComp)
  {
  }
  LuminosityImpl(const LuminosityImpl&) = default;           // Copy Constructor Not Implemented
  LuminosityImpl(LuminosityImpl&&) = default;                // Move Constructor Not Implemented
  LuminosityImpl& operator=(const LuminosityImpl&) = delete; // Copy Assignment Not Implemented
  LuminosityImpl& operator=(LuminosityImpl&&) = delete;      // Move Assignment Not Implemented
  ~LuminosityImpl() = default;

  // Careful when converting from negative floats to unsigned ints. This is why
  // there is a double static cast.
  // C Standard: Section 6.3.1 Arithmetic operands (specifically Section 6.3.1.4 Real floating and integer)
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
  const UInt8AbstractDataStore& m_ImageData;
  UInt8AbstractDataStore& m_FlatImageData;
  const FloatVec3& m_ColorWeights;
  size_t m_NumComp;
};

class LightnessImpl
{
public:
  LightnessImpl(const UInt8AbstractDataStore& data, UInt8AbstractDataStore& outputData, size_t numComp)
  : m_ImageData(data)
  , m_FlatImageData(outputData)
  , m_NumComp(numComp)
  {
  }
  LightnessImpl(const LightnessImpl&) = default;           // Copy Constructor Not Implemented
  LightnessImpl(LightnessImpl&&) = default;                // Move Constructor Not Implemented
  LightnessImpl& operator=(const LightnessImpl&) = delete; // Copy Assignment Not Implemented
  LightnessImpl& operator=(LightnessImpl&&) = delete;      // Move Assignment Not Implemented
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
  const UInt8AbstractDataStore& m_ImageData;
  UInt8AbstractDataStore& m_FlatImageData;
  size_t m_NumComp;
};

template <bool BoundsCheckV>
class SingleChannelImpl
{
public:
  SingleChannelImpl(const UInt8AbstractDataStore& data, UInt8AbstractDataStore& outputData, size_t numComp, int32_t channel)
  : m_ImageData(data)
  , m_FlatImageData(outputData)
  , m_NumComp(numComp)
  , m_Channel(channel)
  {
  }
  SingleChannelImpl(const SingleChannelImpl&) = default;           // Copy Constructor Not Implemented
  SingleChannelImpl(SingleChannelImpl&&) = default;                // Move Constructor Not Implemented
  SingleChannelImpl& operator=(const SingleChannelImpl&) = delete; // Copy Assignment Not Implemented
  SingleChannelImpl& operator=(SingleChannelImpl&&) = delete;      // Move Assignment Not Implemented
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
  const UInt8AbstractDataStore& m_ImageData;
  UInt8AbstractDataStore& m_FlatImageData;
  size_t m_NumComp;
  int32_t m_Channel;
};

class ParallelWrapper
{
public:
  ~ParallelWrapper() = default;
  ParallelWrapper(const ParallelWrapper&) = delete;            // Copy Constructor Not Implemented
  ParallelWrapper(ParallelWrapper&&) = delete;                 // Move Constructor Not Implemented
  ParallelWrapper& operator=(const ParallelWrapper&) = delete; // Copy Assignment Not Implemented
  ParallelWrapper& operator=(ParallelWrapper&&) = delete;      // Move Assignment Not Implemented

  template <typename T>
  static void Run(T impl, size_t totalPoints, const typename IParallelAlgorithm::AlgorithmStores& algStores)
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, totalPoints);
    dataAlg.requireStoresInMemory(algStores);
    dataAlg.execute(impl);
  }

protected:
  ParallelWrapper() = default;
};

} // namespace

// -----------------------------------------------------------------------------
ConvertColorToGrayScale::ConvertColorToGrayScale(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ConvertColorToGrayScaleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertColorToGrayScale::~ConvertColorToGrayScale() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertColorToGrayScale::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertColorToGrayScale::operator()()
{

  auto outputPathIter = m_InputValues->OutputDataArrayPaths.begin();
  for(const auto& arrayPath : m_InputValues->InputDataArrayPaths)
  {
    if(getCancel())
    {
      break;
    }
    const auto& inputColorData = m_DataStructure.getDataAs<UInt8Array>(arrayPath)->getDataStoreRef();
    auto& outputGrayData = m_DataStructure.getDataAs<UInt8Array>(*outputPathIter)->getDataStoreRef();

    auto convType = static_cast<ConversionType>(m_InputValues->ConversionAlgorithm);

    size_t comp = inputColorData.getNumberOfComponents();
    size_t totalPoints = inputColorData.getNumberOfTuples();

    typename IParallelAlgorithm::AlgorithmStores algStores;
    algStores.push_back(&inputColorData);
    algStores.push_back(&outputGrayData);

    switch(convType)
    {
    case ConversionType::Luminosity:
      if(comp < 3) // Pre-check bounds to try to avoid `.at()`; algorithm hardcoded a component access size of 3
      {
        // Do bounds check
        ParallelWrapper::Run<LuminosityImpl<true>>(LuminosityImpl<true>(inputColorData, outputGrayData, m_InputValues->ColorWeights, comp), totalPoints, algStores);
      }
      else
      {
        ParallelWrapper::Run<LuminosityImpl<false>>(LuminosityImpl<false>(inputColorData, outputGrayData, m_InputValues->ColorWeights, comp), totalPoints, algStores);
      }
      break;
    case ConversionType::Average:
      if(comp < 3) // Pre-check bounds to try to avoid `.at()`; algorithm hardcoded a component access size of 3
      {
        // Do bounds check
        ParallelWrapper::Run<LuminosityImpl<true>>(LuminosityImpl<true>(inputColorData, outputGrayData, {0.3333f, 0.3333f, 0.3333f}, comp), totalPoints, algStores);
      }
      else
      {
        ParallelWrapper::Run<LuminosityImpl<false>>(LuminosityImpl<false>(inputColorData, outputGrayData, {0.3333f, 0.3333f, 0.3333f}, comp), totalPoints, algStores);
      }
      break;
    case ConversionType::Lightness: {
      ParallelWrapper::Run<LightnessImpl>(LightnessImpl(inputColorData, outputGrayData, comp), totalPoints, algStores);
      break;
    }
    case ConversionType::SingleChannel:
      if(comp * (totalPoints - 1) + m_InputValues->ColorChannel < inputColorData.getSize()) // Pre-check bounds to try to avoid `.at()`
      {
        // Do bounds check
        ParallelWrapper::Run<SingleChannelImpl<false>>(SingleChannelImpl<false>(inputColorData, outputGrayData, comp, m_InputValues->ColorChannel), totalPoints, algStores);
      }
      else
      {
        ParallelWrapper::Run<SingleChannelImpl<true>>(SingleChannelImpl<true>(inputColorData, outputGrayData, comp, m_InputValues->ColorChannel), totalPoints, algStores);
      }
      break;
    }
  }

  return {};
}
