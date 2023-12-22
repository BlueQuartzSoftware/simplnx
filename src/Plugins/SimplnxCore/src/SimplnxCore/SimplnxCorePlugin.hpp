#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

class SIMPLNXCORE_EXPORT SimplnxCorePlugin : public nx::core::AbstractPlugin
{
public:
  SimplnxCorePlugin();
  ~SimplnxCorePlugin() override;

  SimplnxCorePlugin(const SimplnxCorePlugin&) = delete;
  SimplnxCorePlugin(SimplnxCorePlugin&&) = delete;

  SimplnxCorePlugin& operator=(const SimplnxCorePlugin&) = delete;
  SimplnxCorePlugin& operator=(SimplnxCorePlugin&&) = delete;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their simplnx counterpart
   * @return SIMPLMapType
   */
  SIMPLMapType getSimplToSimplnxMap() const override;
};
