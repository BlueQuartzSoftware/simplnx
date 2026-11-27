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
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& mask_array_path;
  const DataPath& output_array_path;
  usize num_components;
  float64 outside_value;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inputStore = data_structure.getDataRefAs<DataArray<T>>(input_array_path).getDataStoreRef();
    auto& outputStore = data_structure.getDataRefAs<DataArray<T>>(output_array_path).getDataStoreRef();
    const auto& maskArray = data_structure.getDataRefAs<IDataArray>(mask_array_path);
    return ApplyMask<T>(inputStore, outputStore, maskArray, num_components, static_cast<T>(outside_value), should_cancel, message_handler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteMaskImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& maskArrayPath, const DataPath& outputArrayPath, float64 outsideValue,
                                const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicyType = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const usize numComponents = inputArray.getNumberOfComponents();
  detail::MaskExecuteFn executeFn{.data_structure = dataStructure,
                                  .input_array_path = inputArrayPath,
                                  .mask_array_path = maskArrayPath,
                                  .output_array_path = outputArrayPath,
                                  .num_components = numComponents,
                                  .outside_value = outsideValue,
                                  .should_cancel = shouldCancel,
                                  .message_handler = messageHandler};
  return TypeSetPolicyType::Dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
