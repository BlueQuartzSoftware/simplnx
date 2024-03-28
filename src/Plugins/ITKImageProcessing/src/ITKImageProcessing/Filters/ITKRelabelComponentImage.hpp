#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKRelabelComponentImage
 * @brief Relabel the components in an image such that consecutive labels are used.
 *
 * RelabelComponentImageFilter remaps the labels associated with the objects in an image (as from the output of ConnectedComponentImageFilter ) such that the label numbers are consecutive with no gaps
 * between the label numbers used. By default, the relabeling will also sort the labels based on the size of the object: the largest object will have label #1, the second largest will have label #2,
 * etc. If two labels have the same size their initial order is kept. The sorting by size can be disabled using SetSortByObjectSize.
 *
 * Label #0 is assumed to be the background and is left unaltered by the relabeling.
 *
 * RelabelComponentImageFilter is typically used on the output of the ConnectedComponentImageFilter for those applications that want to extract the largest object or the "k" largest objects. Any
 * particular object can be extracted from the relabeled output using a BinaryThresholdImageFilter . A group of objects can be extracted from the relabeled output using a ThresholdImageFilter .
 *
 * Once all the objects are relabeled, the application can query the number of objects and the size of each object. Object sizes are returned in a vector. The size of the background is not calculated.
 * So the size of object #1 is GetSizeOfObjectsInPixels() [0], the size of object #2 is GetSizeOfObjectsInPixels() [1], etc.
 *
 * If user sets a minimum object size, all objects with fewer pixels than the minimum will be discarded, so that the number of objects reported will be only those remaining. The
 * GetOriginalNumberOfObjects method can be called to find out how many objects were present before the small ones were discarded.
 *
 * RelabelComponentImageFilter can be run as an "in place" filter, where it will overwrite its output. The default is run out of place (or generate a separate output). "In place" operation can be
 * controlled via methods in the superclass, InPlaceImageFilter::InPlaceOn() and InPlaceImageFilter::InPlaceOff().
 *
 * @see ConnectedComponentImageFilter , BinaryThresholdImageFilter , ThresholdImageFilter
 *
 * ITK Module: ITKConnectedComponents
 * ITK Group: ConnectedComponents
 */
class ITKIMAGEPROCESSING_EXPORT ITKRelabelComponentImage : public IFilter
{
public:
  ITKRelabelComponentImage() = default;
  ~ITKRelabelComponentImage() noexcept override = default;

  ITKRelabelComponentImage(const ITKRelabelComponentImage&) = delete;
  ITKRelabelComponentImage(ITKRelabelComponentImage&&) noexcept = delete;

  ITKRelabelComponentImage& operator=(const ITKRelabelComponentImage&) = delete;
  ITKRelabelComponentImage& operator=(ITKRelabelComponentImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geom_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_MinimumObjectSize_Key = "minimum_object_size";
  static inline constexpr StringLiteral k_SortByObjectSize_Key = "sort_by_object_size";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKRelabelComponentImage, "37e29d16-1020-478c-a506-c121e8f670ad");
