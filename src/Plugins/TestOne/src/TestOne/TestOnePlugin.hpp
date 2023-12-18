#pragma once

#include "TestOne/TestOne_export.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"

class TESTONE_EXPORT TestOnePlugin : public complex::AbstractPlugin
{
public:
  TestOnePlugin();
  ~TestOnePlugin() override;

  TestOnePlugin(const TestOnePlugin&) = delete;
  TestOnePlugin(TestOnePlugin&&) = delete;

  TestOnePlugin& operator=(const TestOnePlugin&) = delete;
  TestOnePlugin& operator=(TestOnePlugin&&) = delete;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return SIMPLMapType
   */
  SIMPLMapType getSimplToComplexMap() const override;
};
