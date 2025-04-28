SHELL = /bin/sh

CPPFLAGS=-fPIC -O3 -Ipugl/include -Iglad/include `pkg-config --cflags freetype2`

BUILDDIR ?= build

PUGL_C_FILES = pugl/src/common.c pugl/src/internal.c \
    pugl/src/x11.c pugl/src/x11_gl.c pugl/src/x11_stub.c
    
# archive
AR ?= ar
ARFLAGS = rcs
    
all: SignalView.lv2

$(BUILDDIR)/libpugl.a: $(shell find pugl/)
	mkdir -p $(@D)
	mkdir -p $@.tmp
	cd $@.tmp ; gcc -I$(CURDIR)/pugl/include `pkg-config --cflags x11` $(addprefix $(CURDIR)/, $(PUGL_C_FILES)) -c
	$(AR) $(ARFLAGS) $@ $@.tmp/*.o
	rm -rf $@.tmp

SignalView.so: SignalView.o
	g++ -shared -o SignalView.so SignalView.o

SignalView.o: SignalView.cpp SignalView.h uris.h

SignalViewUI.so: SignalViewUI.o Font.o Grid.o LGraph.o Shader.o Spectrum.o Waterfall.o $(BUILDDIR)/libpugl.a
	g++ -Wall -Wextra -shared -fPIC -o SignalViewUI.so SignalViewUI.o Font.o Grid.o LGraph.o Shader.o Spectrum.o Waterfall.o \
	 -L$(BUILDDIR) -lpugl `pkg-config --libs x11 xext xcursor xrandr glx fftw3 freetype2`

SignalViewUI.o: SignalViewUI.cpp

SignalView.lv2: SignalViewUI.so SignalView.so
	mkdir SignalView.lv2
	cp SignalView.so SignalView.lv2
	cp SignalViewUI.so SignalView.lv2
	cp manifest.ttl SignalView.lv2
	cp SignalView.ttl SignalView.lv2
	cp "sui generis rg.otf" SignalView.lv2
	cp -u -r SignalView.lv2 ~/.lv2
	zip -r $(BUILDDIR)/SignalView.lv2.zip SignalView.lv2/*
	rm -rf SignalView.lv2

Font.o: Font.cpp

Grid.o: Grid.cpp

LGraph.o: LGraph.cpp

Shader.o: Shader.cpp

Spectrum.o: Spectrum.cpp

Waterfall.o: Waterfall.cpp


