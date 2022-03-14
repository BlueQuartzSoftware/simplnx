#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT CropVertexGeometry : public IFilter
{
public:
  CropVertexGeometry() = default;
  ~CropVertexGeometry() noexcept override = default;

  CropVertexGeometry(const CropVertexGeometry&) = delete;
  CropVertexGeometry(CropVertexGeometry&&) noexcept = delete;

  CropVertexGeometry& operator=(const CropVertexGeometry&) = delete;
  CropVertexGeometry& operator=(CropVertexGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_VertexGeom_Key = "vertex_geom";
  static inline constexpr StringLiteral k_CroppedGeom_Key = "cropped_geom";
  static inline constexpr StringLiteral k_MinPos_Key = "min_pos";
  static inline constexpr StringLiteral k_MaxPos_Key = "max_pos";
  static inline constexpr StringLiteral k_TargetArrayPaths_Key = "target_arrays";
  static inline constexpr StringLiteral k_CroppedGroupName_Key = "cropped_group";

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
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CropVertexGeometry, "f28cbf07-f15a-53ca-8c7f-b41a11dae6cc");
