#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace complex
{
struct ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontageInputValues
{
  bool allocate = false;
  bool changeOrigin = false;
  bool convertToGrayScale = false;
  fs::path inputFilePath = {};
  IGeometry::LengthUnit lengthUnit = IGeometry::LengthUnit::Micrometer;
  std::vector<int32> columnMontageLimits = {};
  std::vector<int32> rowMontageLimits = {};
  std::vector<float32> origin = {};
  std::vector<float32> colorWeights = {};
  std::string montageName = "";
  std::string imagePrefix = "";
  std::string cellAMName = "";
  std::string imageDataArrayName = "";
};

struct ITKIMAGEPROCESSING_EXPORT BoundsType
{
  fs::path Filepath;
  FloatVec3 Origin;
  FloatVec3 Spacing;
  usize Row;
  usize Col;
  DataPath ImageDataProxy;
};

struct ITKIMAGEPROCESSING_EXPORT FijiCache
{
  fs::path inputFile;
  std::vector<BoundsType> bounds;
  usize maxCol = 0;
  usize maxRow = 0;
  std::string montageInformation;
  fs::file_time_type timeStamp;
  ITKImportFijiMontageInputValues inputValues = {};

  void flush()
  {
    inputFile.clear();
    bounds.clear();
    maxCol = 0;
    maxRow = 0;
    montageInformation = "";
    timeStamp = fs::file_time_type();
    inputValues = {};
  }

  bool valuesChanged(const ITKImportFijiMontageInputValues& newVals)
  {
    if(inputValues.changeOrigin != newVals.changeOrigin)
    {
      return true;
    }

    if(inputValues.convertToGrayScale != newVals.convertToGrayScale)
    {
      return true;
    }

    // no need to check input file as it will be done before this run anyway

    // no need to check the length unit because it doesn't affect anything till execute

    if(inputValues.montageName != newVals.montageName)
    {
      return true;
    }

    if(inputValues.imagePrefix != newVals.imagePrefix)
    {
      return true;
    }

    if(inputValues.cellAMName != newVals.cellAMName)
    {
      return true;
    }

    if(inputValues.imageDataArrayName != newVals.imageDataArrayName)
    {
      return true;
    }

    if(inputValues.columnMontageLimits.size() != newVals.columnMontageLimits.size())
    {
      return true;
    }

    if(inputValues.rowMontageLimits.size() != newVals.rowMontageLimits.size())
    {
      return true;
    }

    // check loops last to avoid running them unless absolutely necessary
    for(usize i = 0; i < inputValues.rowMontageLimits.size(); i++)
    {
      if(inputValues.rowMontageLimits[i] != newVals.rowMontageLimits[i])
      {
        return true;
      }
    }

    for(usize i = 0; i < inputValues.columnMontageLimits.size(); i++)
    {
      if(inputValues.columnMontageLimits[i] != newVals.columnMontageLimits[i])
      {
        return true;
      }
    }

    if(newVals.changeOrigin)
    {
      for(usize i = 0; i < inputValues.origin.size(); i++)
      {
        if(inputValues.origin[i] != newVals.origin[i])
        {
          return true;
        }
      }
    }

    if(newVals.convertToGrayScale)
    {
      for(usize i = 0; i < inputValues.colorWeights.size(); i++)
      {
        if(inputValues.colorWeights[i] != newVals.colorWeights[i])
        {
          return true;
        }
      }
    }

    return false;
  }
};

/**
 * @class ITKImportImageStack
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontage
{
public:
  ITKImportFijiMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportFijiMontageInputValues* inputValues, FijiCache& cache);
  ~ITKImportFijiMontage() noexcept = default;

  ITKImportFijiMontage(const ITKImportFijiMontage&) = delete;
  ITKImportFijiMontage(ITKImportFijiMontage&&) noexcept = delete;
  ITKImportFijiMontage& operator=(const ITKImportFijiMontage&) = delete;
  ITKImportFijiMontage& operator=(ITKImportFijiMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  FijiCache& getCache();

  void sendUpdate(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const ITKImportFijiMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  FijiCache m_Cache = {};
};
} // namespace complex
