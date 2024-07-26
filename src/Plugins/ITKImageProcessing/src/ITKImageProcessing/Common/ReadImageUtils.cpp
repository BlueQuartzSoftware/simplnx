#include "ReadImageUtils.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"

using namespace nx::core;

namespace cxItkImageReaderFilter
{

//------------------------------------------------------------------------------
Result<OutputActions> ReadImagePreflight(const std::string& fileName, DataPath imageGeomPath, const std::string& cellDataName, const std::string& arrayName,
                                         const ImageReaderOptions& imageReaderOptions)
{
  OutputActions actions;

  try
  {
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(fileName.c_str(), itk::CommonEnums::IOFileMode::ReadMode);
    if(imageIO == nullptr)
    {
      return MakeErrorResult<OutputActions>(-5, fmt::format("ITK could not read the given file \"{}\". Format is likely unsupported.", fileName));
    }

    imageIO->SetFileName(fileName);
    imageIO->ReadImageInformation();

    itk::ImageIOBase::IOComponentEnum component = imageIO->GetComponentType();

    std::optional<DataType> numericType = ITK::ConvertIOComponentToDataType(component);
    if(!numericType.has_value())
    {
      return MakeErrorResult<OutputActions>(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
    }

    uint32 nDims = imageIO->GetNumberOfDimensions();

    std::vector<size_t> dims = {1, 1, 1};
    FloatVec3 origin = {0.0f, 0.0f, 0.0f};
    FloatVec3 spacing = {1.0f, 1.0f, 1.0f};

    for(uint32 i = 0; i < nDims; i++)
    {
      dims[i] = static_cast<usize>(imageIO->GetDimensions(i));
      origin[i] = static_cast<float32>(imageIO->GetOrigin(i));
      spacing[i] = static_cast<float32>(imageIO->GetSpacing(i));
    }

    if(imageReaderOptions.OverrideSpacing)
    {
      spacing = imageReaderOptions.Spacing;
    }

    if(imageReaderOptions.OverrideOrigin)
    {
      DataStructure junk;
      ImageGeom* imageGeomPtr = ImageGeom::Create(junk, "Junk");

      origin = imageReaderOptions.Origin;

      imageGeomPtr->setDimensions(dims);
      imageGeomPtr->setOrigin(origin);
      imageGeomPtr->setSpacing(spacing);

      if(imageReaderOptions.OriginAtCenterOfGeometry)
      {
        BoundingBox3Df bounds = imageGeomPtr->getBoundingBoxf();
        FloatVec3 centerPoint(bounds.center());
        origin = origin - (centerPoint - origin);
      }
    }

    uint32 nComponents = imageIO->GetNumberOfComponents();

    // DataArray dimensions are stored slowest to fastest, the opposite of ImageGeometry
    std::vector<usize> arrayDims(dims.crbegin(), dims.crend());

    std::vector<usize> cDims = {nComponents};

    actions.appendAction(std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(dims), origin.toContainer<CreateImageGeometryAction::OriginType>(),
                                                                     spacing.toContainer<CreateImageGeometryAction::SpacingType>(), cellDataName));

    if(imageReaderOptions.ChangeDataType)
    {
      DataPath tempPath = imageGeomPath.createChildPath(cellDataName).createChildPath("." + arrayName);
      actions.appendAction(std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(imageReaderOptions.ImageDataType), std::move(arrayDims), std::move(cDims),
                                                               imageGeomPath.createChildPath(cellDataName).createChildPath(arrayName)));
      actions.appendAction(std::make_unique<CreateArrayAction>(*numericType, std::move(arrayDims), std::move(cDims), tempPath));
      actions.appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath, DeleteDataAction::DeleteType::JustObject));
    }
    else
    {
      actions.appendAction(std::make_unique<CreateArrayAction>(*numericType, std::move(arrayDims), std::move(cDims), imageGeomPath.createChildPath(cellDataName).createChildPath(arrayName)));
    }
  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult<OutputActions>(-55557, fmt::format("ITK exception was thrown while processing input file: {}", err.what()));
  }

  return {std::move(actions)};
}
} // namespace cxItkImageReaderFilter
