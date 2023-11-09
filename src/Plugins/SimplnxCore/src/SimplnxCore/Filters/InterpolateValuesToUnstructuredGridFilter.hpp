#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT InterpolateValuesToUnstructuredGridFilter : public IFilter
{
public:
  InterpolateValuesToUnstructuredGridFilter() = default;
  ~InterpolateValuesToUnstructuredGridFilter() noexcept override = default;

  InterpolateValuesToUnstructuredGridFilter(const InterpolateValuesToUnstructuredGridFilter&) = delete;
  InterpolateValuesToUnstructuredGridFilter(InterpolateValuesToUnstructuredGridFilter&&) noexcept = delete;

  InterpolateValuesToUnstructuredGridFilter& operator=(const InterpolateValuesToUnstructuredGridFilter&) = delete;
  InterpolateValuesToUnstructuredGridFilter& operator=(InterpolateValuesToUnstructuredGridFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SourceGeometry_Key = "source_geometry";
  static inline constexpr StringLiteral k_InterpolatedArrayPaths_Key = "interpolated_array_paths";
  static inline constexpr StringLiteral k_UseExistingAttrMatrix_Key = "use_existing_attr_matrix";
  static inline constexpr StringLiteral k_ExistingAttrMatrix_Key = "existing_attr_matrix";
  static inline constexpr StringLiteral k_CreatedAttrMatrix_Key = "created_attr_matrix";
  static inline constexpr StringLiteral k_DestinationGeometry_Key = "destination_geometry";

  /**
   * @brief
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, InterpolateValuesToUnstructuredGridFilter, "d8477024-7f4a-44eb-ad3f-aed6d07e972b");
