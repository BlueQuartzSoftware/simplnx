#pragma once

#include "Complex/Filtering/AbstractFilter.h"

#include <test2plugin_export.h>

class TEST2PLUGIN_EXPORT Test2Filter : public complex::AbstractFilter
{
public:
  Test2Filter();
  virtual ~Test2Filter();

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

protected:
  void preflightImpl(complex::FilterDataOps& data, const complex::Arguments& args) override;
  void executeImpl(complex::FilterDataOps& data, const complex::Arguments& args) override;
};
