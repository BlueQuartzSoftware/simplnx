#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKBinaryThresholdImageFilter
 * @brief Binarize an input image by thresholding.
 *
 * This filter produces an output image whose pixels are either one of two values ( OutsideValue or InsideValue ), depending on whether the corresponding input image pixels lie between the two
 * thresholds ( LowerThreshold and UpperThreshold ). Values equal to either threshold is considered to be between the thresholds.
 *
 * More precisely \f[ Output(x_i) = \begin{cases} InsideValue & \text{if \f$LowerThreshold \leq x_i \leq UpperThreshold\f$} \\ OutsideValue & \text{otherwise} \end{cases} \f]
 *
 * This filter is templated over the input image type and the output image type.
 *
 * The filter expect both images to have the same number of dimensions.
 *
 * The default values for LowerThreshold and UpperThreshold are: LowerThreshold = NumericTraits<TInput>::NonpositiveMin() ; UpperThreshold = NumericTraits<TInput>::max() ; Therefore, generally only
 * one of these needs to be set, depending on whether the user wants to threshold above or below the desired threshold.
 *
 * ITK Module: ITKThresholding
 * ITK Group: Thresholding
 */
class ITKIMAGEPROCESSING_EXPORT ITKBinaryThresholdImageFilter : public IFilter
{
public:
  ITKBinaryThresholdImageFilter() = default;
  ~ITKBinaryThresholdImageFilter() noexcept override = default;

  ITKBinaryThresholdImageFilter(const ITKBinaryThresholdImageFilter&) = delete;
  ITKBinaryThresholdImageFilter(ITKBinaryThresholdImageFilter&&) noexcept = delete;

  ITKBinaryThresholdImageFilter& operator=(const ITKBinaryThresholdImageFilter&) = delete;
  ITKBinaryThresholdImageFilter& operator=(ITKBinaryThresholdImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_LowerThreshold_Key = "lower_threshold";
  static inline constexpr StringLiteral k_UpperThreshold_Key = "upper_threshold";
  static inline constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static inline constexpr StringLiteral k_OutsideValue_Key = "outside_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKBinaryThresholdImageFilter, "ba2494b0-c4f0-43ff-9d08-900395900e0c");
