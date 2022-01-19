# TMF882X Driver Porting Guide

This document gives an overview of the general changes necessary to port the
TMF882X MCU driver to other platforms.

## Core Driver Interface OS Abstraction Layer (OSAL)

The TMF882X core driver relies on an OSAL layer to abstract I/O, logging,
time interfaces, etc. from the core driver functionality.


See the Control / Data Flow graph below:

```
                                                +------------------------------+
============================                    | ==========================   |
 Platform Wrapper Interface                     |     TMF882X Core Driver      |
============================                    | ==========================   |
                                                |                              |
  platform_wrapper.c(h)   --------------------->|   tmf882x_interface.c(h) /   |
         ^                                      |     tmf882x_mode_app.c(h) /  |
         |                                      |       tmf882x_mode_bl.c(h) / |
         |                                      |               ...            |
         |                                      |             |                |
         |                                      |             |                |
         |                                      |             |                |
         +-------------  platform_shim.h  <-------------------+                |
                                |               |                              |
                                |               +------------------------------+
                                V
                    ' platform HAL/logging/etc '

```

See the module dependency chain below:

```
 tmf882x.h  <--+
               |
               |
       platform_wrapper.h/c  <--+
                                |
                                |
                      platform_shim.h  <--+
                                          |
                                          |
                                 tmf882x_host_interface.h  <--+
                                                              |
                                                              |
                                                <TMF882X Core Driver Modules>
```

Abstraction Module descriptions:

- tmf882x.h
    - Core driver output data types
    - This is part of the core driver and should not be changed. Client
      applications can include this file for parsing output data.

- platform_wrapper.c(h)
    - Abstraction layer between core driver and target platform/Client applications
    - purpose of this module is to decouple client applications from the core
      driver interface. In the reference MCU driver, platform interfaces
      needed by the core driver in the platform_shim.h layer are implemented here.
    - **This should be re-implemented for porting to new platforms**

- platform_shim.h
    - part of OSAL/HAL abstraction between the tmf882x core driver and target platform
    - all OSAL interface functions in this file must be implemented for the
      target platform for full functionality. The only exception are logging
      functions which are optional
    - **This should be re-implemented for porting to new platforms**

- tmf882x_host_interface.h
    - The purpose of this file is to include the target platform interface
      shim layer. This include is used by the core driver to include the
      OSAL interface declarations

- TMF882X Core Driver
    - All other files not listed above are considered part of the core driver
      and should not need to be modified for porting to other target platforms.

