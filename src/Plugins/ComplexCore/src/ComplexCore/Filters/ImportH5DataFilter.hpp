#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Parameters.hpp"

namespace complex
{
/**
 * @class ImportH5DataFilter
 * @brief The ImportH5DataFilter is an IFilter class designed to import the
 * DataStructure from a target HDF5 file.
 */
class COMPLEXCORE_EXPORT ImportH5DataFilter : public IFilter
{
public:
  ImportH5DataFilter() = default;
  ~ImportH5DataFilter() noexcept override = default;

  ImportH5DataFilter(const ImportH5DataFilter&) = delete;
  ImportH5DataFilter(ImportH5DataFilter&&) noexcept = delete;

  ImportH5DataFilter& operator=(const ImportH5DataFilter&) = delete;
  ImportH5DataFilter& operator=(ImportH5DataFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImportFileData = "Import_File_Data";

  /**
   * @brief Returns the name of the filter class.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the ImportH5DataFilter class's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns a collection of the filter's parameters (i.e. its inputs)
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter as a std::unique_ptr.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Classes that implement IFilter must provide this function for preflight.
   * Runs after the filter runs the checks in its parameters.
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  Result<OutputActions> preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief Classes that implement IFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ImportH5DataFilter, "0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
