#pragma once

#include "Complex/Filtering/AbstractFilter.h"

#include <test2plugin_export.h>

class TEST2PLUGIN_EXPORT Test2Filter : public Complex::AbstractFilter
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
  void preflightImpl(Complex::FilterDataOps& data, const Complex::Arguments& args) override;
  void executeImpl(Complex::FilterDataOps& data, const Complex::Arguments& args) override;
};
