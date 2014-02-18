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
&lt;?xml version="1.0" encoding="UTF-8"?&gt;
&lt;!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"&gt;
&lt;plist version="1.0"&gt;
&lt;dict&gt;
    &lt;key&gt;Label&lt;/key&gt;
    &lt;string&gt;com.example.vmlauncher&lt;/string&gt;
    &lt;!-- The virtual machine may take some time to suspend. Setting this value too low could lead to VMWare being forcibly killed during suspending. --&gt;
    &lt;key&gt;ExitTimeOut&lt;/key&gt;
    &lt;integer&gt;300&lt;/integer&gt;
    &lt;!-- vmlauncher takes a single argument: the absolute path to the vmx file (not the vmwarevm bundle). --&gt;
    &lt;key&gt;ProgramArguments&lt;/key&gt;
    &lt;array&gt;
        &lt;string&gt;/usr/local/bin/vmlauncher&lt;/string&gt;
        &lt;string&gt;/Volumes/Virtual Machines/Testing.vmwarevm/Testing.vmx&lt;/string&gt;
    &lt;/array&gt;
    &lt;key&gt;RunAtLoad&lt;/key&gt;
    &lt;true/&gt;
&lt;/dict&gt;
&lt;/plist&gt;
```

##License

vmlauncher is released under the zlib license. To see the exact details, look at the `LICENSE` file.
