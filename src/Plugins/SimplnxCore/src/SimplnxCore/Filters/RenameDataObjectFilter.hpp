#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RenameDataObjectFilter
 * @brief RenameDataObjectFilter class is used to rename any DataObject.
 */
class SIMPLNXCORE_EXPORT RenameDataObjectFilter : public IFilter
{
public:
  RenameDataObjectFilter() = default;
  ~RenameDataObjectFilter() noexcept override = default;

  RenameDataObjectFilter(const RenameDataObjectFilter&) = delete;
  RenameDataObjectFilter(RenameDataObjectFilter&&) noexcept = delete;

  RenameDataObjectFilter& operator=(const RenameDataObjectFilter&) = delete;
  RenameDataObjectFilter& operator=(RenameDataObjectFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SourceDataObjectPath_Key = "source_data_object_path";
  static inline constexpr StringLiteral k_NewName_Key = "new_name";

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
   * @param shouldCancel
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RenameDataObjectFilter, "911a3aa9-d3c2-4f66-9451-8861c4b726d5");
