#pragma once

#include "Core/Core_export.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"

class CORE_EXPORT CorePlugin : public complex::AbstractPlugin
{
public:
  CorePlugin();
  ~CorePlugin() override;

  CorePlugin(const CorePlugin&) = delete;
  CorePlugin(CorePlugin&&) = delete;

  CorePlugin& operator=(const CorePlugin&) = delete;
  CorePlugin& operator=(CorePlugin&&) = delete;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return std::map<complex::Uuid, complex::Uuid>
   */
  std::map<complex::Uuid, complex::Uuid> getSimplToComplexMap() const override;
};
