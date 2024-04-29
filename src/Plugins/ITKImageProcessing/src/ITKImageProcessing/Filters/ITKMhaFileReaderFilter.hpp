#pragma once

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

namespace nx::core
{
class ITKIMAGEPROCESSING_EXPORT ITKMhaFileReaderFilter : public IFilter
{
public:
  ITKMhaFileReaderFilter() = default;
  ~ITKMhaFileReaderFilter() noexcept override = default;

  ITKMhaFileReaderFilter(const ITKMhaFileReaderFilter&) = delete;
  ITKMhaFileReaderFilter(ITKMhaFileReaderFilter&&) noexcept = delete;

  ITKMhaFileReaderFilter& operator=(const ITKMhaFileReaderFilter&) = delete;
  ITKMhaFileReaderFilter& operator=(ITKMhaFileReaderFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ApplyImageTransformation = "apply_image_transformation";
  static inline constexpr StringLiteral k_SaveImageTransformationAsArray = "save_image_transformation";
  static inline constexpr StringLiteral k_TransformationMatrixDataArrayPathKey = "output_transformation_matrix_path";
  static inline constexpr StringLiteral k_TransposeTransformMatrix = "transpose_transform_matrix";
  static inline constexpr StringLiteral k_InterpolationTypeKey = "interpolation_type_index";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  [[nodiscard]] Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKMhaFileReaderFilter, "41c33a08-0052-4915-8d53-d503f85f30d9");
