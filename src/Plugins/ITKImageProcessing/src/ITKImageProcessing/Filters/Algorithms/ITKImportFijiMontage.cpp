#include "ITKImportFijiMontage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/ReadImageUtils.hpp"
#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_SimplnxCorePluginId);

template <bool GenerateCache = true>
class IOHandler
{
public:
  IOHandler(ITKImportFijiMontage* filter, DataStructure& dataStructure, const ITKImportFijiMontageInputValues* inputs)
  : m_Filter(filter)
  , m_DataStructure(dataStructure)
  , m_InputValues(inputs)
  , m_Allocate(inputs->allocate)
  , m_Cache(m_Filter->getCache())
  {
  }
  ~IOHandler() noexcept = default;

  IOHandler(const IOHandler&) = delete;
  IOHandler(IOHandler&&) noexcept = delete;
  IOHandler& operator=(const IOHandler&) = delete;
  IOHandler& operator=(IOHandler&&) noexcept = delete;

  [[nodiscard]] Result<> operator()()
  {
    m_InStream = std::ifstream(m_InputValues->inputFilePath, std::ios_base::binary);
    if(!m_InStream.is_open())
    {
      return MakeErrorResult(-2013, fmt::format("Unable to open the provided file to read at path : {}", m_InputValues->inputFilePath.string()));
    }

    if(!m_Allocate)
    {
      fillCache();
    }
    else
    {
      return readImages();
    }

    return {};
  }

private:
  // member variables
  ITKImportFijiMontage* m_Filter;
  DataStructure& m_DataStructure;
  std::ifstream m_InStream;
  const bool m_Allocate;
  FijiCache& m_Cache;
  const ITKImportFijiMontageInputValues* m_InputValues;

  // -----------------------------------------------------------------------------
  void parseConfigFile()
  {
    bool dimFound = false;
    bool dataFound = false;

    std::string line;
    while(std::getline(m_InStream, line))
    {
      line = StringUtilities::trimmed(line);

      if(StringUtilities::starts_with(line, "dim =")) // found the dimensions
      {
        dimFound = true;
        // Should check that the value = 2
      }
      if(StringUtilities::starts_with(line, "# Define the image coordinates"))
      {
        // Found the start of the data
        dataFound = true;
        break;
      }
      if(StringUtilities::starts_with(line, "#")) // comment line
      {
        continue;
      }
    }

    if(!dimFound || !dataFound)
    {
      return;
    }

    // Example line to be parsed: slice_12.tif; ; (471.2965233276666, -0.522608066434236)
    while(std::getline(m_InStream, line))
    {
      line = StringUtilities::trimmed(line);
      if(line.empty())
      {
        continue;
      }
      std::vector<std::string> tokens = StringUtilities::split(line, ';');
      if(tokens.size() != 3)
      {
        continue;
      }
      BoundsType bound;
      bound.Filepath = m_InputValues->inputFilePath.parent_path() / tokens[0];

      std::string coords = StringUtilities::trimmed(tokens[2]);
      coords = StringUtilities::replace(coords, "(", "");
      coords = StringUtilities::replace(coords, ")", "");
      tokens = StringUtilities::split(coords, ',');
      if(tokens.size() != 2)
      {
        continue;
      }
      float32 x = std::stof(tokens[0]);
      float32 y = std::stof(tokens[1]);
      bound.Origin = Point3Df(x, y, 0.0f);
      m_Cache.bounds.push_back(bound);
    }
  }

  // -----------------------------------------------------------------------------
  void fillCache()
  {
    parseConfigFile();

    Point3Df minCoord = {std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max()};

    // Get the meta information from disk for each image
    for(auto& bound : m_Cache.bounds)
    {
      std::stringstream dcNameStream;
      dcNameStream << m_InputValues->imagePrefix << bound.Filepath.stem().string();
      bound.ImageName = dcNameStream.str();

      {
        minCoord[0] = std::min(bound.Origin[0], minCoord[0]);
        minCoord[1] = std::min(bound.Origin[1], minCoord[1]);
        minCoord[2] = 0.0f;
      }
    }

    Point3Df overrideOrigin;
    // Now adjust the origin/spacing if needed
    if(m_InputValues->changeOrigin)
    {
      overrideOrigin = m_InputValues->origin;
      Point3Df delta = {minCoord[0] - overrideOrigin[0], minCoord[1] - overrideOrigin[1], minCoord[2] - overrideOrigin[2]};
      for(auto& bound : m_Cache.bounds)
      {
        std::transform(bound.Origin.begin(), bound.Origin.end(), delta.begin(), bound.Origin.begin(), std::minus<>());
      }
    }

    std::stringstream ss;
    ss << "\n"
       << "Imported Image Count: " << m_Cache.bounds.size();
    m_Cache.montageInformation = ss.str();
  }

