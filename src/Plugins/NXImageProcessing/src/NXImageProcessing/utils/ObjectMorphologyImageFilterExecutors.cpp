#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ObjectMorphologyEngine.hpp"

#include <atomic>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
struct ObjectMorphologyExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  const std::vector<SEOffset>& boxOffsets;
  ObjectMorphOp op;
  float64 objectValue;
  float64 backgroundValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // The in/out IDataArrays are needed by ApplyObjectMorphology's DispatchAlgorithm to decide the storage
    // path; the typed stores are the buffers the engine reads/writes. Output type == input type (SameAsInput).
    // The Float64 object/background values are cast to the element type here; the caller's preflight guard has
    // already rejected non-finite/out-of-range values, so the narrowing conversion is well-defined.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyObjectMorphology<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, se, boxOffsets, op, static_cast<T>(objectValue), static_cast<T>(backgroundValue), inArray, outArray,
                                    shouldCancel, messageHandler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteObjectMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                            ObjectMorphOp op, float64 objectValue, float64 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, false, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  const std::vector<SEOffset> boxOffsets = detail::MakeFullBoxNeighborOffsets();
  detail::ObjectMorphologyExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, boxOffsets, op, objectValue, backgroundValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
