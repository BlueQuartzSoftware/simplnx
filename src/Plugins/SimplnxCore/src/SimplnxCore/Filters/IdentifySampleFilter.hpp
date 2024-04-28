#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT IdentifySampleFilter : public IFilter
{
public:
  IdentifySampleFilter() = default;
  ~IdentifySampleFilter() noexcept override = default;

  IdentifySampleFilter(const IdentifySampleFilter&) = delete;
  IdentifySampleFilter(IdentifySampleFilter&&) noexcept = delete;

  IdentifySampleFilter& operator=(const IdentifySampleFilter&) = delete;
  IdentifySampleFilter& operator=(IdentifySampleFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FillHoles_Key = "fill_holes";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return
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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, IdentifySampleFilter, "94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
