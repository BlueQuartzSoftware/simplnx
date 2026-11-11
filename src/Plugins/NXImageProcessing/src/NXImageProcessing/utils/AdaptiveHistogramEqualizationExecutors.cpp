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
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  std::array<usize, 3> radius;
  float32 alpha;
  float32 beta;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Output type == input type (SameAsInput). The engine is single-implementation (no DispatchAlgorithm), so
    // it needs only the typed stores.
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    return ApplyAdaptiveHistogramEqualization<T>(inputStore, outputStore, dims, radius, alpha, beta, shouldCancel, messageHandler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteAdaptiveHistogramEqualizationImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                         const std::array<usize, 3>& radius, float32 alpha, float32 beta, const std::atomic_bool& shouldCancel,
                                                         const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::AdaptiveHistogramEqualizationExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, radius, alpha, beta, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
