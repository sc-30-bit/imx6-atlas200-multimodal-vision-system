CGI_SRC_DIR   = cgi-bin
WWW_DIR       = www
DB_DIR        = database

CXX           = arm-poky-linux-gnueabi-g++
PKG_CXXFLAGS  = -std=c++11 -Wall
PKG_LDFLAGS   = -lsqlite3

ARCH_FLAGS    = -march=armv7-a -mthumb-interwork -mfloat-abi=hard -mfpu=neon -mtune=cortex-a9

ifdef SDKTARGETSYSROOT
  SYSROOT_FLAGS = --sysroot=$(SDKTARGETSYSROOT)
endif

.PHONY: all clean strip install distclean

all: $(CGI_SRC_DIR)/analyze.cgi $(CGI_SRC_DIR)/history.cgi $(CGI_SRC_DIR)/dotmatrix.cgi
	@echo "=== Build completed ==="
	@file $(CGI_SRC_DIR)/analyze.cgi 2>/dev/null || true
	@ls -lh $(CGI_SRC_DIR)/*.cgi

$(CGI_SRC_DIR)/analyze.cgi: $(CGI_SRC_DIR)/analyze.cpp $(CGI_SRC_DIR)/db.h $(CGI_SRC_DIR)/utils.h
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(SYSROOT_FLAGS) $(PKG_CXXFLAGS) -o $@ $(CGI_SRC_DIR)/analyze.cpp $(LDFLAGS) $(SYSROOT_FLAGS) $(PKG_LDFLAGS) -lpthread

$(CGI_SRC_DIR)/history.cgi: $(CGI_SRC_DIR)/history.cpp $(CGI_SRC_DIR)/db.h $(CGI_SRC_DIR)/utils.h
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(SYSROOT_FLAGS) $(PKG_CXXFLAGS) -o $@ $(CGI_SRC_DIR)/history.cpp $(LDFLAGS) $(SYSROOT_FLAGS) $(PKG_LDFLAGS)

$(CGI_SRC_DIR)/dotmatrix.cgi: $(CGI_SRC_DIR)/dotmatrix.cpp
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(SYSROOT_FLAGS) $(PKG_CXXFLAGS) -o $@ $(CGI_SRC_DIR)/dotmatrix.cpp $(LDFLAGS) $(SYSROOT_FLAGS)

strip:
	arm-poky-linux-gnueabi-strip $(CGI_SRC_DIR)/analyze.cgi $(CGI_SRC_DIR)/history.cgi $(CGI_SRC_DIR)/dotmatrix.cgi 2>/dev/null || strip $(CGI_SRC_DIR)/analyze.cgi $(CGI_SRC_DIR)/history.cgi $(CGI_SRC_DIR)/dotmatrix.cgi 2>/dev/null || true
	@echo "=== Strip completed ==="
	@ls -lh $(CGI_SRC_DIR)/*.cgi

install:
	@chmod +x $(CGI_SRC_DIR)/analyze.cgi $(CGI_SRC_DIR)/history.cgi 2>/dev/null || true
	@chmod 755 $(DB_DIR) 2>/dev/null || true
	@echo "=== Install completed ==="

clean:
	rm -f $(CGI_SRC_DIR)/analyze.cgi $(CGI_SRC_DIR)/history.cgi $(CGI_SRC_DIR)/dotmatrix.cgi
	@echo "=== Clean completed (source files preserved) ==="

distclean: clean
	rm -f $(DB_DIR)/analysis.db
