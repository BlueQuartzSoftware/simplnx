#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class IDataStoreWritePreparation
 * @brief Optional store capability for completing first-write setup before an algorithm reserves its working memory.
 */
class SIMPLNX_EXPORT IDataStoreWritePreparation
{
public:
  virtual ~IDataStoreWritePreparation() noexcept;

  /**
   * @brief Prepares the existing values and shape for writing and releases temporary setup memory before returning.
   * @details A store may switch to writable backing storage. Calling this on an already-writable store succeeds without repeating setup.
   * @return Preparation diagnostics. The caller must stop before writing if preparation fails.
   */
  [[nodiscard]] virtual Result<> prepareForWrite() = 0;

  IDataStoreWritePreparation(const IDataStoreWritePreparation&) = delete;
  IDataStoreWritePreparation(IDataStoreWritePreparation&&) = delete;
  IDataStoreWritePreparation& operator=(const IDataStoreWritePreparation&) = delete;
  IDataStoreWritePreparation& operator=(IDataStoreWritePreparation&&) = delete;

protected:
  IDataStoreWritePreparation() = default;
};
} // namespace nx::core
