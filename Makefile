BINDIR = bin
SRCDIR = src

CCC=/opt/homebrew/bin/g++-13

CCFLAGS = -std=c++11 -O3 -fopenmp -I /opt/homebrew/include -I src


THREADS=8

LIBS = /opt/homebrew/lib/libtiff.a  /opt/homebrew/lib/libturbojpeg.a \
    /opt/homebrew/lib/libz-ng.a /opt/homebrew/lib/libzlibstatic.a /opt/homebrew/lib/libzstd.a /opt/homebrew/lib/liblzma.a  -l gomp

PROGRAMS = $(BINDIR)/strips2strip $(BINDIR)/buildmarsimage $(BINDIR)/halftiff_stb $(BINDIR)/tiffmerge.last $(BINDIR)/tiffmerge.first  \
               $(BINDIR)/strip2tiled.jpg $(BINDIR)/strip2tiled.zip $(BINDIR)/check_all $(BINDIR)/check_full \
			   $(BINDIR)/extract_deflate_streams 

all: $(PROGRAMS)

clean:
	rm -rf *~ *# .??*
	rm -rf $(SRCDIR)*~ $(SRCDIR)*# $(SRCDIR).??*
	rm -rf $(PROGRAMS)

$(BINDIR)/strips2strip: $(SRCDIR)/strips2strip.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/strips2strip.cpp -o $(BINDIR)/strips2strip $(LIBS)

$(BINDIR)/buildmarsimage: $(SRCDIR)/buildmarsimage.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/buildmarsimage.cpp -o $(BINDIR)/buildmarsimage  $(LIBS)

$(BINDIR)/buildmarsimage_noinflate: $(SRCDIR)/buildmarsimage_noinflate.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/buildmarsimage_noinflate.cpp -o $(BINDIR)/buildmarsimage_noinflate  $(LIBS)

$(BINDIR)/halftiff_stb: $(SRCDIR)/halftiff_stb.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/halftiff_stb.cpp -o $(BINDIR)/halftiff_stb  $(LIBS)

$(BINDIR)/strip2tiled.jpg: $(SRCDIR)/strip2tiled.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) -DUSE_JPEG $(SRCDIR)/strip2tiled.cpp -o $(BINDIR)/strip2tiled.jpg  $(LIBS)

$(BINDIR)/strip2tiled.zip: $(SRCDIR)/strip2tiled.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) $(SRCDIR)/strip2tiled.cpp -o $(BINDIR)/strip2tiled.zip  $(LIBS)

$(BINDIR)/tiffmerge.first: $(SRCDIR)/tiffmerge.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/tiffmerge.cpp -o $(BINDIR)/tiffmerge.first $(LIBS)

$(BINDIR)/tiffmerge.last: $(SRCDIR)/tiffmerge.cpp
	$(CCC) $(CCFLAGS) -DFULL_RES_LAST $(SRCDIR)/tiffmerge.cpp -o $(BINDIR)/tiffmerge.last $(LIBS)

$(BINDIR)/check_all: $(SRCDIR)/check_all.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/check_all.cpp -o $(BINDIR)/check_all $(LIBS)

$(BINDIR)/check_full: $(SRCDIR)/check_full.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/check_full.cpp -o $(BINDIR)/check_full $(LIBS)

$(BINDIR)/extract_deflate_streams: $(SRCDIR)/extract_deflate_streams.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/extract_deflate_streams.cpp -o $(BINDIR)/extract_deflate_streams $(LIBS)

