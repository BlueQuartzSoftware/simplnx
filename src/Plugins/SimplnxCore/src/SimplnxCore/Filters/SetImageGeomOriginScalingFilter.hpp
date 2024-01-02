#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace nx::core
{
class SIMPLNXCORE_EXPORT SetImageGeomOriginScalingFilter : public IFilter
{
public:
  SetImageGeomOriginScalingFilter() = default;
  ~SetImageGeomOriginScalingFilter() noexcept override = default;

  SetImageGeomOriginScalingFilter(const SetImageGeomOriginScalingFilter&) = delete;
  SetImageGeomOriginScalingFilter(SetImageGeomOriginScalingFilter&&) noexcept = delete;

  SetImageGeomOriginScalingFilter& operator=(const SetImageGeomOriginScalingFilter&) = delete;
  SetImageGeomOriginScalingFilter& operator=(SetImageGeomOriginScalingFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImageGeomPath_Key = "image_geom";
  static inline constexpr StringLiteral k_ChangeOrigin_Key = "change_origin";
  static inline constexpr StringLiteral k_CenterOrigin_Key = "center_origin";
  static inline constexpr StringLiteral k_ChangeResolution_Key = "change_resolution";
  static inline constexpr StringLiteral k_Origin_Key = "origin";
  static inline constexpr StringLiteral k_Spacing_Key = "spacing";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SetImageGeomOriginScalingFilter, "057bc7fd-c84a-4902-9397-87e51b1b1fe0");
