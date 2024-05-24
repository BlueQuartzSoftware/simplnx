# Generate Python Plugin and/or Filters

## Description

The **Generate Python Plugin and/or Filters** is a powerful tool in the DREAM3D-NX environment that allows users to generate or update Python plugins and filter codes. This filter provides an interface for setting up and configuring Python filters within DREAM3D-NX pipelines, either by creating new plugins or by adding to existing ones.

## Usage

### Configuration

The filter requires several parameters to be set, which dictate whether a new plugin is created or an existing one is modified. Key parameters include:

- `Use Existing Plugin`: A flag indicating whether to modify an existing plugin.
- `Name of Plugin`: The name of the plugin.
- `Human Name of Plugin`: A human-readable name for the plugin.
- `Existing Plugin Location`: The directory where the existing plugin is located (for updates).
- `Plugin Output Directory`: The directory where the new plugin will be stored.
- `Filter Names`: A list of filter names to be included in the plugin, separated by commas.

### Generating a New Plugin

To generate a new Python plugin:
1. Set `Use Existing Plugin` to `Off`.
2. Provide a `Name of Plugin`, `Human Name of Plugin`, `Plugin Output Directory`, and a list of `Filter Names`.
3. Execute the filter. It will generate the necessary plugin structure and files in the specified output directory.

### Updating an Existing Plugin

To add filters to an existing Python plugin:
1. Set `Use Existing Plugin` to `On`.
2. Provide the `Existing Plugin Location` where the existing plugin is located.
3. Provide the `Name of Plugin`, `Human Name of Plugin`, and the list of `Filter Names` you wish to add.
4. Execute the filter. It will update the existing plugin with the new filters.

#### Manual Plugin Creation

If your plugin was not generated using the provided filter but was instead created manually, you may need to perform additional steps to ensure proper integration:

1. **Python Import Statements**:
   - In the absence of the `# FILTER_INCLUDE_INSERT` token, you must manually insert the Python import statements for the new filters into both the `__init__.py` file and the `Plugin.py` file.

2. **Filter Name Insertion**:
   - If the `# FILTER_NAME_INSERT` token is missing, you need to manually add each filter name to specific methods in two files:
       - In the `__init__.py` file, add to the `all` method.
       - In the `Plugin.py` file, add to the `get_filters` method. 

## Example Pipelines

None

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
