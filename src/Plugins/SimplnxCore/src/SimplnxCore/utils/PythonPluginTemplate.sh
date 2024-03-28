#!/bin/sh

# This batch file can be run to activate a specific python virtual environment
# and set the necessary envionment variables to load the DREAM3D-NX Plugin

# If you launch an Anaconda Prompt you can then run this batch file to 
# activate your environment and set the needed variables. If you have installed
# your anaconda environment into a non-default location then you will need to
# modify the below line with this command instead, being sure to substitue
# the proper path to the environment.
# conda activate --prefix /opt/local/conda3/envs/pyd3d

conda activate @ANACONDA_ENV_NAME@

# ----------------------------------------------------------------------------
# The first variable to set is the PYTHONPATH which should point to a
# directory or directories that contains the top level python plugin
# folders. If you need to list mulitple directories to search for plugins
# you can use the ":" character to separate those paths.

export PYTHONPATH=@PYTHONPATH@

# ----------------------------------------------------------------------------
# The next variable is a list of all plugins that you would like to 
# load when DREAM3D-NX (or NXRunner) is run. If you have multiple plugins that
# you would like to load, list them all separated by a ":" character.

export SIMPLNX_PYTHON_PLUGINS=@SIMPLNX_PYTHON_PLUGINS@
