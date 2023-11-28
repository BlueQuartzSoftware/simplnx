.. This is the root documentation file. Note that if you are
   compiling additional plugins, and you want the python docs
   generated, you will need to add those to the list below

DREAM3D-NX Python Docs
=======================

Latest Version: 1.2.1
---------------------

The *complex* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n cxpython python=3.10
    conda activate cxpython
    conda install -c bluequartzsoftware complex

.. toctree::
   :maxdepth: 3
   :caption: Contents:

   Installation
   Overview
   DataObjects
   Geometry
   API
   Python_Introduction
   Reference_Frame_Notes
   ReleaseNotes_110
   ReleaseNotes_120
   ReleaseNotes_121
@PLUGIN_LIST@

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
