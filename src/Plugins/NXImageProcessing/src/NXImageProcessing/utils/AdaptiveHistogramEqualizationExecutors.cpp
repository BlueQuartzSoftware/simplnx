#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/AdaptiveHistogramEqualizationEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include <array>
#include <atomic>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
struct AdaptiveHistogramEqualizationExecuteFn
{
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& output_array_path;
  SizeVec3Type dims;
  std::array<usize, 3> radius;
  float32 alpha;
  float32 beta;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;

  template <class T>
  Result<> operator()() const
  {
    // Output type == input type (SameAsInput). The engine is single-implementation (no DispatchAlgorithm), so
    // it needs only the typed stores.
    const auto& inputStore = data_structure.getDataRefAs<DataArray<T>>(input_array_path).getDataStoreRef();
    auto& outputStore = data_structure.getDataRefAs<DataArray<T>>(output_array_path).getDataStoreRef();
    return ApplyAdaptiveHistogramEqualization<T>(inputStore, outputStore, dims, radius, alpha, beta, should_cancel, message_handler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteAdaptiveHistogramEqualizationImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                         const std::array<usize, 3>& radius, float32 alpha, float32 beta, const std::atomic_bool& shouldCancel,
                                                         const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicyType = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3Type dims = imageGeom.getDimensions();
  detail::AdaptiveHistogramEqualizationExecuteFn executeFn{.data_structure = dataStructure,
                                                           .input_array_path = inputArrayPath,
                                                           .output_array_path = outputArrayPath,
                                                           .dims = dims,
                                                           .radius = radius,
                                                           .alpha = alpha,
                                                           .beta = beta,
                                                           .should_cancel = shouldCancel,
                                                           .message_handler = messageHandler};
  return TypeSetPolicyType::Dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
