#pragma once

#include "Complex/Filtering/AbstractFilter.h"

#include <testplugin_export.h>

class TESTPLUGIN_EXPORT TestFilter : public Complex::AbstractFilter
{
public:
  TestFilter();
  virtual ~TestFilter();

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

protected:
  void preflightImpl(Complex::FilterDataOps& data, const Complex::Arguments& args) override;
  void executeImpl(Complex::FilterDataOps& data, const Complex::Arguments& args) override;
};
