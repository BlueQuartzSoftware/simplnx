#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKShotNoiseImage
 * @brief Alter an image with shot noise.
 *
 * The shot noise follows a Poisson distribution:
 *
 * \par
 * \f$ I = N(I_0) \f$
 *
 *
 * \par
 * where \f$ N(I_0) \f$ is a Poisson-distributed random variable of mean \f$ I_0 \f$ . The noise is thus dependent on the pixel intensities in the image.
 *
 *
 * The intensities in the image can be scaled by a user provided value to map pixel values to the actual number of particles. The scaling can be seen as the inverse of the gain used during the
 * acquisition. The noisy signal is then scaled back to its input intensity range:
 *
 * \par
 * \f$ I = \frac{N(I_0 \times s)}{s} \f$
 *
 *
 * \par
 * where \f$ s \f$ is the scale factor.
 *
 *
 * The Poisson-distributed variable \f$ \lambda \f$ is computed by using the algorithm:
 *
 * \par
 * \f$ \begin{array}{l} k \leftarrow 0 \\ p \leftarrow 1 \\ \textbf{repeat} \\ \left\{ \begin{array}{l} k \leftarrow k+1 \\ p \leftarrow p \ast U() \end{array} \right. \\ \textbf{until } p >
 * e^{\lambda} \\ \textbf{return} (k) \end{array} \f$
 *
 *
 * \par
 * where \f$ U() \f$ provides a uniformly distributed random variable in the interval \f$ [0,1] \f$ .
 *
 *
 * This algorithm is very inefficient for large values of \f$ \lambda \f$ , though. Fortunately, the Poisson distribution can be accurately approximated by a Gaussian distribution of mean and variance
 * \f$ \lambda \f$ when \f$ \lambda \f$ is large enough. In this implementation, this value is considered to be 50. This leads to the faster algorithm:
 *
 * \par
 * \f$ \lambda + \sqrt{\lambda} \times N()\f$
 *
 *
 * \par
 * where \f$ N() \f$ is a normally distributed random variable of mean 0 and variance 1.
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
class ITKIMAGEPROCESSING_EXPORT ITKShotNoiseImage : public IFilter
{
public:
  ITKShotNoiseImage() = default;
  ~ITKShotNoiseImage() noexcept override = default;

  ITKShotNoiseImage(const ITKShotNoiseImage&) = delete;
  ITKShotNoiseImage(ITKShotNoiseImage&&) noexcept = delete;

  ITKShotNoiseImage& operator=(const ITKShotNoiseImage&) = delete;
  ITKShotNoiseImage& operator=(ITKShotNoiseImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_Scale_Key = "Scale";
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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKShotNoiseImage, "97f20f54-276b-54f3-87c9-5eaf16e6c4df");
