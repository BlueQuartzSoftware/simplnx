#include "NormalizeImage.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidInputArrayError = -94002;
constexpr int32 k_ComponentShapeError = -94001;
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
Result<> NormalizeImage::ValidateInputs(const DataStructure& dataStructure, const NormalizeImageInputValues& inputValues)
{
  const auto* input = dataStructure.getDataAs<IDataArray>(inputValues.InputArrayPath);
  if(input == nullptr)
  {
    return MakeErrorResult(k_InvalidInputArrayError,
                           fmt::format("Input path '{}' is missing or is not a numeric data array. Select an existing numeric data array.", inputValues.InputArrayPath.toString()));
  }
  if(input->getComponentShape() != ShapeType{1})
  {
    return MakeErrorResult(k_ComponentShapeError, fmt::format("Input array '{}' has component shape ({}), but requires {{1}}. Select an array with exactly one component dimension of size 1.",
                                                              inputValues.InputArrayPath.toString(), StringUtilities::formatDimensions(input->getComponentShape())));
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> NormalizeImage::operator()()
{
  if(auto validation = ValidateInputs(m_DataStructure, *m_InputValues); validation.invalid())
  {
    return validation;
  }
  const NormalizeOperation operation{};
  return ImageProcessing::ExecuteTwoPassImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPath,
                                                                                                                 operation, m_ShouldCancel, m_MessageHandler);
}
