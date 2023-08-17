import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor


import numpy as np
import matplotlib.pyplot as plt


# Create the DataStructure object and define the dimensions of the geometry
data_structure = cx.DataStructure()

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "/private/tmp/basic_ebsd.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")


#------------------------------------------------------------------------------
# Get the underlying data from the DataStructure
#------------------------------------------------------------------------------
data_array = data_structure[cx.DataPath(["Small IN100", "Scan Data", "Image Quality"])]
# Get the underlying DataStore object
data_store = data_array.store

# Get a View into the DataArray through a Numpy.View
npview = data_store.npview()
# Change the underlying data based on some criteria using Numpy
npview[npview < 120] = 0

#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------
output_file_path = "/tmp/import_data.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ExportDREAM3DFilter")


#------------------------------------------------------------------------------
# View with MatPlotLib
#------------------------------------------------------------------------------
# Make a copy to that we can use MatPlotLib
npdata = data_store.npview().copy()

# Remove any dimension with '1' for MatPlotLib?
npdata = np.squeeze(npdata, axis=0)
npdata = np.squeeze(npdata, axis=2)

plt.imshow(npdata)
plt.title("Image Quality (Small IN100 from File)")
#plt.axis('off')  # to turn off axes
plt.show()
