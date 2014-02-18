# This is the path to VMWare's "Public" directory. If your VMWare installation
# is different, adjust this.
VMWARE=/Applications/VMware\ Fusion.app/Contents/Public/
CFLAGS=-Wall -Wextra -Werror -isystem$(VMWARE)/include/

# The install name for libvixAllProducts.dylib is simply 'libvixAllProducts.dylib',
# so use DYLD_FALLBACK_LIBRARY_PATH to hook things up. This probably could be
# solved by rewriting the install name, but myeh.
LDFLAGS=$(VMWARE)/libvixAllProducts.dylib -Xlinker -dyld_env -Xlinker DYLD_FALLBACK_LIBRARY_PATH=$(VMWARE)

vmlauncher: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o vmlauncher main.c

all: vmlauncher

install: vmlauncher
	mkdir -p /usr/local/bin/
	cp vmlauncher /usr/local/bin/vmlauncher

clean:
	rm vmlauncher
