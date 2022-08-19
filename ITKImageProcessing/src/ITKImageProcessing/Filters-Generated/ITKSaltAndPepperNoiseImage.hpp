#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKSaltAndPepperNoiseImage
 * @brief Alter an image with fixed value impulse noise, often called salt and pepper noise.
 *
 * Salt (sensor saturation) and pepper (dead pixels) noise is a special kind of impulse noise where the value of the noise is either the maximum possible value in the image or its minimum. This is not
 * necessarily the maximal/minimal possible intensity value based on the pixel type. For example, the native pixel type for CT is a signed 16 bit integer, but only 12 bits used, so we would like to
 * set the salt and pepper values to match this smaller intensity range and not the range the pixel type represents. It can be modeled as:
 *
 * \par
 * \f$ I = \begin{cases} M, & \quad \text{if } U < p/2 \\ m, & \quad \text{if } U > 1 - p/2 \\ I_0, & \quad \text{if } p/2 \geq U \leq 1 - p/2 \end{cases} \f$
 *
 *
 * \par
 * where \f$ p \f$ is the probability of the noise event, \f$ U \f$ is a uniformly distributed random variable in the \f$ [0,1] \f$ range, \f$ M \f$ is the greatest possible pixel value, and \f$ m \f$
 * the smallest possible pixel value.
 *
 *
 * Pixel alteration occurs at a user defined probability. Salt and pepper pixels are equally distributed.
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
class ITKIMAGEPROCESSING_EXPORT ITKSaltAndPepperNoiseImage : public IFilter
{
public:
  ITKSaltAndPepperNoiseImage() = default;
  ~ITKSaltAndPepperNoiseImage() noexcept override = default;

  ITKSaltAndPepperNoiseImage(const ITKSaltAndPepperNoiseImage&) = delete;
  ITKSaltAndPepperNoiseImage(ITKSaltAndPepperNoiseImage&&) noexcept = delete;

  ITKSaltAndPepperNoiseImage& operator=(const ITKSaltAndPepperNoiseImage&) = delete;
  ITKSaltAndPepperNoiseImage& operator=(ITKSaltAndPepperNoiseImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_Probability_Key = "Probability";
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
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Boolean that gets set if the filter should stop executing and return
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKSaltAndPepperNoiseImage, "90e69ba1-5480-49af-835d-ae2082a70247");
