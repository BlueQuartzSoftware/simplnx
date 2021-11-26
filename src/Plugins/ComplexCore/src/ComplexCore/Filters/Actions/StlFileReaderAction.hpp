#pragma once

#include "complex/Filter/Output.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace complex
{
/**
 * @brief Action to check if an STL file is ASCII or Binary
 */
class StlFileReaderAction : public IDataAction
{
public:
  StlFileReaderAction() = default;
  StlFileReaderAction(const fs::path& stlFilePath);

  ~StlFileReaderAction() noexcept override;

  StlFileReaderAction(const StlFileReaderAction&) = delete;
  StlFileReaderAction(StlFileReaderAction&&) noexcept = delete;
  StlFileReaderAction& operator=(const StlFileReaderAction&) = delete;
  StlFileReaderAction& operator=(StlFileReaderAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @param mode
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;


  /**
   * @brief Returns the path used to insert the DataObject into the DataStructure.
   * @return DataPath
   */
  fs::path path() const;

private:
  std::shared_ptr<DataObject> m_ImportData;
  fs::path m_Path;
};
} // namespace complex
