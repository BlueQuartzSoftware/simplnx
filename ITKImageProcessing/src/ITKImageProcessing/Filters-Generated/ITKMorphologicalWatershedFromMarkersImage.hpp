#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKMorphologicalWatershedFromMarkersImage
 * @brief Morphological watershed transform from markers.
 *
 * The watershed transform is a tool for image segmentation that is fast and flexible and potentially fairly parameter free. It was originally derived from a geophysical model of rain falling on a
 * terrain and a variety of more formal definitions have been devised to allow development of practical algorithms. If an image is considered as a terrain and divided into catchment basins then the
 * hope is that each catchment basin would contain an object of interest.
 *
 * The output is a label image. A label image, sometimes referred to as a categorical image, has unique values for each region. For example, if a watershed produces 2 regions, all pixels belonging to
 * one region would have value A, and all belonging to the other might have value B. Unassigned pixels, such as watershed lines, might have the background value (0 by convention).
 *
 * The simplest way of using the watershed is to preprocess the image we want to segment so that the boundaries of our objects are bright (e.g apply an edge detector) and compute the watershed
 * transform of the edge image. Watershed lines will correspond to the boundaries and our problem will be solved. This is rarely useful in practice because there are always more regional minima than
 * there are objects, either due to noise or natural variations in the object surfaces. Therefore, while many watershed lines do lie on significant boundaries, there are many that don't. Various
 * methods can be used to reduce the number of minima in the image, like thresholding the smallest values, filtering the minima and/or smoothing the image.
 *
 * This filter use another approach to avoid the problem of over segmentation: it let the user provide a marker image which mark the minima in the input image and give them a label. The minima are
 * imposed in the input image by the markers. The labels of the output image are the label of the marker image.
 *
 * The morphological watershed transform algorithm is described in Chapter 9.2 of Pierre Soille's book "Morphological Image Analysis:
 *  Principles and Applications", Second Edition, Springer, 2003.
 *
 * This code was contributed in the Insight Journal paper: "The watershed transform in ITK - discussion and new developments" by Beare R., Lehmann G. https://hdl.handle.net/1926/202
 * http://www.insight-journal.org/browse/publication/92
 *
 * @author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 *
 * @author Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.
 *
 *
 * @see WatershedImageFilter , MorphologicalWatershedImageFilter
 *
 * ITK Module: ITKWatersheds
 * ITK Group: Watersheds
 */
class ITKIMAGEPROCESSING_EXPORT ITKMorphologicalWatershedFromMarkersImage : public IFilter
{
public:
  ITKMorphologicalWatershedFromMarkersImage() = default;
  ~ITKMorphologicalWatershedFromMarkersImage() noexcept override = default;

  ITKMorphologicalWatershedFromMarkersImage(const ITKMorphologicalWatershedFromMarkersImage&) = delete;
  ITKMorphologicalWatershedFromMarkersImage(ITKMorphologicalWatershedFromMarkersImage&&) noexcept = delete;

  ITKMorphologicalWatershedFromMarkersImage& operator=(const ITKMorphologicalWatershedFromMarkersImage&) = delete;
  ITKMorphologicalWatershedFromMarkersImage& operator=(ITKMorphologicalWatershedFromMarkersImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "InputImageDataPath";
  static inline constexpr StringLiteral k_OutputImageDataPath_Key = "OutputImageDataPath";
  static inline constexpr StringLiteral k_MarkWatershedLine_Key = "MarkWatershedLine";
  static inline constexpr StringLiteral k_FullyConnected_Key = "FullyConnected";
  static inline constexpr StringLiteral k_ImageDataPath_Key = "ImageDataPath";
  static inline constexpr StringLiteral k_MarkerImage_Key = "MarkerImage";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ITKMorphologicalWatershedFromMarkersImage, "81308863-094b-511d-9aa9-865e02e2b8d5");
