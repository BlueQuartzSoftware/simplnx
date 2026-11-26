#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyLayout.hpp"
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
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& output_array_path;
  SizeVec3 dims;
  const StructuringElement& se;
  const std::vector<SEOffset>& box_offsets;
  ObjectMorphOp op;
  float64 object_value;
  float64 background_value;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;

  template <class T>
  Result<> operator()() const
  {
    // The in/out IDataArrays are needed by ApplyObjectMorphology's DispatchAlgorithm to decide the storage
    // path; the typed stores are the buffers the engine reads/writes. Output type == input type (SameAsInput).
    // The Float64 object/background values are cast to the element type here; the caller's preflight guard has
    // already rejected non-finite/out-of-range values, so the narrowing conversion is well-defined.
    const auto& inArray = data_structure.getDataRefAs<DataArray<T>>(input_array_path);
    auto& outArray = data_structure.getDataRefAs<DataArray<T>>(output_array_path);
    return ApplyObjectMorphology<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, se, box_offsets, op, static_cast<T>(object_value), static_cast<T>(background_value), inArray, outArray,
                                    should_cancel, message_handler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteObjectMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                            ObjectMorphOp op, float64 objectValue, float64 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicyType = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, false, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  const std::vector<SEOffset> boxOffsets = detail::MakeFullBoxNeighborOffsets();
  detail::ObjectMorphologyExecuteFn executeFn{.data_structure = dataStructure,
                                              .input_array_path = inputArrayPath,
                                              .output_array_path = outputArrayPath,
                                              .dims = dims,
                                              .se = se,
                                              .box_offsets = boxOffsets,
                                              .op = op,
                                              .object_value = objectValue,
                                              .background_value = backgroundValue,
                                              .should_cancel = shouldCancel,
                                              .message_handler = messageHandler};
  return TypeSetPolicyType::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
