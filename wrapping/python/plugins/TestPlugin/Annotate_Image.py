import simplnx as nx
import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest
import numpy as np
from PIL import Image, ImageDraw, ImageFont

def generate_data_structure(data_structure: nx.DataStructure):
    # Filter 1
    nx_filter = cxor.ReadAngDataFilter()
    result = nx_filter.execute(
        data_structure=data_structure,
        cell_attribute_matrix_name=("Cell Data"),
        cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
        data_container_name=nx.DataPath("DataContainer"),
        input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
    )
    nxtest.check_filter_result(nx_filter, result)

    # Filter 2
    nx_filter = cxor.RotateEulerRefFrameFilter()
    result = nx_filter.execute(
        data_structure=data_structure,
        euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
        rotation_axis=[0.0, 0.0, 1.0, 90.0]
    )
    nxtest.check_filter_result(nx_filter, result)

    # Filter 3
    nx_filter = nx.RotateSampleRefFrameFilter()
    result = nx_filter.execute(
        data_structure=data_structure,
        remove_original_geometry=True,
        rotate_slice_by_slice=False,
        rotation_axis=[0.0, 1.0, 0.0, 180.0],
        rotation_representation=0,
        selected_image_geometry=nx.DataPath("DataContainer")
    )
    nxtest.check_filter_result(nx_filter, result)

    # Filter 4
    threshold_1 = nx.ArrayThreshold()
    threshold_1.array_path = nx.DataPath("DataContainer/Cell Data/Confidence Index")
    threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
    threshold_1.value = 0.1

    threshold_set = nx.ArrayThresholdSet()
    threshold_set.thresholds = [threshold_1]

    nx_filter = nx.MultiThresholdObjects()
    result = nx_filter.execute(
        data_structure=data_structure,
        array_thresholds=threshold_set,
        created_data_path="Mask",
        created_mask_type=nx.DataType.boolean
    )
    nxtest.check_filter_result(nx_filter, result)

    # Filter 5
    nx_filter = cxor.GenerateIPFColorsFilter()
    result = nx_filter.execute(
        data_structure=data_structure,
        cell_euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
        cell_ipf_colors_array_name=("IPFColors"),
        cell_phases_array_path=nx.DataPath("DataContainer/Cell Data/Phases"),
        crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
        mask_array_path=nx.DataPath("DataContainer/Cell Data/Mask"),
        reference_dir=[0.0, 0.0, 1.0],
        use_mask=True
    )
    nxtest.check_filter_result(nx_filter, result)

    # Filter 6
    nx_filter = cxitk.ITKImageWriter()
    output_file_path = nxtest.GetDataDirectory() + "/Output/Edax_IPF_Colors/Small_IN100_Slice_1.png"
    result = nx_filter.execute(
        data_structure=data_structure,
        file_name=output_file_path,
        image_array_path=nx.DataPath("DataContainer/Cell Data/IPFColors"),
        image_geom_path=nx.DataPath("DataContainer"),
        index_offset=0,
        plane=0
    )
    nxtest.check_filter_result(nx_filter, result)

#------------------------------------------------------------------------------

def annotate_image(array_path, title, output_path):
    # Create a Data Structure
    data_structure = nx.DataStructure()
    generate_data_structure(data_structure)

    # Get numpy view of image data
    array_path = nx.DataPath("DataContainer/Cell Data/IPFColors")
    npdata = data_structure[array_path].npview()
    
    # Check dimensions
    if npdata.shape[0] != 1 and npdata.shape[1] != 1 and npdata.shape[2] != 1:
        raise ValueError("The dimensions of the array do not meet the required condition.")
    
    npdata = npdata.squeeze()
    image = Image.fromarray(npdata)

    # Increase the image size
    border_size = 50
    new_size = (image.width + 2 * border_size, image.height + 2 * border_size)
    new_image = Image.new("RGB", new_size, color="black")
    new_image.paste(image, (border_size, border_size))

    font_path = "arial.ttf" 
    font_size = 10  # Adjust font size as needed
    font = ImageFont.truetype(font_path, font_size)

    # Ask user for micro bar inclusion
    micro_bar = input("Include micro bar? (y/n): ").lower() == 'y'

    # Add a micro/scalar bar
    if micro_bar:
        draw = ImageDraw.Draw(new_image)
        bar_width = 150
        bar_height = 6
        bar_margin = 10

        # Position of the micro bar at the bottom right
        bar_x = new_image.width - bar_width - bar_margin - border_size + 54
        bar_y = new_image.height - bar_height - bar_margin - border_size + 54

        # Draw the micro bar
        draw.rectangle([bar_x, bar_y, bar_x + bar_width, bar_y + bar_height], fill="white")

        # Adding tick marks for the micro bar
        tick_length = 6
        num_ticks = 3
        tick_interval = bar_width / (num_ticks - 1)

        for i in range(num_ticks):
            tick_x = bar_x + i * tick_interval
            draw.line([(tick_x, bar_y), (tick_x, bar_y - tick_length)], fill="white")

        # Optionally, add labels for 1, 5, 10 microns
        draw.text((bar_x, bar_y - 15), "1µ", fill="white", font=font)
        draw.text((bar_x + bar_width // 2 - 10, bar_y - 15), "5µ", fill="white", font=font)
        draw.text((bar_x + bar_width - 20, bar_y - 15), "10µ", fill="white", font=font)


    # Ask user for title position
    valid_positions = ["top_left", "top_center", "top_right"]
    while True:
        title_pos = input("Enter title position (top_left, top_center, top_right): ")
        if title_pos in valid_positions:
            break
        print("Invalid position. Please choose from 'top_left', 'top_center', 'top_right'.")

    # Add a title
    draw = ImageDraw.Draw(new_image)
    text_width, text_height = draw.textsize(title, font=font)
    title_margin = 5
    
    # Determine title position
    if title_pos == "top_left":
        text_x = border_size - 45
    elif title_pos == "top_center":
        text_x = (new_image.width - text_width) // 2
    elif title_pos == "top_right":
        text_x = new_image.width - border_size - text_width + 45
    else:
        raise ValueError("Invalid title position")
    
    text_y = title_margin
    draw.text((text_x, text_y), title, fill="yellow", font=font)

    # Save the modified image
    new_image.save(output_path)

annotate_image(
    nx.DataPath("DataContainer/Cell Data/IPFColors"),
    "Edax Colors", 
    "C:/Users/Alex Jackson/Downloads/DREAM3DNX-7.0.0-RC-9-windows-AMD64/Data/Output/Edax_IPF_Colors/modified_image_1.png"
)