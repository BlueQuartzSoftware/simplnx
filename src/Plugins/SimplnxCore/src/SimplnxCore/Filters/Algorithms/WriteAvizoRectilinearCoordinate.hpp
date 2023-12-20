#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/AvizoWriter.hpp"

namespace nx::core
{
/**
 * @class WriteAvizoRectilinearCoordinate
 * @brief This filter writes out a native Avizo Rectilinear Coordinate data file.
 */

class SIMPLNXCORE_EXPORT WriteAvizoRectilinearCoordinate : public AvizoWriter
{
public:
  WriteAvizoRectilinearCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~WriteAvizoRectilinearCoordinate() noexcept override;

  WriteAvizoRectilinearCoordinate(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate(WriteAvizoRectilinearCoordinate&&) noexcept = delete;
  WriteAvizoRectilinearCoordinate& operator=(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate& operator=(WriteAvizoRectilinearCoordinate&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> generateHeader(FILE* outputFile) const override;
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
