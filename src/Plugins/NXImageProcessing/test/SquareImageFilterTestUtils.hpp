#pragma once

#include <filesystem>
#include <functional>
#include <memory>

namespace nx::core
{
class IDataArray;
class IDataStoreFormatResolver;
} // namespace nx::core

namespace ip_test
{
/** @brief Checks an array's backing and, when supplied, its reopened source file. */
using SquareStoreCheckType = std::function<void(const nx::core::IDataArray&, const std::filesystem::path&)>;

/**
 * @brief Verifies exact integer pixels through DREAM3D, raw MetaImage, and connected components.
 * @param checkStore Checks each array's expected storage and, for a reopened input, its source file.
 * @param runWriter Executes the writer under the calling suite's algorithm-dispatch witness.
 * @param resolver Selects storage when reopening the saved DREAM3D file.
 */
void checkSquareConsumers(const SquareStoreCheckType& checkStore, const std::function<void(const std::function<void()>&)>& runWriter,
                          std::shared_ptr<const nx::core::IDataStoreFormatResolver> resolver);
} // namespace ip_test
