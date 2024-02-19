BINDIR = bin
SRCDIR = src

CCC=/opt/homebrew/bin/g++-13

CCFLAGS = -std=c++11 -O3 -fopenmp -I /opt/homebrew/include -I src

THREADS=8

FOLDER = "/Volumes/My Book/BigMars"

LIBS = /opt/homebrew/lib/libtiff.a  /opt/homebrew/lib/libturbojpeg.a \
    /opt/homebrew/lib/libz-ng.a /opt/homebrew/lib/libzlibstatic.a /opt/homebrew/lib/libzstd.a /opt/homebrew/lib/liblzma.a  -l gomp

PROGRAMS = $(BINDIR)/check_all $(BINDIR)/buildmarsimage $(BINDIR)/halftiff_stb $(BINDIR)/strip2tiled.zip $(BINDIR)/strip2tiled.jpg \
	$(BINDIR)/tiffmerge.first $(BINDIR)/pyramid2deepzoom $(BINDIR)/check_full


all:  $(PROGRAMS)

clean:
	rm -rf *~ *# .??*
	rm -rf $(SRCDIR)*~ $(SRCDIR)*# $(SRCDIR).??*
	rm -rf $(PROGRAMS)

$(BINDIR):
	mkdir -p bin

$(BINDIR)/check_all: $(SRCDIR)/check_all.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/check_all.cpp -o $(BINDIR)/check_all $(LIBS)

$(BINDIR)/check_full: $(SRCDIR)/check_full.cpp 
	$(CCC) $(CCFLAGS) $(SRCDIR)/check_full.cpp -o $(BINDIR)/check_full $(LIBS)

$(BINDIR)/buildmarsimage: $(SRCDIR)/buildmarsimage.cpp 
	$(CCC) $(CCFLAGS) -DDEFAULT_FOLDER=\"$(FOLDER)\" $(SRCDIR)/buildmarsimage.cpp -o $(BINDIR)/buildmarsimage  $(LIBS)

$(BINDIR)/halftiff_stb: $(SRCDIR)/halftiff_stb.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/halftiff_stb.cpp -o $(BINDIR)/halftiff_stb  $(LIBS)

$(BINDIR)/strip2tiled.jpg: $(SRCDIR)/strip2tiled.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) -DUSE_JPEG $(SRCDIR)/strip2tiled.cpp -o $(BINDIR)/strip2tiled.jpg  $(LIBS)

$(BINDIR)/strip2tiled.zip: $(SRCDIR)/strip2tiled.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) $(SRCDIR)/strip2tiled.cpp -o $(BINDIR)/strip2tiled.zip  $(LIBS)

$(BINDIR)/tiffmerge.first: $(SRCDIR)/tiffmerge.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/tiffmerge.cpp -o $(BINDIR)/tiffmerge.first $(LIBS)

$(BINDIR)/pyramid2deepzoom: $(SRCDIR)/pyramid2deepzoom.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/pyramid2deepzoom.cpp -o $(BINDIR)/pyramid2deepzoom $(LIBS)
