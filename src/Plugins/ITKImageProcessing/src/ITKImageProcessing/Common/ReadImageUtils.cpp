#include "ReadImageUtils.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

namespace
{
const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_CropImageGeomFilterId = *Uuid::FromString("e6476737-4aa7-48ba-a702-3dfab82c96e2");
const FilterHandle k_CropImageGeomFilterHandle(k_CropImageGeomFilterId, k_SimplnxCorePluginId);
} // namespace

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

    bool cropImage = imageReaderOptions.CroppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping;
    bool crop2dImage = cropImage && (imageReaderOptions.CroppingOptions.cropX || imageReaderOptions.CroppingOptions.cropY);
    if(crop2dImage)
    {
      FilterList* filterListPtr = Application::Instance()->getFilterList();
      if(!filterListPtr->containsPlugin(k_SimplnxCorePluginId))
      {
        IFilter::PreflightResult errorResult = IFilter::MakePreflightErrorResult(-18542, "The plugin SimplnxCore was not instantiated in this instance, so image cropping is not available.");
        return errorResult.outputActions;
      }

      std::unique_ptr<IFilter> cropImageGeomFilter = filterListPtr->createFilter(k_CropImageGeomFilterHandle);
      if(nullptr == cropImageGeomFilter)
      {
        IFilter::PreflightResult errorResult = IFilter::MakePreflightErrorResult(-18543, "Unable to create an instance of the crop image geometry filter, so image cropping is not available.");
        return errorResult.outputActions;
      }

      DataStructure tmpDs;
      DataPath tmpGeomPath = DataPath({"tmpGeom"});
      ImageGeom* tmpGeom = ImageGeom::Create(tmpDs, tmpGeomPath.getTargetName());
      AttributeMatrix* am = AttributeMatrix::Create(tmpDs, "CellData", std::vector<usize>(dims.crbegin(), dims.crend()), tmpGeom->getId());
      tmpGeom->setCellData(*am);
      tmpGeom->setDimensions(dims);
      tmpGeom->setOrigin(origin);
      tmpGeom->setSpacing(spacing);

      Arguments cropImageGeomArgs;
      cropImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(tmpGeomPath));
      cropImageGeomArgs.insertOrAssign("use_physical_bounds", std::make_any<bool>(imageReaderOptions.CroppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume));
      cropImageGeomArgs.insertOrAssign("crop_x_dim", std::make_any<bool>(imageReaderOptions.CroppingOptions.cropX));
      cropImageGeomArgs.insertOrAssign("crop_y_dim", std::make_any<bool>(imageReaderOptions.CroppingOptions.cropY));
      cropImageGeomArgs.insertOrAssign("crop_z_dim", std::make_any<bool>(false)); // Do this because we're cropping a 2D image
      if(imageReaderOptions.CroppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
      {
        cropImageGeomArgs.insertOrAssign("min_voxel", std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(imageReaderOptions.CroppingOptions.xBoundVoxels[0]),
                                                                                                       static_cast<uint64>(imageReaderOptions.CroppingOptions.yBoundVoxels[0]),
                                                                                                       static_cast<uint64>(imageReaderOptions.CroppingOptions.zBoundVoxels[0])}));
        cropImageGeomArgs.insertOrAssign("max_voxel", std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(imageReaderOptions.CroppingOptions.xBoundVoxels[1]),
                                                                                                       static_cast<uint64>(imageReaderOptions.CroppingOptions.yBoundVoxels[1]),
                                                                                                       static_cast<uint64>(imageReaderOptions.CroppingOptions.zBoundVoxels[1])}));
      }
      else
      {
        cropImageGeomArgs.insertOrAssign("min_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(imageReaderOptions.CroppingOptions.xBoundPhysical[0]),
                                                                                                        static_cast<float64>(imageReaderOptions.CroppingOptions.yBoundPhysical[0]),
                                                                                                        static_cast<float64>(imageReaderOptions.CroppingOptions.zBoundPhysical[0])}));
        cropImageGeomArgs.insertOrAssign("max_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(imageReaderOptions.CroppingOptions.xBoundPhysical[1]),
                                                                                                        static_cast<float64>(imageReaderOptions.CroppingOptions.yBoundPhysical[1]),
                                                                                                        static_cast<float64>(imageReaderOptions.CroppingOptions.zBoundPhysical[1])}));
      }
      cropImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(true));

      IFilter::PreflightResult cropImageResult = cropImageGeomFilter->preflight(tmpDs, cropImageGeomArgs);
      if(cropImageResult.outputActions.invalid())
      {
        return cropImageResult.outputActions;
      }

      Result<> actionsResult = cropImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
      if(actionsResult.invalid())
      {
        return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
      }

      auto croppedGeom = tmpDs.getDataRefAs<ImageGeom>(tmpGeomPath);
      dims = croppedGeom.getDimensions().toContainer<std::vector<usize>>();
      spacing = croppedGeom.getSpacing().toContainer<std::vector<float32>>();
      origin = croppedGeom.getOrigin().toContainer<std::vector<float32>>();
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

    ShapeType cDims = {nComponents};

    actions.appendAction(std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(dims), origin.toContainer<CreateImageGeometryAction::OriginType>(),
                                                                     spacing.toContainer<CreateImageGeometryAction::SpacingType>(), cellDataName));

    if(imageReaderOptions.ChangeDataType && ExecuteNeighborFunction(ITK::detail::PreflightTypeConversionValidateFunctor{}, *numericType, imageReaderOptions.ImageDataType))
    {
      actions.appendAction(
          std::make_unique<CreateArrayAction>(imageReaderOptions.ImageDataType, std::move(arrayDims), std::move(cDims), imageGeomPath.createChildPath(cellDataName).createChildPath(arrayName)));
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
