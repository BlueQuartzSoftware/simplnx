#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"

#include "test/testplugin_export.hpp"

class TESTPLUGIN_EXPORT TestPlugin : public complex::AbstractPlugin
{
public:
  static const IdType ID;

  TestPlugin();
  ~TestPlugin() override;
};

extern "C" {
TESTPLUGIN_EXPORT complex::AbstractPlugin* initPlugin();
}
