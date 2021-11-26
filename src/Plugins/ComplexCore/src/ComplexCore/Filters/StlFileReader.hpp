#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class StlFileReader
 *
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT StlFileReader : public IFilter
{
public:
  StlFileReader() = default;
  ~StlFileReader() noexcept override = default;

  StlFileReader(const StlFileReader&) = delete;
  StlFileReader(StlFileReader&&) noexcept = delete;

  StlFileReader& operator=(const StlFileReader&) = delete;
  StlFileReader& operator=(StlFileReader&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_StlFilePath_Key = "StlFilePath";
  static inline constexpr StringLiteral k_ParentDataGroupPath_Key = "ParentGroupDataPath";
  static inline constexpr StringLiteral k_GeometryName_Key = "GeometryName";
  static inline constexpr StringLiteral k_FaceDataGroupName_Key = "FaceDataGroupName";
  static inline constexpr StringLiteral k_FaceNormalsArrayName_Key = "FaceNormalsArrayName";

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


private:
  // Holds the min/max coordinates for X, Y, Z laid out at { X_Min, X_Max, Y_Min, Y_Max, Z_Min, Z_Max}
  std::array<float, 6> m_MinMaxCoords = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
                                          std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
                                          std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, StlFileReader, "11608280-cec5-55d8-972b-d858a9caa74b");
