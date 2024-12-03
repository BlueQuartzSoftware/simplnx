Installation
============

Latest Version: 24.12.02
-------------------------

The *simplnx* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment and installing SIMPLNX into that environment.


.. attention::

    MacOS Machines will need to export an environment variable BEFORE trying to do the
    conda installer.

.. code:: shell

    export CONDA_OVERRIDE_OSX=11.0


Full DREAM3D-NX Installation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. attention::

    This kind of installation will allow you to utilize the full DREAM3D-NX application

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.12
    conda activate nxpython
    conda install -c bluequartzsoftware dream3dnx
