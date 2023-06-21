#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

namespace complex
{
class ITKIMAGEPROCESSING_EXPORT ITKMhaFileReader : public IFilter
{
public:
  ITKMhaFileReader() = default;
  ~ITKMhaFileReader() noexcept override = default;

  ITKMhaFileReader(const ITKMhaFileReader&) = delete;
  ITKMhaFileReader(ITKMhaFileReader&&) noexcept = delete;

  ITKMhaFileReader& operator=(const ITKMhaFileReader&) = delete;
  ITKMhaFileReader& operator=(ITKMhaFileReader&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ApplyImageTransformation = "apply_image_transformation";
  static inline constexpr StringLiteral k_SaveImageTransformationAsArray = "save_image_transformation";
  static inline constexpr StringLiteral k_TransformationMatrixDataArrayPath_Key = "transformation_matrix_data_array_path";

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Parameters parameters() const override;

  /**
   * @brief
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
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKMhaFileReader, "41c33a08-0052-4915-8d53-d503f85f30d9");
