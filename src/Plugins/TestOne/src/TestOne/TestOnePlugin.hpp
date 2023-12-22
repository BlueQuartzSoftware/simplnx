#pragma once

#include "TestOne/TestOne_export.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

class TESTONE_EXPORT TestOnePlugin : public nx::core::AbstractPlugin
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
   * their simplnx counterpart
   * @return SIMPLMapType
   */
  SIMPLMapType getSimplToSimplnxMap() const override;
};
