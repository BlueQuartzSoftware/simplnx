#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class RenameDataObject
 * @brief RenameDataObject class is used to rename any DataObject.
 */
class COMPLEXCORE_EXPORT RenameDataObject : public IFilter
{
public:
  RenameDataObject() = default;
  ~RenameDataObject() noexcept override = default;

  RenameDataObject(const RenameDataObject&) = delete;
  RenameDataObject(RenameDataObject&&) noexcept = delete;

  RenameDataObject& operator=(const RenameDataObject&) = delete;
  RenameDataObject& operator=(RenameDataObject&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataObject_Key = "data_object";
  static inline constexpr StringLiteral k_NewName_Key = "new_name";

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
   * @param shouldCancel
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, RenameDataObject, "d53c808f-004d-5fac-b125-0fffc8cc78d6");
