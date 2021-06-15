#pragma once

#include <vector>

#include "Complex/Plugin/AbstractPlugin.h"

#include <testplugin_export.h>

class TESTPLUGIN_EXPORT TestPlugin : public Complex::AbstractPlugin
{
public:
  static const IdType ID;

  TestPlugin();
  ~TestPlugin() override;
};

extern "C"
{
TESTPLUGIN_EXPORT Complex::AbstractPlugin* initPlugin();
}
