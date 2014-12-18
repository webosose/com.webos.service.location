location
======

Summary
-------
Location Framework Module is a middleware component which is responsible for providing geo-information

Description
-----------
Location framework is a service which provides the APIs to Application Layer for getting geo-in formations.LFW will
obtain geo-information from various positioning sources, such as GPS, WPS (Wi-Fi Positioning System), Cell ID based
positioning System. These services will be registered initially with the Luna system bus and Applications use these
services through the Luna system calls.

How to Build on Linux
=====================

## Dependencies

Below are the tools and libraries (and their minimum versions) required to build location:

* cmake (version required by openwebos/cmake-modules-webos)
* gcc 4.6.3
* glib-2.0 2.32.1
* geoclue.0.12.99
* dbus-glib.0.100
* dbus
* make (any version)
* openwebos/pbnjson
* openwebos/cmake-modules-webos 1.0.0 RC3
* openwebos/luna-service2 3.0.0
* openwebos/nyx-lib 2.0.0 RC2
* pkg-config 0.26


## Building

Once you have downloaded the source, enter the following to build it (after
changing into the directory under which it was downloaded):

    $ mkdir BUILD
    $ cd BUILD
    $ cmake ..
    $ make
    $ sudo make install

The directory under which the files are installed defaults to `/usr/local/webos`.
You can install them elsewhere by supplying a value for `WEBOS_INSTALL_ROOT`
when invoking `cmake`. For example:

    $ cmake -D WEBOS_INSTALL_ROOT:PATH=$HOME/projects/openwebos ..
    $ make
    $ make install

will install the files in subdirectories of `$HOME/projects/openwebos`.

Specifying `WEBOS_INSTALL_ROOT` also causes `pkg-config` to look in that tree
first before searching the standard locations. You can specify additional
directories to be searched prior to this one by setting the `PKG_CONFIG_PATH`
environment variable.

If not specified, `WEBOS_INSTALL_ROOT` defaults to `/usr/local/webos`.

To configure for a debug build, enter:

    $ cmake -D CMAKE_BUILD_TYPE:STRING=Debug ..

To see a list of the make targets that `cmake` has generated, enter:

    $ make help

## Uninstalling

From the directory where you originally ran `make install`, enter:

 $ [sudo] make uninstall

You will need to use `sudo` if you did not specify `WEBOS_INSTALL_ROOT`.

# Copyright and License Information

Unless otherwise specified, all content, including all source code files and
documentation files in this repository are:

Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved.

