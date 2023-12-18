#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"

class COMPLEXCORE_EXPORT ComplexCorePlugin : public complex::AbstractPlugin
{
public:
  ComplexCorePlugin();
  ~ComplexCorePlugin() override;

  ComplexCorePlugin(const ComplexCorePlugin&) = delete;
  ComplexCorePlugin(ComplexCorePlugin&&) = delete;

  ComplexCorePlugin& operator=(const ComplexCorePlugin&) = delete;
  ComplexCorePlugin& operator=(ComplexCorePlugin&&) = delete;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return SIMPLMapType
   */
  SIMPLMapType getSimplToComplexMap() const override;
};
