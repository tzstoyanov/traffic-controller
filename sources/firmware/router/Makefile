
#export MIPS_CROSS=1

ifdef MIPS_CROSS
#Path to where I have the toolchain
TOOLCHAIN_PATH=/home/workspace/TL/tools/toolchain-mipsel_4.1.1_BRCM24/
TOOLCHAIN_BIN_PATH=$(TOOLCHAIN_PATH)bin/
TOOLCHAIN_LIB_PATH=$(TOOLCHAIN_PATH)lib/

AR=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-ar
AS=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-as
LD=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-ld
NM=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-nm
CC=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-gcc
CPP=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-cpp
GCC=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-gcc
CXX=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-g++
RANLIB=$(TOOLCHAIN_BIN_PATH)mipsel-linux-uclibc-ranlib
ac_cv_linux_vers=2.4.37

#Where the libs for current toolchain are
LDFLAGS="-L$(TOOLCHAIN_LIB_PATH)"

#Where the include files for current toolchain are
#Trying with different compiler flags
CFLAGS="-I$(TOOLCHAIN_PATH)include -Os -pipe -mips32 -mtune=mips32 -funit-at-a-time"
#CFLAGS="-I$(TOOLCHAIN_PATH)include -D__KERNEL__ -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fno-builtin-strpbrk -fno-builtin-sprintf -fomit-frame-pointer  -funit-at-a-time  -fno-delayed-branch -mno-abicalls -pipe -DMODULE -mlong-calls"

else

CC=gcc 
CFLAGS = -Wall -Werror -g -I.

endif

APP_NAME = tlight
APP_FILES = linux_thread linux_sem tl_main tl_controlers \
			tl_program tl_program_cfg tl_api tl_serial \
			lib_tl_log lib_tl_config lib_tl_com lib_tl_packets
			
CFG_NAME = mgmt_tlight
CFG_FILES = lib_tl_log mgmt_tl_main mgmt_tl_client \
			mgmt_tl_debug mgmt_tl_status mgmt_tl_program

SIM_NAME = sim_tlight
SIM_FILES = linux_thread sim_tl_main sim_tl_packets sim_tl_controlers \
			 lib_tl_log lib_tl_config lib_tl_com lib_tl_packets
			
APP_OBJ = $(addsuffix .o, $(APP_FILES))
CFG_OBJ = $(addsuffix .o, $(CFG_FILES)) 
SIM_OBJ = $(addsuffix .o, $(SIM_FILES)) 

all: $(APP_NAME) $(CFG_NAME) $(SIM_NAME)

$(APP_NAME): $(APP_OBJ)
	$(CC) -o $(APP_NAME) $(APP_OBJ)

$(CFG_NAME): $(CFG_OBJ)
	$(CC) -o $(CFG_NAME) $(CFG_OBJ)

$(SIM_NAME): $(SIM_OBJ)
	$(CC) -o $(SIM_NAME) $(SIM_OBJ)

clean:
	rm -f $(APP_NAME) $(CFG_NAME) $(SIM_NAME) *.o *~




