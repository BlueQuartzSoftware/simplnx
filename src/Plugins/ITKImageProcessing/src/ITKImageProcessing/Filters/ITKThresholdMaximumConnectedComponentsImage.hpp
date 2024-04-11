#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ITKThresholdMaximumConnectedComponentsImage
 * @brief Finds the threshold value of an image based on maximizing the number of objects in the image that are larger than a given minimal size.
 *
 * \par
 * This method is based on Topological Stable State Thresholding to calculate the threshold set point. This method is particularly effective when there are a large number of objects in a microscopy
 * image. Compiling in Debug mode and enable the debug flag for this filter to print debug information to see how the filter focuses in on a threshold value. Please see the Insight Journal's MICCAI
 * 2005 workshop for a complete description. References are below.
 *
 *
 * \par Parameters
 * The MinimumObjectSizeInPixels parameter is controlled through the class Get/SetMinimumObjectSizeInPixels() method. Similar to the standard itk::BinaryThresholdImageFilter the Get/SetInside and
 * Get/SetOutside values of the threshold can be set. The GetNumberOfObjects() and GetThresholdValue() methods return the number of objects above the minimum pixel size and the calculated threshold
 * value.
 *
 *
 * \par Automatic Thresholding in ITK
 * There are multiple methods to automatically calculate the threshold intensity value of an image. As of version 4.0, ITK has a Thresholding ( ITKThresholding ) module which contains numerous
 * automatic thresholding methods.implements two of these. Topological Stable State Thresholding works well on images with a large number of objects to be counted.
 *
 *
 * \par References:
 * 1) Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber
 * counting in immunofluorescent microscopy images". Insight Journal. ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005) https://insight-journal.org/browse/publication/40 2) Pikaz A, Averbuch,
 * A. "Digital image thresholding based on topological
 * stable-state". Pattern Recognition, 29(5): 829-843, 1996.
 *
 *
 * \par
 * Questions: email Ken Urish at ken.urish(at)gmail.com Please cc the itk list serve for archival purposes.
 *
 * ITK Module: ITKConnectedComponents
 * ITK Group: ConnectedComponents
 */
class ITKIMAGEPROCESSING_EXPORT ITKThresholdMaximumConnectedComponentsImage : public IFilter
{
public:
  ITKThresholdMaximumConnectedComponentsImage() = default;
  ~ITKThresholdMaximumConnectedComponentsImage() noexcept override = default;

  ITKThresholdMaximumConnectedComponentsImage(const ITKThresholdMaximumConnectedComponentsImage&) = delete;
  ITKThresholdMaximumConnectedComponentsImage(ITKThresholdMaximumConnectedComponentsImage&&) noexcept = delete;

  ITKThresholdMaximumConnectedComponentsImage& operator=(const ITKThresholdMaximumConnectedComponentsImage&) = delete;
  ITKThresholdMaximumConnectedComponentsImage& operator=(ITKThresholdMaximumConnectedComponentsImage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_SelectedImageDataPath_Key = "input_image_data_path";
  static inline constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static inline constexpr StringLiteral k_MinimumObjectSizeInPixels_Key = "minimum_object_size_in_pixels";
  static inline constexpr StringLiteral k_UpperBoundary_Key = "upper_boundary";
  static inline constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static inline constexpr StringLiteral k_OutsideValue_Key = "outside_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ITKThresholdMaximumConnectedComponentsImage, "dc0f6771-87bf-457f-8430-2d943e039a24");
