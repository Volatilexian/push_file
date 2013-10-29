CC = /home/volatile_xian/openwrt_12_09/attitude_adjustment/bin/x86/OpenWrt-SDK-x86-for-linux-i686-gcc-4.6-linaro_uClibc-0.9.33.2/staging_dir/toolchain-i386_gcc-4.6-linaro_uClibc-0.9.33.2/bin/i486-openwrt-linux-gcc
CFLAGS = -g -Wall
LIBS = -lsqlite3 -lm
LIBPATH = /home/volatile_xian/openwrt_12_09/attitude_adjustment/bin/x86/OpenWrt-SDK-x86-for-linux-i686-gcc-4.6-linaro_uClibc-0.9.33.2/staging_dir/target-i386_uClibc-0.9.33.2/usr/lib/

push : push.c
	gcc $^ -o $@ $(CFLAGS) $(LIBS)
pull : pull.c
	gcc $^ -o $@ $(CFLAGS) $(LIBS)
	#$(CC) $^ -o $@ $(CFLAGS) -L$(LIBPATH) $(LIBS)
