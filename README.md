#vmlauncher
##Introduction

vmlauncher is a launchd daemon that manages the lifetime of a VMWare virtual machine. When the daemon starts, it launches the managed virtual machine and then suspends the virtual machine when the daemon is terminated. Whie it's running, it monitors the virtual machine's status and will power it back on if it shuts down.

Note: **VMWare will crash if you open it while the virtual machines are being managed**. If you need to look at the virtual machines in the VMWare application, you must first stop or unload the daemon.

##Installation

vmlauncher requires VMWare 6.0.2 installed to `/Applications/VMWare Fusion.app`. If your installation path differs, adjust the `VMWARE` variable in the Makefile accordingly. Older versions of VMWare may work, but have not been tested.

     make
     sudo make install

##Usage

To use vmlauncher as a launchd daemon, the [appropriate configuration plist](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man5/launchd.plist.5.html) to be installed to `/Library/LaunchDaemons` and the job loaded/started using [launchctl](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/launchctl.1.html). Here is an example configuration:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.example.vmlauncher</string>
    <!-- The virtual machine may take some time to suspend. Setting this value too low could lead to VMWare being forcibly killed during suspending. -->
    <key>ExitTimeOut</key>
    <integer>300</integer>
    <!-- vmlauncher takes a single argument: the absolute path to the vmx file (not the vmwarevm bundle). -->
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/vmlauncher</string>
        <string>/Volumes/Virtual Machines/Testing.vmwarevm/Testing.vmx</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
</dict>
</plist>
```

##License

vmlauncher is released under the zlib license. To see the exact details, look at the `LICENSE` file.
