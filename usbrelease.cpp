#include <libusb-1.0/libusb.h>
#include <ios>
#include <string>
#include <iostream>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <cstring>

void raise_as_ioerror(int code) throw(std::ios_base::failure) {
  throw std::ios_base::failure(libusb_error_name(code));
}

class USBContext {
 private:
  libusb_context* mContext;
  
 public:
  USBContext() throw(std::ios_base::failure) {
    int ret = libusb_init(&mContext);
    if (ret != 0)
      raise_as_ioerror(ret);
  }
  
  ~USBContext() {
    libusb_exit(mContext);
  }

  libusb_context* getContext() const {
    return mContext;
  }
};

class USBDevice;
class USBHandle {
private:
  libusb_device_handle* mHandle;
  
public:
  USBHandle(const USBDevice& device);

  ~USBHandle() {
    libusb_close(mHandle);
  }

  int detachKernelDriver(int interface) {
    int ret = libusb_detach_kernel_driver(mHandle, interface);
    if (ret != 0)
      raise_as_ioerror(ret);
  }

  int reattachKernelDriver(int interface) {
    int ret = libusb_attach_kernel_driver(mHandle, interface);
    if (ret != 0)
      raise_as_ioerror(ret);
  }
};

class USBDevice {
private:
  libusb_device* mDevice;

public:
  USBDevice(libusb_device* aDevice) {
    mDevice = aDevice;
  }

  USBDevice(const USBDevice& aOther) {
    mDevice = aOther.getDevice();
    libusb_ref_device(mDevice);
  }
  
  ~USBDevice() {
    libusb_unref_device(mDevice);
  }

  libusb_device* getDevice() const {
    return mDevice;
  }

  static std::list<USBDevice> listAllDevices(const USBContext& context)
    throw(std::ios_base::failure) {
    
    std::list<USBDevice> devices;
    libusb_device** device_list;

    device_list = NULL;
    ssize_t device_list_size = libusb_get_device_list(context.getContext(),
                                                      &device_list);

    if (device_list == NULL)
      raise_as_ioerror(device_list_size);
    
    for (ssize_t i = 0; i < device_list_size; i++) {
      devices.push_back(USBDevice(device_list[i]));
    }

    libusb_free_device_list(device_list, 0);
    
    return devices;
  }

  uint8_t getBusNumber() const {
    return libusb_get_bus_number(mDevice);
  }

  uint8_t getPortNumber() const {
    return libusb_get_port_number(mDevice);
  }

  uint8_t getDeviceAddress() const {
    return libusb_get_device_address(mDevice);
  }  
};

class AttachSpec {
private:
  uint8_t mBus;
  uint8_t mAddr;
  uint8_t mInterface;

  static uint8_t parseOneParam(const char* input, const char* what)
    throw (std::invalid_argument) {
    
    try {
      int ret = std::stoi(input);
      if (ret < 0 || ret > 255)
        throw std::invalid_argument(std::string("Invalid ") + what);

      return static_cast<uint8_t>(ret);
    } catch (std::exception e) {
      throw std::invalid_argument(std::string("Invalid ") + what);
    }
  }
  
public:
  AttachSpec(uint8_t aBus, uint8_t aAddr, uint8_t aInterface) {
    mBus = aBus;
    mAddr = aAddr;
    mInterface = aInterface;
  }

  static AttachSpec parseFromCommand(const char* busArg, const char* addrArg,
                                     const char* ifaceArg)
    throw(std::invalid_argument) {

    uint8_t bus = parseOneParam(busArg, "busID"),
            addr = parseOneParam(addrArg, "addrressID"),
            iface = parseOneParam(ifaceArg, "interfaceID");
    return AttachSpec(bus, addr, iface);
  }

  bool matches(const USBDevice& device) const {
    return device.getBusNumber() == mBus && device.getDeviceAddress() == mAddr;
  }

  uint8_t getInterface() {
    return mInterface;
  }
};

USBHandle::USBHandle(const USBDevice& device)
{
  libusb_open(device.getDevice(), &mHandle);
}

void print_usage() {
  std::cout << "Usage: usbrelease command" << std::endl
            << "Commands:" << std::endl
            << "   list" << std::endl
            << "      Lists all USB devices" << std::endl
            << "   detach (busID) (addr) (interfaceID)" << std::endl
            << "      Detaches a USB interface from the kernel driver" << std::endl
            << "   reattach (busID) (addr) (interfaceID)" << std::endl
            << "      Reattaches a USB interface to the kernel driver" << std::endl;
}

void do_list(std::list<USBDevice>& devices) throw(std::ios_base::failure) {
  for (std::list<USBDevice>::iterator i = devices.begin(); i != devices.end(); i++) {
    std::cout << "Device: bus=" << static_cast<int>((*i).getBusNumber()) << " port="
              << static_cast<int>((*i).getPortNumber())
              << " addr=" << static_cast<int>((*i).getDeviceAddress()) << std::endl;
  }
}

int do_reattach(USBDevice& device, AttachSpec& spec) {
  USBHandle(device).reattachKernelDriver(spec.getInterface());
}

int do_detach(USBDevice& device, AttachSpec& spec) {
  USBHandle(device).detachKernelDriver(spec.getInterface());
}

int main(int argc, char** argv) {
  try {
    USBContext context = USBContext();
    std::list<USBDevice> devices = USBDevice::listAllDevices(context);

    if (argc < 2) {
      print_usage();
      return 1;
    }

    if (!strcmp(argv[1], "list")) {
      do_list(devices);
    } else if (!strcmp(argv[1], "detach") || !strcmp(argv[1], "reattach")) {
      if (argc < 5) {
        print_usage();
        return 1;
      }
      else {
        AttachSpec spec = AttachSpec::parseFromCommand(argv[2], argv[3], argv[4]);
        auto it =
          std::find_if(devices.begin(), devices.end(), [spec](const USBDevice& dev) {
              return spec.matches(dev);
            });
        if (it == devices.end()) {
          std::cout << "Device not found" << std::endl;
        }

        if (!strcmp(argv[1], "detach"))
          do_detach(*it, spec);
        else
          do_reattach(*it, spec);
      }
    } else {
      print_usage();
      return 1;
    }

    return 0;
  } catch (std::exception& e) {
    std::cout << "Failure: " << e.what() << std::endl;
    return 2;
  }
}
