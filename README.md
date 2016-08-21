=== Overview ===

usbrelease is a simple tool for use on Linux to detach USB devices from their
kernel driver, and to reattach them later. This is useful if you need to use
the device from userspace, but you can't completely remove the kernel driver
(for example, because you have several devices handled by the same driver).

=== Installation ===

==== Pre-requisites ====
You need to have libusb1.0 installed (including development libraries and
headers). You also need to have GNU make and a recent (>= 4.9.0) version of g++
installed.

On Debian and Ubuntu distributions, you can install the required libraries with:

    sudo apt-get install build-essential libusb-1.0-0-dev

==== Build and install ====

You can install it as follows:

    git clone https://github.com/A1kmm/usbrelease
    cd usbrelease
    make
    sudo make install

=== Usage ===

You firstly need to identify what device you want to detach in terms of the bus ID
and address. Using lsusb may be helpful. You can use:

    usbrelease list

to list all devices and make sure you have selected a valid bus ID and address.

You also need to know what interface ID you want to detach the driver from.
Many USB devices only have one interface (numbered as interface 0), but others
might have more than one (for example, a network device that also has mass
storage for drivers). If in doubt about the interface number, but you know what
kernel driver, use cat /sys/bus/usb/drivers/*driverID*/*devicePath*/bInterface
to get the interface ID.

Once you have the bus ID, address, and interface to detach, use:

    usbrelease detach busID address interfaceID

Substituting busID, address, and interfaceID for the actual values.
Note that if the device is in use, the command may hang, so make sure nothing
is using the device before running the command. Due to the risk of a hang with
some drivers, always test this thoroughly with the device and driver you want
to use prior to use on a production system!

If you want to reattach the kernel driver, use:

    usbrelease reattach busID address interfaceID
