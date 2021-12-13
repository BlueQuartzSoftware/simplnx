#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKSignedMaurerDistanceMapImage
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT ITKSignedMaurerDistanceMapImage : public IFilter
{
public:
  ITKSignedMaurerDistanceMapImage() = default;
  ~ITKSignedMaurerDistanceMapImage() noexcept override = default;

  ITKSignedMaurerDistanceMapImage(const ITKSignedMaurerDistanceMapImage&) = delete;
  ITKSignedMaurerDistanceMapImage(ITKSignedMaurerDistanceMapImage&&) noexcept = delete;

  ITKSignedMaurerDistanceMapImage& operator=(const ITKSignedMaurerDistanceMapImage&) = delete;
  ITKSignedMaurerDistanceMapImage& operator=(ITKSignedMaurerDistanceMapImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InsideIsPositive_Key = "InsideIsPositive";
  static inline constexpr StringLiteral k_SquaredDistance_Key = "SquaredDistance";
  static inline constexpr StringLiteral k_UseImageSpacing_Key = "UseImageSpacing";
  static inline constexpr StringLiteral k_BackgroundValue_Key = "BackgroundValue";
  static inline constexpr StringLiteral k_SelectedCellArrayPath_Key = "SelectedCellArrayPath";
  static inline constexpr StringLiteral k_NewCellArrayName_Key = "NewCellArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKSignedMaurerDistanceMapImage, "bb15d42a-3077-582a-be1a-76b2bae172e9");
