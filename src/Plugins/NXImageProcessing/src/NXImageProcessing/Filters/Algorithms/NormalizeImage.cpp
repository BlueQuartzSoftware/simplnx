#include "NormalizeImage.hpp"

#include "NXImageProcessing/utils/ImageFilterTemplateExecutors.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct NormalizeOperation
{
  template <class T>
  Result<ImageProcessing::ArrayStatistics<T>> computeStatistics(const AbstractDataStore<T>& inputStore, const std::atomic_bool& shouldCancel) const
  {
    return ImageProcessing::ComputeNormalizeStatistics(inputStore, shouldCancel);
  }

  template <class T>
  Result<> validate(const ImageProcessing::ArrayStatistics<T>& /*stats*/) const
  {
    return {};
  }
  template <class T, class U>
  auto makeMapOp(const ImageProcessing::ArrayStatistics<T>& stats) const
  {
    const float64 mean = stats.mean;
    const float64 sigma = stats.sigma;
    return [mean, sigma](T value) -> U { return static_cast<U>((static_cast<float64>(value) - mean) / sigma); };
  }
};
} // namespace

// -----------------------------------------------------------------------------
NormalizeImage::NormalizeImage(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const NormalizeImageInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(messageHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
NormalizeImage::~NormalizeImage() noexcept = default;

// -----------------------------------------------------------------------------
Result<> NormalizeImage::operator()()
{
  const NormalizeOperation operation{};
  return ImageProcessing::ExecuteTwoPassImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPath,
                                                                                                                 operation, m_ShouldCancel, m_MessageHandler);
}
