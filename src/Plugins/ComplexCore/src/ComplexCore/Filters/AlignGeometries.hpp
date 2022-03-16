#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT AlignGeometries : public IFilter
{
public:
  AlignGeometries() = default;
  ~AlignGeometries() noexcept override = default;

  AlignGeometries(const AlignGeometries&) = delete;
  AlignGeometries(AlignGeometries&&) noexcept = delete;

  AlignGeometries& operator=(const AlignGeometries&) = delete;
  AlignGeometries& operator=(AlignGeometries&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MovingGeometry_Key = "moving_geometry";
  static inline constexpr StringLiteral k_TargetGeometry_Key = "target_geometry";
  static inline constexpr StringLiteral k_AlignmentType_Key = "alignment_type";

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

COMPLEX_DEF_FILTER_TRAITS(complex, AlignGeometries, "ce1ee404-0336-536c-8aad-f9641c9458be");
