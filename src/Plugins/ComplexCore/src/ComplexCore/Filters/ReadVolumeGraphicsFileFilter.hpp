#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ReadVolumeGraphicsFileFilter
 * @brief This filter will import Volume Graphics data files in the form of .vgi/.vol pairs.
 * Both files must exist and be in the same directory for the filter to work. The .vgi file
 * is read to find out the dimensions, spacing and units of the data. The name of the .vol
 * file is also contained in the .vgi file.
 */
class COMPLEXCORE_EXPORT ReadVolumeGraphicsFileFilter : public IFilter
{
public:
  ReadVolumeGraphicsFileFilter();
  ~ReadVolumeGraphicsFileFilter() noexcept override;

  ReadVolumeGraphicsFileFilter(const ReadVolumeGraphicsFileFilter&) = delete;
  ReadVolumeGraphicsFileFilter(ReadVolumeGraphicsFileFilter&&) noexcept = delete;

  ReadVolumeGraphicsFileFilter& operator=(const ReadVolumeGraphicsFileFilter&) = delete;
  ReadVolumeGraphicsFileFilter& operator=(ReadVolumeGraphicsFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_VGHeaderFile_Key = "vg_header_file";
  static inline constexpr StringLiteral k_NewImageGeometry_Key = "new_image_geometry";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_DensityArrayName_Key = "density_array_name";

  struct HeaderMetadata
  {
    SizeVec3 Dimensions;
    FloatVec3 Spacing;
    IGeometry::LengthUnit Units = IGeometry::LengthUnit::Unspecified;
    std::string DataFilePath;
  };

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
   * @param shouldCancel Should the current filter be canceled.
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The pipeline filter node
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Should the current filter be canceled.
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;

private:
  int32 m_InstanceId;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ReadVolumeGraphicsFileFilter, "dfd93665-b455-499a-abed-36ae7dbcdf57");
