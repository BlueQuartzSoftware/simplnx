#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/AvizoWriter.hpp"

namespace nx::core
{
/**
 * @class WriteAvizoUniformCoordinate
 * @brief This filter writes out a native Avizo Uniform Coordinate data file.
 */

class SIMPLNXCORE_EXPORT WriteAvizoUniformCoordinate : public AvizoWriter
{
public:
  WriteAvizoUniformCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~WriteAvizoUniformCoordinate() noexcept override;

  WriteAvizoUniformCoordinate(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate(WriteAvizoUniformCoordinate&&) noexcept = delete;
  WriteAvizoUniformCoordinate& operator=(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate& operator=(WriteAvizoUniformCoordinate&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> generateHeader(FILE* outputFile) const override;
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
