#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ITKImageWriter
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT ITKImageWriter : public IFilter
{
public:
  ITKImageWriter() = default;
  ~ITKImageWriter() noexcept override = default;

  ITKImageWriter(const ITKImageWriter&) = delete;
  ITKImageWriter(ITKImageWriter&&) noexcept = delete;

  ITKImageWriter& operator=(const ITKImageWriter&) = delete;
  ITKImageWriter& operator=(ITKImageWriter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Plane_Key = "plane";
  static inline constexpr StringLiteral k_FileName_Key = "file_name";
  static inline constexpr StringLiteral k_IndexOffset_Key = "index_offset";
  static inline constexpr StringLiteral k_ImageArrayPath_Key = "image_array_path";
  static inline constexpr StringLiteral k_ImageGeomPath_Key = "image_geom_path";

  static inline constexpr usize k_XYPlane = 0;
  static inline constexpr usize k_XZPlane = 1;
  static inline constexpr usize k_YZPlane = 2;

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
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ITKImageWriter, "a181ee3e-1678-4133-b9c5-a9dd7bfec62f");
