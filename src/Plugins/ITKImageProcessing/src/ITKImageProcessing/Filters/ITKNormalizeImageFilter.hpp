#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKNormalizeImageFilter
 * @brief Normalize an image by setting its mean to zero and variance to one.
 *
 * NormalizeImageFilter shifts and scales an image so that the pixels in the image have a zero mean and unit variance. This filter uses StatisticsImageFilter to compute the mean and variance of the
 * input and then applies ShiftScaleImageFilter to shift and scale the pixels.
 *
 * NB: since this filter normalizes the data such that the mean is at 0, and \f$-\sigma\f$ to \f$+\sigma\f$ is mapped to -1.0 to 1.0, output image integral types will produce an image that DOES NOT
 * HAVE a unit variance due to 68% of the intensity values being mapped to the real number range of -1.0 to 1.0 and then cast to the output integral value.
 *
 * @see NormalizeToConstantImageFilter
 *
 * ITK Module: ITKImageIntensity
 * ITK Group: ImageIntensity
 */
class ITKIMAGEPROCESSING_EXPORT ITKNormalizeImageFilter : public IFilter
{
public:
  ITKNormalizeImageFilter() = default;
  ~ITKNormalizeImageFilter() noexcept override = default;

  ITKNormalizeImageFilter(const ITKNormalizeImageFilter&) = delete;
  ITKNormalizeImageFilter(ITKNormalizeImageFilter&&) noexcept = delete;

  ITKNormalizeImageFilter& operator=(const ITKNormalizeImageFilter&) = delete;
  ITKNormalizeImageFilter& operator=(ITKNormalizeImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKNormalizeImageFilter, "9d8ce30e-c75e-4ca8-b6be-0b11baa7e6ce");
