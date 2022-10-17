#pragma once

#include "SIMPL/SIMPL_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class CreateGeometry
 * @brief This filter will ....
 */
class SIMPL_EXPORT CreateGeometry : public IFilter
{
public:
  CreateGeometry() = default;
  ~CreateGeometry() noexcept override = default;

  CreateGeometry(const CreateGeometry&) = delete;
  CreateGeometry(CreateGeometry&&) noexcept = delete;

  CreateGeometry& operator=(const CreateGeometry&) = delete;
  CreateGeometry& operator=(CreateGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GeometryType_Key = "GeometryType";
  static inline constexpr StringLiteral k_TreatWarningsAsErrors_Key = "TreatWarningsAsErrors";
  static inline constexpr StringLiteral k_ArrayHandling_Key = "ArrayHandling";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_Dimensions_Key = "Dimensions";
  static inline constexpr StringLiteral k_Origin_Key = "Origin";
  static inline constexpr StringLiteral k_Spacing_Key = "Spacing";
  static inline constexpr StringLiteral k_ImageCellAttributeMatrixName_Key = "ImageCellAttributeMatrixName";
  static inline constexpr StringLiteral k_XBoundsArrayPath_Key = "XBoundsArrayPath";
  static inline constexpr StringLiteral k_YBoundsArrayPath_Key = "YBoundsArrayPath";
  static inline constexpr StringLiteral k_ZBoundsArrayPath_Key = "ZBoundsArrayPath";
  static inline constexpr StringLiteral k_RectGridCellAttributeMatrixName_Key = "RectGridCellAttributeMatrixName";
  static inline constexpr StringLiteral k_SharedVertexListArrayPath0_Key = "SharedVertexListArrayPath0";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName0_Key = "VertexAttributeMatrixName0";
  static inline constexpr StringLiteral k_SharedVertexListArrayPath1_Key = "SharedVertexListArrayPath1";
  static inline constexpr StringLiteral k_SharedEdgeListArrayPath_Key = "SharedEdgeListArrayPath";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName1_Key = "VertexAttributeMatrixName1";
  static inline constexpr StringLiteral k_EdgeAttributeMatrixName_Key = "EdgeAttributeMatrixName";
  static inline constexpr StringLiteral k_SharedVertexListArrayPath2_Key = "SharedVertexListArrayPath2";
  static inline constexpr StringLiteral k_SharedTriListArrayPath_Key = "SharedTriListArrayPath";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName2_Key = "VertexAttributeMatrixName2";
  static inline constexpr StringLiteral k_FaceAttributeMatrixName0_Key = "FaceAttributeMatrixName0";
  static inline constexpr StringLiteral k_SharedVertexListArrayPath3_Key = "SharedVertexListArrayPath3";
  static inline constexpr StringLiteral k_SharedQuadListArrayPath_Key = "SharedQuadListArrayPath";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName3_Key = "VertexAttributeMatrixName3";
  static inline constexpr StringLiteral k_FaceAttributeMatrixName1_Key = "FaceAttributeMatrixName1";
  static inline constexpr StringLiteral k_SharedVertexListArrayPath4_Key = "SharedVertexListArrayPath4";
  static inline constexpr StringLiteral k_SharedTetListArrayPath_Key = "SharedTetListArrayPath";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName4_Key = "VertexAttributeMatrixName4";
  static inline constexpr StringLiteral k_TetCellAttributeMatrixName_Key = "TetCellAttributeMatrixName";
  static inline constexpr StringLiteral k_SharedVertexListArrayPath5_Key = "SharedVertexListArrayPath5";
  static inline constexpr StringLiteral k_SharedHexListArrayPath_Key = "SharedHexListArrayPath";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName5_Key = "VertexAttributeMatrixName5";
  static inline constexpr StringLiteral k_HexCellAttributeMatrixName_Key = "HexCellAttributeMatrixName";

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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CreateGeometry, "52d105df-80d2-44bf-8cdd-e5ba1902076d");
