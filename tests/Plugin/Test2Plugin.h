#pragma once

#include <vector>

#include "Complex/Plugin/AbstractPlugin.h"

#include <test2plugin_export.h>

class TEST2PLUGIN_EXPORT Test2Plugin : public Complex::AbstractPlugin
{
public:
  static const IdType ID;

  Test2Plugin();
  ~Test2Plugin() override;
};

extern "C" {
TEST2PLUGIN_EXPORT Complex::AbstractPlugin* initPlugin();
}
