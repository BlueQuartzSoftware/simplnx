#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKZeroCrossingImageFilter
 * @brief This filter finds the closest pixel to the zero-crossings (sign changes) in a signed itk::Image .
 *
 * Pixels closest to zero-crossings are labeled with a foreground value. All other pixels are marked with a background value. The algorithm works by detecting differences in sign among neighbors using
 * city-block style connectivity (4-neighbors in 2d, 6-neighbors in 3d, etc.).
 *
 * \par Inputs and Outputs
 * The input to this filter is an itk::Image of arbitrary dimension. The algorithm assumes a signed data type (zero-crossings are not defined for unsigned int data types), and requires that operator>,
 * operator<, operator==, and operator!= are defined.
 *
 *
 * \par
 * The output of the filter is a binary, labeled image of user-specified type. By default, zero-crossing pixels are labeled with a default "foreground" value of
 * itk::NumericTraits<OutputDataType>::OneValue() , where OutputDataType is the data type of the output image. All other pixels are labeled with a default "background" value of
 * itk::NumericTraits<OutputDataType>::ZeroValue() .
 *
 *
 * \par Parameters
 * There are two parameters for this filter. ForegroundValue is the value that marks zero-crossing pixels. The BackgroundValue is the value given to all other pixels.
 *
 *
 * @see Image
 *
 *
 * @see Neighborhood
 *
 *
 * @see NeighborhoodOperator
 *
 *
 * @see NeighborhoodIterator
 *
 * ITK Module: ITKImageFeature
 * ITK Group: ImageFeature
 */
class ITKIMAGEPROCESSING_EXPORT ITKZeroCrossingImageFilter : public IFilter
{
public:
  ITKZeroCrossingImageFilter() = default;
  ~ITKZeroCrossingImageFilter() noexcept override = default;

  ITKZeroCrossingImageFilter(const ITKZeroCrossingImageFilter&) = delete;
  ITKZeroCrossingImageFilter(ITKZeroCrossingImageFilter&&) noexcept = delete;

  ITKZeroCrossingImageFilter& operator=(const ITKZeroCrossingImageFilter&) = delete;
  ITKZeroCrossingImageFilter& operator=(ITKZeroCrossingImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static inline constexpr StringLiteral k_BackgroundValue_Key = "background_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKZeroCrossingImageFilter, "89a14057-776a-4e35-80b6-69361e078394");
