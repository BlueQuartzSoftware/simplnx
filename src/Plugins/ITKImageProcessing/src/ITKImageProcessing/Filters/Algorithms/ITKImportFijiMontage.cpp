#include "ITKImportFijiMontage.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Common/Array.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_ComplexCorePluginId);

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
  ~IOHandler() = default;

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
      FillCache();
    }
    else
    {
      return ReadImages();
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
  void ParseConfigFile()
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
      std::stringstream path;
      path << m_InputValues->inputFilePath.parent_path().string() << fs::path::preferred_separator << tokens[0];
      bound.Filepath = fs::path(path.str());

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
  void FillCache()
  {
    // This next function will set the FileName (Partial), Row, Col for each "bound" object
    ParseConfigFile();

    Point3Df minCoord = {std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max()};

    // Get the meta information from disk for each image
    for(auto& bound : m_Cache.bounds)
    {
      std::stringstream dcNameStream;
      dcNameStream << m_InputValues->imagePrefix << bound.Filepath.filename().string().substr(0, bound.Filepath.filename().string().find(bound.Filepath.extension().string()));
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
  Result<> ReadImages()
  {
    Result<> outputResult = {};

    auto* filterListPtr = Application::Instance()->getFilterList();
    auto imageImportFilter = filterListPtr->createFilter(FilterTraits<ITKImageReader>::uuid);
    if(nullptr == imageImportFilter.get())
    {
      return MakeErrorResult(-18544, "Unable to create ITKImageReader filter");
    }

    for(const auto& bound : m_Cache.bounds)
    {
      m_Filter->sendUpdate(("Importing " + bound.Filepath.filename().string()));

      DataPath imageDataProxy = {};
      if(m_InputValues->parentDataGroup)
      {
        imageDataProxy = DataPath({m_InputValues->DataGroupName, bound.ImageName, m_InputValues->cellAMName, m_InputValues->imageDataArrayName});
      }
      else
      {
        imageDataProxy = DataPath({bound.ImageName, bound.ImageName, m_InputValues->cellAMName, m_InputValues->imageDataArrayName});
      }

      // Instantiate the Image Import Filter to actually read the image into a data array
      {
        // execute image import filter
        // This same filter was used to preflight so as long as nothing changes on disk this really should work....
        Arguments imageImportArgs;
        imageImportArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(bound.Filepath));
        imageImportArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageDataProxy.getParent().getParent()));
        imageImportArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<std::string>(imageDataProxy.getParent().getTargetName()));
        imageImportArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(imageDataProxy));

        auto result = imageImportFilter->execute(m_DataStructure, imageImportArgs).result;
        if(result.invalid())
        {
          outputResult = MergeResults(outputResult, result);
          continue;
        }
      }

      auto* image = m_DataStructure.getDataAs<ImageGeom>(imageDataProxy.getParent().getParent());
      image->setUnits(m_InputValues->lengthUnit);
      image->setOrigin(bound.Origin);
      image->setSpacing(FloatVec3(1.0f, 1.0f, 1.0f));

      // Now transfer the image data from the actual image data read from disk into our existing Attribute Matrix
      if(m_InputValues->convertToGrayScale)
      {
        if(!filterListPtr->containsPlugin(k_ComplexCorePluginId))
        {
          return MakeErrorResult(-18542, "ComplexCore was not instantiated in this instance, so color to grayscale is not a valid option.");
        }
        auto grayScaleFilter = filterListPtr->createFilter(k_ColorToGrayScaleFilterHandle);
        if(nullptr == grayScaleFilter.get())
        {
          continue;
        }

        // This same filter was used to preflight so as long as nothing changes on disk this really should work....
        Arguments colorToGrayscaleArgs;
        colorToGrayscaleArgs.insertOrAssign("conversion_algorithm", std::make_any<ChoicesParameter::ValueType>(0));
        colorToGrayscaleArgs.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(m_InputValues->colorWeights));
        colorToGrayscaleArgs.insertOrAssign("input_data_array_vector", std::make_any<std::vector<DataPath>>(std::vector<DataPath>{imageDataProxy}));
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
          auto& oldArray = m_DataStructure.getDataRefAs<IDataArray>(imageDataProxy);
          id = oldArray.getId();
        }
        m_DataStructure.removeData(id);

        // rename grayscale array to reflect original
        {
          auto& gray = m_DataStructure.getDataRefAs<IDataArray>(imageDataProxy.getParent().createChildPath("gray" + imageDataProxy.getTargetName()));
          if(!gray.canRename(imageDataProxy.getTargetName()))
          {
            return MakeErrorResult(-18543, fmt::format("Unable to rename the grayscale array to {}", imageDataProxy.getTargetName()));
          }
          gray.rename(imageDataProxy.getTargetName());
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
