# gridfan
A C/C++ implementation of the [NZXT Grid+ v2](https://www.nzxt.com/products/grid-plus-v2) fan controller for GNU/Linux.

## Abstract
This is a C/C++ implementation of [CapitalF work](https://github.com/CapitalF/gridfan), similarly to [other works](https://github.com/m00dawg/controlfans) this one periodically checks system temperatures and adjust the fans speed accordingly.  
It is composed of 2 parts, a dynamic library `libgridfan.so` exposing the actual functionalities of the fan bus and the binary itself `gridfan` that uses the library to interact with it.

### Q/A
- Q: Why the serial file implementation is in C?
- A: I reused an old library I wrote years ago when I used to play around with Arduino, at the time C89 was my language of choice.
- Q: And why there is a Win32 implementation as well?
- A: Because at the time I was still mostly a Windows user, and I wanted the library to be crossplatform.
- Q: Does this thing work on Windows as well?
- A: No, `libsensors` (see below the [Dependencies](#deps) section) is not available on Windows (AFAICT). Maybe it could be possible to build the project using [MinGW](http://www.mingw.org/) but I haven't tried.

## Requirements
- A C++14 compiler (gcc 5 or clang 3.4 should work).
- CMake 3.1.3 or newer.

## <a name="deps"></a> Dependencies
- Te project uses [libsensors](https://linux.die.net/man/3/libsensors) to query system temperatures.

On Ubuntu 18.04 the library can be installed through `sudo apt install -y libsensors4-dev`, on CentOS/RHEL something like `yum install -y lm_sensors-devel` should work.

## Building
This is a simple "typical" CMake project so it should be enough to:
```
git clone https://github.com/lucastoro/gridfan.git
mkdir gridfan-build && cd gridfan-build
cmake ../gridfan
make -j (grep processor /proc/cpuinfo | wc -l)
```

## Installation
The project can be installed system-wide via
```
sudo make install
```
It installs 3 files:
- `gridfan`: the binary itself (default: `/usr/local/bin`)
- `libgridfan`: the library exposing the fanbus functionalies (default: `/usr/local/lib`)
- `gridfan.service`: a systemd unit file in `/etc/systemd/system`

The `.service` unit file will enable starting/stopping the daemon via systemct but to enable auto-starting at system boot you need to issue a:
```
sudo systemctl enable gridfan.service
```

## Algorithm
Currently the fan speed correction is done with a very simple algorithm that maps linearly the CPU temperature to a speed percentage, where `25° C = 0% speed` and `70° C = 100% speed`.  
The temperature is checked periodaically every second and the speed adjustment is performed on all 6 fans.

## Usage
The process cannot be configured, it takes no arguments and produces no output but the logs.  
Log messages are emitted via syslog (identifier = `gridfan`), by default the process produces very little logs, but sending it the SIGUSR1 signal will put it in a "verbose" mode so that every time it performs a fan speed adjustment it gets logged.  
Receiving again the SIGUSR1 signal will deactivate the verbose mode.  
It can be started either manually or as a systemd service (`systemctl enable gridfan; systemctl start gridfan`).

## Device access
When using the process through `systemctl` there will be no need for other configurations as the process will run as `root` but if you're willing to run the process as an unproviledged user you'll need to grant that user permissions to read and write the fan bus serial virtual file, please follow the [INSTRUCTIONS](https://github.com/CapitalF/gridfan/blob/master/README.txt) to configure your system properly.
