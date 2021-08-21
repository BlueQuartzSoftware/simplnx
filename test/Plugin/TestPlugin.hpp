#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"

#include "test/testplugin_export.hpp"

class TESTPLUGIN_EXPORT TestPlugin : public complex::AbstractPlugin
{
public:
  TestPlugin();
  ~TestPlugin() override;

  TestPlugin(const TestPlugin&) = delete;
  TestPlugin(TestPlugin&&) = delete;

  TestPlugin& operator=(const TestPlugin&) = delete;
  TestPlugin& operator=(TestPlugin&&) = delete;
};
