#!/bin/sh
gdbus-codegen   --interface-prefix org.usb.device.            \
                --generate-c-code gadgetd-gdbus-codegen       \
                --c-generate-object-manager                   \
                --c-namespace Gadgetd                         \
                --generate-docbook generated-docs             \
                  gadgetd.xml

