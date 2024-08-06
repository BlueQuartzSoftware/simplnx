#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace nx::core
{
struct ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontageInputValues
{
  bool allocate = false;
  bool changeOrigin = false;
  bool convertToGrayScale = false;
  bool parentDataGroup = false;
  bool changeDataType = false;
  DataType destType = DataType::uint8;
  fs::path inputFilePath = {};
  IGeometry::LengthUnit lengthUnit = IGeometry::LengthUnit::Micrometer;
  std::vector<float32> origin = {};
  std::vector<float32> colorWeights = {};
  std::string DataGroupName = "";
  std::string imagePrefix = "";
  std::string cellAMName = "";
  std::string imageDataArrayName = "";
};

struct ITKIMAGEPROCESSING_EXPORT BoundsType
{
  fs::path Filepath;
  FloatVec3 Origin;
  std::string ImageName;
};

struct ITKIMAGEPROCESSING_EXPORT FijiCache
{
  fs::path inputFile;
  std::vector<BoundsType> bounds;
  std::string montageInformation;
  fs::file_time_type timeStamp;
  ITKImportFijiMontageInputValues inputValues = {};

  void flush()
  {
    inputFile.clear();
    bounds.clear();
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

    // no need to check input file as it will be done before this run anyway

    // no need to check the length unit because it doesn't affect anything till execute

    if(inputValues.imagePrefix != newVals.imagePrefix)
    {
      return true;
    }

    // check loops last to avoid running them unless absolutely necessary
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
  FijiCache& m_Cache;
};
} // namespace nx::core
