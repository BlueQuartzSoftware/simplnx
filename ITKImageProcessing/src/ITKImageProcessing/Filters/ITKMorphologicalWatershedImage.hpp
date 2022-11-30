#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKMorphologicalWatershedImage
 * @brief Watershed segmentation implementation with morphological operators.
 *
 * Watershed pixel are labeled 0. TOutputImage should be an integer type. Labels of output image are in no particular order. You can reorder the labels such that object labels are consecutive and
 * sorted based on object size by passing the output of this filter to a RelabelComponentImageFilter .
 *
 * The morphological watershed transform algorithm is described in Chapter 9.2 of Pierre Soille's book "Morphological Image Analysis:
 * Principles and Applications", Second Edition, Springer, 2003.
 *
 * This code was contributed in the Insight Journal paper: "The watershed transform in ITK - discussion and new developments" by Beare R., Lehmann G. https://hdl.handle.net/1926/202
 * http://www.insight-journal.org/browse/publication/92
 *
 * @author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 *
 * @see WatershedImageFilter , MorphologicalWatershedFromMarkersImageFilter
 *
 * ITK Module: ITKWatersheds
 * ITK Group: Watersheds
 */
class ITKIMAGEPROCESSING_EXPORT ITKMorphologicalWatershedImage : public IFilter
{
public:
  ITKMorphologicalWatershedImage() = default;
  ~ITKMorphologicalWatershedImage() noexcept override = default;

  ITKMorphologicalWatershedImage(const ITKMorphologicalWatershedImage&) = delete;
  ITKMorphologicalWatershedImage(ITKMorphologicalWatershedImage&&) noexcept = delete;

  ITKMorphologicalWatershedImage& operator=(const ITKMorphologicalWatershedImage&) = delete;
  ITKMorphologicalWatershedImage& operator=(ITKMorphologicalWatershedImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "output_image_data_path";
  static inline constexpr StringLiteral k_Level_Key = "level";
  static inline constexpr StringLiteral k_MarkWatershedLine_Key = "mark_watershed_line";
  static inline constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKMorphologicalWatershedImage, "f70337e5-4435-41f7-aecc-d79b4b1faccd");
