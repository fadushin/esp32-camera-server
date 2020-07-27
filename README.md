# ESP32-CamServer

This ESP32 application will create a camera server accessible via HTTP, for grabbing screen captures and sending them over HTTP to a client (e.g., web browser, script, etc.)  In addition, the HTTP endpoint supports some management endpoints, so that you can configure parts of the WIFI and settings.

> Note.  This program only supports screen _capture_.  Typical code for streaming JPEGs over HTTP relies on use of the HTTP `multipart/x-mixed-replace` header, which is kind of a hacky way to do streaming.  It also doesn't work very well with ESP32 devices currently, as support for streaming will take over a full core and will block management API calls.  Future versions of this software may support streaming.  But for now, only screen capture is supported.

This application is meant to be self-contained, in that you should be able to build and flash the application, and then use the application without having to make any changes in source code.  Users _may_, of course, make as many changes as they like, subject to the licensing terms of the software.  However, the intention is that most users should be able to just flash the application and then make any needed changes through the REST API.

Note that this software also includes scripts for configuring the device.  These scripts provide a simple front end for device configuration.  The scripts are equivalent to operations that could otherwise be implemented through `curl` or a similar program.  The scripts are only there to help guide the user and maybe reduce the amount of work needed to manage the device.

> IMPORTANT.  This application currently provides no security over HTTP connections established with the device.  This is especially important when running administrative commands over the device, as in all liklihood you will need to pass WIFI credentials to the device over plain HTTP.  Unauthorized snooping applications (e.g., `tcpdump`, Wireshark) can trivially intercept and read any traffic used over these connections.  This software should *ONLY* be run on a trusted network.

# Getting Started

## Building and Flashing ESP32-CamServer


## Running ESP32-CamServer

When the device first starts, it will run in AP mode.  The WIFI network created is called `ESP32-CamServer-<mac>` where `<mac>` is the factory-assigned MAC address of the device.

You can change the mode to `STA` and set the SSID and password for
