Installation
============

The *complex* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n cxpython python=3.8
    conda activate cxpython
    conda install -c bluequartzsoftware complex
