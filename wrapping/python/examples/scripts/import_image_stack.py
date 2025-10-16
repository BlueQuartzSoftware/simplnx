import simplnx as nx
import itkimageprocessing as itknx
import simplnx_test_dirs as nxtest

def main():
    ds = nx.DataStructure()

    # --- Describe your input file list (e.g., img_0000.tif ... img_0199.tif) ---
    gfl = nx.GeneratedFileListParameter.ValueType()
    gfl.input_path    = "/path/to/images"            # <--- CHANGE ME
    gfl.file_prefix   = "input_image_"
    gfl.file_suffix   = ""
    gfl.file_extension= ".tif"
    gfl.start_index   = 0
    gfl.end_index     = 199
    gfl.increment_index = 1
    gfl.padding_digits  = 4                          # img_0000.tif style
    gfl.ordering      = nx.GeneratedFileListParameter.Ordering.LowToHigh

    # --- Geometry + data names to create/populate ---
    image_geom_path   = nx.DataPath("Image Geometry")
    image_data_name   = "ImageData"

    # --- Origin & spacing for the imported stack ---
    origin  = [0.0, 0.0, 0.0]
    spacing = [1.0, 1.0, 1.0]

    # --- Build the cropping parameter value ---
    cv = nx.CropGeometryParameter.ValueType()

    # Option A: crop by voxel indices (inclusive)
    cv.type   = nx.CropGeometryParameter.TypeEnum.VoxelSubvolume
    cv.crop_x = True
    cv.crop_y = True
    cv.crop_z = True
    # IntVec2(min_idx, max_idx) for each axis
    cv.x_bound_voxels = nx.IntVec2(10, 210)
    cv.y_bound_voxels = nx.IntVec2(20, 220)
    cv.z_bound_voxels = nx.IntVec2(5, 150)

    # Option B (alternative): crop by physical coordinates
    # cv.type   = nx.CropGeometryParameter.TypeEnum.PhysicalSubvolume
    # cv.crop_x = True; cv.crop_y = True; cv.crop_z = False
    # cv.x_bound_physical = nx.FloatVec2(0.0, 200.0)
    # cv.y_bound_physical = nx.FloatVec2(10.0, 210.0)
    # cv.z_bound_physical = nx.FloatVec2(0.0, 0.0)  # ignored if crop_z == False

    # --- Other filter options ---
    image_transform_choice = 0          # 0 = "No Transform", 1 = "Flip About X", 2 = "Flip About Y"
    convert_to_grayscale   = False
    color_weights          = [0.2126, 0.7152, 0.0722]  # used if convert_to_grayscale=True
    resample_images_choice = 0          # 0 = "No Resample", 1 = "Scaling as Percent", 2 = "Exact X/Y Dimensions For Resampling Along Z Axis"
    scaling                = 1.0        # used if resample_images_choice is 1
    exact_xy_dimensions    = [0, 0]     # used if resample_images_choice is 2

    # --- Execute the filter ---
    result = itknx.ITKImportImageStackFilter.execute(
        data_structure=ds,
        input_file_list_object=gfl,
        origin=origin,
        spacing=spacing,
        output_image_geometry_path=image_geom_path,
        image_data_array_name=image_data_name,
        image_transform_index=image_transform_choice,
        convert_to_gray_scale=convert_to_grayscale,
        color_weights=color_weights,
        resample_images_index=resample_images_choice,
        scaling=scaling,
        exact_xy_dimensions=exact_xy_dimensions,
        cropping_options=cv
    )

    nxtest.check_filter_result(itknx.ITKImportImageStackFilter, result)

if __name__ == "__main__":
    main()