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
  static inline constexpr StringLiteral k_MinX_Key = "min_x";
  static inline constexpr StringLiteral k_MinY_Key = "min_y";
  static inline constexpr StringLiteral k_MinZ_Key = "min_z";
  static inline constexpr StringLiteral k_MaxX_Key = "max_x";
  static inline constexpr StringLiteral k_MaxY_Key = "max_y";
  static inline constexpr StringLiteral k_MaxZ_Key = "max_z";

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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CropVertexGeometry, "f28cbf07-f15a-53ca-8c7f-b41a11dae6cc");
