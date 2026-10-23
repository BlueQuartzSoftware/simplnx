#pragma once

#include "simplnx/Core/Preferences.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>

namespace ip_bench
{
/**
 * @brief Restores exact explicit storage preferences after a benchmark scope.
 *
 * The benchmark tools do not link UnitTestCommon. This guard changes only the
 * in-memory Preferences object and does not save a preferences file.
 */
class BenchmarkPreferenceSnapshot
{
public:
  /**
   * @brief Validates and applies a temporary storage mode and threshold.
   * @param preferences Preference object to change and restore.
   * @param mode Temporary storage mode.
   * @param temporaryThreshold Signed temporary threshold in bytes.
   */
  BenchmarkPreferenceSnapshot(nx::core::Preferences& preferences, nx::core::DataStorageMode mode, nx::core::int64 temporaryThreshold = 0)
  : m_Preferences(preferences)
  , m_HadMode(preferences.contains(nx::core::Preferences::k_DataStorageMode_Key))
  , m_OriginalMode(preferences.value(nx::core::Preferences::k_DataStorageMode_Key))
  , m_HadThreshold(preferences.contains(nx::core::Preferences::k_LargeDataSize_Key))
  , m_OriginalThreshold(preferences.value(nx::core::Preferences::k_LargeDataSize_Key))
  {
    const nlohmann::json proposed(temporaryThreshold);
    const auto validation = nx::core::Preferences::ValidateOocSizeValue(nx::core::Preferences::k_LargeDataSize_Key, proposed);
    if(validation.invalid())
    {
      throw std::invalid_argument(validation.errors().front().message);
    }
    m_Preferences.setDataStorageMode(mode);
    m_Preferences.setValue(nx::core::Preferences::k_LargeDataSize_Key, proposed);
  }

  ~BenchmarkPreferenceSnapshot()
  {
    if(m_HadThreshold)
    {
      m_Preferences.setValue(nx::core::Preferences::k_LargeDataSize_Key, m_OriginalThreshold);
    }
    else
    {
      m_Preferences.removeValue(nx::core::Preferences::k_LargeDataSize_Key);
    }
    if(m_HadMode)
    {
      m_Preferences.setValue(nx::core::Preferences::k_DataStorageMode_Key, m_OriginalMode);
    }
    else
    {
      m_Preferences.removeValue(nx::core::Preferences::k_DataStorageMode_Key);
    }
  }

  BenchmarkPreferenceSnapshot(const BenchmarkPreferenceSnapshot&) = delete;
  BenchmarkPreferenceSnapshot(BenchmarkPreferenceSnapshot&&) = delete;
  BenchmarkPreferenceSnapshot& operator=(const BenchmarkPreferenceSnapshot&) = delete;
  BenchmarkPreferenceSnapshot& operator=(BenchmarkPreferenceSnapshot&&) = delete;

private:
  nx::core::Preferences& m_Preferences;
  bool m_HadMode = false;
  nlohmann::json m_OriginalMode;
  bool m_HadThreshold = false;
  nlohmann::json m_OriginalThreshold;
};
} // namespace ip_bench
