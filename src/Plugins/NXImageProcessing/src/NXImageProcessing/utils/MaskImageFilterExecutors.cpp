#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MaskEngine.hpp"

#include <atomic>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
struct MaskExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& maskArrayPath;
  const DataPath& outputArrayPath;
  usize numComponents;
  float64 outsideValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);
    return ApplyMask<T>(inputStore, outputStore, maskArray, numComponents, static_cast<T>(outsideValue), shouldCancel, messageHandler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteMaskImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& maskArrayPath, const DataPath& outputArrayPath, float64 outsideValue,
                                const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const usize numComponents = inputArray.getNumberOfComponents();
  detail::MaskExecuteFn executeFn{dataStructure, inputArrayPath, maskArrayPath, outputArrayPath, numComponents, outsideValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