  // -----------------------------------------------------------------------------
  Result<> readImages()
  {
    Result<> outputResult = {};

    auto* filterListPtr = Application::Instance()->getFilterList();
    // auto imageImportFilter = ITKImageReader();

    for(const auto& bound : m_Cache.bounds)
    {
      m_Filter->sendUpdate(("Importing " + bound.Filepath.filename().string()));

      DataPath imageDataPath = {};
      if(m_InputValues->parentDataGroup)
      {
        imageDataPath = DataPath({m_InputValues->DataGroupName, bound.ImageName, m_InputValues->cellAMName, m_InputValues->imageDataArrayName});
      }
      else
      {
        imageDataPath = DataPath({bound.ImageName, m_InputValues->cellAMName, m_InputValues->imageDataArrayName});
      }

      // Ensure that we are dealing with in-core memory ONLY
      const IDataArray* inputArrayPtr = m_DataStructure.getDataAs<IDataArray>(imageDataPath);
      if(inputArrayPtr->getDataFormat() != "")
      {
        return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageDataPath.toString()));
      }

      // Set the Correct Origin, Spacing and Units for the Image Geometry
      auto* image = m_DataStructure.getDataAs<ImageGeom>(imageDataPath.getParent().getParent());
      image->setUnits(m_InputValues->lengthUnit);
      image->setOrigin(bound.Origin);
      image->setSpacing(FloatVec3(1.0f, 1.0f, 1.0f));

      // Use ITKUtils to read the image into the DataStructure
      Result<> imageReaderResult = cxItkImageReader::ReadImageExecute<cxItkImageReader::ReadImageIntoArrayFunctor>(bound.Filepath.string(), m_DataStructure, imageDataPath, bound.Filepath.string());
      if(imageReaderResult.invalid())
      {
        for(const auto& error : imageReaderResult.errors())
        {
          m_Filter->sendUpdate(fmt::format("|-- Error Reading Image: Code ({}) - {}", error.code, error.message));
        }
        continue; // Let us try to continue to the next image if we encountered a problem reading this image
      }

      // Check if we need to convert to grayscale
      if(m_InputValues->convertToGrayScale)
      {
        if(!filterListPtr->containsPlugin(k_SimplnxCorePluginId))
        {
          return MakeErrorResult(-18542, "SimplnxCore was not instantiated in this instance, so color to grayscale is not a valid option.");
        }
        auto grayScaleFilter = filterListPtr->createFilter(k_ColorToGrayScaleFilterHandle);
        if(nullptr == grayScaleFilter.get())
        {
          continue;
        }

        if(m_DataStructure.getDataRefAs<IDataArray>(imageDataPath).getDataType() != DataType::uint8)
        {
          outputResult.warnings().emplace_back(
              Warning{-74320, fmt::format("The array ({}) is not a UIntArray, so it will not be converted to grayscale. Continuing...", imageDataPath.getTargetName())});
          continue;
        }

        // This same filter was used to preflight so as long as nothing changes on disk this really should work....
        Arguments colorToGrayscaleArgs;
        colorToGrayscaleArgs.insertOrAssign("conversion_algorithm", std::make_any<ChoicesParameter::ValueType>(0));
        colorToGrayscaleArgs.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(m_InputValues->colorWeights));
        colorToGrayscaleArgs.insertOrAssign("input_data_array_vector", std::make_any<std::vector<DataPath>>(std::vector<DataPath>{imageDataPath}));
        colorToGrayscaleArgs.insertOrAssign("output_array_prefix", std::make_any<std::string>("gray"));

        // Run grayscale filter and process results and messages
        auto result = grayScaleFilter->execute(m_DataStructure, colorToGrayscaleArgs).result;
        if(result.invalid())
        {
          return result;
        }

        // deletion of non-grayscale array
        DataObject::IdType id;
        { // scoped for safety since this reference will be nonexistent in a moment
          auto& oldArray = m_DataStructure.getDataRefAs<IDataArray>(imageDataPath);
          id = oldArray.getId();
        }
        m_DataStructure.removeData(id);

        // rename grayscale array to reflect original
        {
          auto& gray = m_DataStructure.getDataRefAs<IDataArray>(imageDataPath.getParent().createChildPath("gray" + imageDataPath.getTargetName()));
          if(!gray.canRename(imageDataPath.getTargetName()))
          {
            return MakeErrorResult(-18543, fmt::format("Unable to rename the grayscale array to {}", imageDataPath.getTargetName()));
          }
          gray.rename(imageDataPath.getTargetName());
        }
      }
    }

    return outputResult;
  }
};
} // namespace

// -----------------------------------------------------------------------------
ITKImportFijiMontage::ITKImportFijiMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportFijiMontageInputValues* inputValues,
                                           FijiCache& cache)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Cache(cache)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ITKImportFijiMontage::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
FijiCache& ITKImportFijiMontage::getCache()
{
  return m_Cache;
}

// -----------------------------------------------------------------------------
void ITKImportFijiMontage::sendUpdate(const std::string& message)
{
  m_MessageHandler({IFilter::Message::Type::Info, message});
}

// -----------------------------------------------------------------------------
Result<> ITKImportFijiMontage::operator()()
{
  /*
   * IF allocate is false DO NOT TOUCH DATA STRUCTURE IN ANY WAY
   *
   * If allocate is false then we are here in preflight meaning we
   * DO NOT have a working DataStructure and that the DataPaths that
   * have been passed in within inputValues are not valid as they have
   * not been created.
   */
  return IOHandler(this, m_DataStructure, m_InputValues)();
}
