#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKSpeckleNoiseImage
 * @brief Alter an image with speckle (multiplicative) noise.
 *
 * The speckle noise follows a gamma distribution of mean 1 and standard deviation provided by the user. The noise is proportional to the pixel intensity.
 *
 * It can be modeled as:
 *
 * \par
 * \f$ I = I_0 \ast G \f$
 *
 *
 * \par
 * where \f$ G \f$ is a is a gamma distributed random variable of mean 1 and variance proportional to the noise level:
 *
 *
 * \par
 * \f$ G \sim \Gamma(\frac{1}{\sigma^2}, \sigma^2) \f$
 *
 *
 * @author Gaetan Lehmann
 *
 *
 * This code was contributed in the Insight Journal paper "Noise
 *  Simulation". https://hdl.handle.net/10380/3158
 *
 * ITK Module: ITKImageNoise
 * ITK Group: ImageNoise
 */
class ITKIMAGEPROCESSING_EXPORT ITKSpeckleNoiseImage : public IFilter
{
public:
  ITKSpeckleNoiseImage() = default;
  ~ITKSpeckleNoiseImage() noexcept override = default;

  ITKSpeckleNoiseImage(const ITKSpeckleNoiseImage&) = delete;
  ITKSpeckleNoiseImage(ITKSpeckleNoiseImage&&) noexcept = delete;

  ITKSpeckleNoiseImage& operator=(const ITKSpeckleNoiseImage&) = delete;
  ITKSpeckleNoiseImage& operator=(ITKSpeckleNoiseImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_StandardDeviation_Key = "StandardDeviation";
  static inline constexpr StringLiteral k_Seed_Key = "Seed";

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
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKSpeckleNoiseImage, "764085a4-6ecb-5fb7-891d-2fda208ba5d8");
