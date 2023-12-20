#pragma once

#include "TestTwo/TestTwo_export.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

class TESTTWO_EXPORT TestTwoPlugin : public nx::core::AbstractPlugin
{
public:
  TestTwoPlugin();
  ~TestTwoPlugin() override;

  TestTwoPlugin(const TestTwoPlugin&) = delete;
  TestTwoPlugin(TestTwoPlugin&&) = delete;

  TestTwoPlugin& operator=(const TestTwoPlugin&) = delete;
  TestTwoPlugin& operator=(TestTwoPlugin&&) = delete;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their simplnx counterpart
   * @return SIMPLMapType
   */
  SIMPLMapType getSimplToSimplnxMap() const override;
};
