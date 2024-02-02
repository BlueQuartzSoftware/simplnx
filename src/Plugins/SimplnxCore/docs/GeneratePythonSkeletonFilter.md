# Generate Python Plugin or Python Filter

## Group

Python

## Description

This **Filter** generates the Python skeleton code for a new DREAM3D-NX/SIMPL-NX filters.  These new Python filters can either be written to a newly generated Python plugin, or to an existing Python plugin.

### Generating A New Plugin

If the **Use Existing Plugin** parameter is OFF, then this filter will generate both a new Python skeleton plugin and new Python skeleton filters.

The following parameters are available when **Use Existing Plugin** is OFF:

+ Name of Plugin - The name of the plugin, referred to in the generated code.
+ Human Name of Plugin - The human-readable version of the plugin name.
+ Plugin Output Directory - The location on the file system where the plugin will be generated.
+ Filter Names - The names of filters that will be generated in the new plugin, separated by commas.

### Using An Existing Plugin

If the **Use Existing Plugin** parameter is ON, then this filter will generate new Python skeleton filters in an existing plugin.  If the existing plugin was created manually and NOT created using this filter, some additional edits will need to be done in a few of the other plugin files.

The following parameters are available when **Use Existing Plugin** is ON:

+ Existing Plugin Location - The file system path where the existing plugin is located.
+ Filter Names - The names of filters that will be generated in the existing plugin, separated by commas.

**NOTE:** This option searches the existing plugin's **\_\_init\_\_.py** and **Plugin.py** files for the following comment tokens:

1. \# FILTER_INCLUDE_INSERT
2. \# FILTER_NAME_INSERT

Both of these tokens are included in both files for any plugin that was originally generated using this filter.

#### What if my plugin was created manually (not generated using this filter)?

+ If the **\# FILTER_INCLUDE_INSERT** token cannot be found, then the Python import statements for the new filters will need to be manually added to both files.
+ If the **\# FILTER_NAME_INSERT** token cannot be found, then each filter name will need to be manually added to the **all** and **get_filters** methods in the **\_\_init\_\_.py** and **Plugin.py** files, respectively.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
