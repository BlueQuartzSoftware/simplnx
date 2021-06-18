#pragma once

#include "Complex/Filtering/AbstractFilter.h"

#include <testplugin_export.h>

class TESTPLUGIN_EXPORT TestFilter : public complex::AbstractFilter
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
  void preflightImpl(complex::FilterDataOps& data, const complex::Arguments& args) override;
  void executeImpl(complex::FilterDataOps& data, const complex::Arguments& args) override;
};
