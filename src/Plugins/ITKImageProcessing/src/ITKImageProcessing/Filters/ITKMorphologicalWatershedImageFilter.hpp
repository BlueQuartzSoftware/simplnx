#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKMorphologicalWatershedImageFilter
 * @brief Watershed segmentation implementation with morphological operators.
 *
 * Watershed pixel are labeled 0. TOutputImage should be an integer type. Labels of output image are in no particular order. You can reorder the labels such that object labels are consecutive and
 * sorted based on object size by passing the output of this filter to a RelabelComponentImageFilter .
 *
 * The morphological watershed transform algorithm is described in Chapter 9.2 of Pierre Soille's book "Morphological Image Analysis:
 * Principles and Applications", Second Edition, Springer, 2003.
 *
 * This code was contributed in the Insight Journal paper: "The watershed transform in ITK - discussion and new developments" by Beare R., Lehmann G. https://www.insight-
 * journal.org/browse/publication/92
 *
 * @author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 *
 * @see WatershedImageFilter , MorphologicalWatershedFromMarkersImageFilter
 *
 * ITK Module: ITKWatersheds
 * ITK Group: Watersheds
 */
class ITKIMAGEPROCESSING_EXPORT ITKMorphologicalWatershedImageFilter : public IFilter
{
public:
  ITKMorphologicalWatershedImageFilter() = default;
  ~ITKMorphologicalWatershedImageFilter() noexcept override = default;

  ITKMorphologicalWatershedImageFilter(const ITKMorphologicalWatershedImageFilter&) = delete;
  ITKMorphologicalWatershedImageFilter(ITKMorphologicalWatershedImageFilter&&) noexcept = delete;

  ITKMorphologicalWatershedImageFilter& operator=(const ITKMorphologicalWatershedImageFilter&) = delete;
  ITKMorphologicalWatershedImageFilter& operator=(ITKMorphologicalWatershedImageFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_Level_Key = "level";
  static inline constexpr StringLiteral k_MarkWatershedLine_Key = "mark_watershed_line";
  static inline constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKMorphologicalWatershedImageFilter, "f70337e5-4435-41f7-aecc-d79b4b1faccd");
