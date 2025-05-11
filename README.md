# SignalViewLV2
![SignalView LV2 plugin analysing speech with a sine sweep in the right channel.](https://github.com/TimKrause2/SignalViewLV2/blob/main/screenshot.png "SignalView analysing speech")
*SignalView in action analysing speech with a sine sweep in the right channel.*

## Description

SignalView is a LV2 plugin that provides a visual representation of a stereo audio signal. There is
a time domain view, frequency domain view and a sonogram.

## Download the plugin

Precompiled binaries and an LV2 plugin bundle is available for download [here](https://twkrause.ca/#signalviewlv2)

## User Interaction

To adjust the level limits use the scroll wheel on the mouse while hovering over the spectrum.
To adjust the frequency limit while using the linear scale press the left mouse button and move the mouse left or right.
The frequency limit for the logarithmic scale is fixed at the Nyquist frequency.
To toggle between logarithmic scale and linear scale click the right mouse button.

## Building

### Obtaining the source

The plugin depends on other github projects and therefore requires arguments to the clone
command.

    git clone --recurse-submodules https://github.com/TimKrause2/SignalViewLV2.git


### Prerequisits

If you need to install a compiler and the `make` command and you are on Ubuntu use the following
command.

    sudo apt install build-essential

To build the plugin there are packages that are required. If you are using Ubuntu you can use
the following commands to install the dependencies.

    sudo apt install libxext-dev
    sudo apt install libfftw3-dev
    sudo apt install libx11-dev
    sudo apt install libxcursor-dev
    sudo apt install libxrandr-dev
    sudo apt install libglx-dev
    sudo apt install libfreetype-dev
    sudo apt install libglm-dev

### Make and Install

To build and install SignalView is simple. In the source directory just enter the following command.

    make

