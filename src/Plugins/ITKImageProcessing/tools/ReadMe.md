# Using these tools

You will need to clone SimpleITK from:
[https://github.com/SimpleITK/SimpleITK](https://github.com/SimpleITK/SimpleITK)

From the file `itk_filter_generator.py`, you will need to update the python variable `filters_to_process` to include the filters that you want to generate.

Then you can run the `itk_filter_generator.py` file to generate the filters.

An example usage of the python script is this:

````
python itk_filter_generator.py `pwd` `pwd`/SimpleITK/Code/BasicFilters/json /Users/mjackson/Workspace1/complex/src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters /Users/mjackson/Workspace1/complex/src/Plugins/ITKImageProcessing/test
````

## Modification Notes

For the ITKDiscreteGaussianImageTest that is generated, for the test against the `short` type, the tolerance should be 1.0. It is set at `0.6` in the JSON file. This will not work because we are comparing an integer type. The input, baseline and generated files were all visually compared to ensure the output is indeed correct (M. A. Jackson)
