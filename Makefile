BINDIR = bin
SRCDIR = src

CCC=/opt/homebrew/bin/g++-14


CCFLAGS_DEBUG= -g
CCFLAGS_OPTIM = -O2

CCFLAGS =  -std=c++11 $(CCFLAGS_DEBUG) -I /opt/homebrew/include -I src

FOLDER = "/Volumes/My Book/BigMars"
BARRY_FOLDER = "/Volumes/My Book/CinemaRedux"
JACOTIN_FOLDER = "/Users/fredericdelhoume/Downloads/Egypte"

LIBS += -L /opt/homebrew/lib -ltiff -lturbojpeg -lz-ng -lz -lzstd -llzma


PROGRAMS = $(BINDIR)/check_all $(BINDIR)/buildmarsimagetiled $(BINDIR)/buildmarsimagetiled $(BINDIR)/halftiff_stb $(BINDIR)/strip2tiled.zip $(BINDIR)/strip2tiled.jpg \
	$(BINDIR)/tiffmerge.first $(BINDIR)/pyramid2deepzoom $(BINDIR)/check_full $(BINDIR)/build_barry_lyndon $(BINDIR)/build_jacotin_pages $(BINDIR)/strips2strip \
	$(BINDIR)/build_invader_paris $(BINDIR)/build_mosaic


all:  $(PROGRAMS) $(BINDIR)

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

$(BINDIR)/buildmarsimagetiled: $(SRCDIR)/buildmarsimagetiled.cpp 
	$(CCC) $(CCFLAGS) -DDEFAULT_FOLDER=\"$(FOLDER)\" $(SRCDIR)/buildmarsimagetiled.cpp -o $(BINDIR)/buildmarsimagetiled  $(LIBS)

$(BINDIR)/halftiff_stb: $(SRCDIR)/halftiff_stb.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/halftiff_stb.cpp -o $(BINDIR)/halftiff_stb  $(LIBS)

$(BINDIR)/strip2tiled.zip: $(SRCDIR)/strip2tiled.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) $(SRCDIR)/strip2tiled.cpp -o $(BINDIR)/strip2tiled.zip  $(LIBS)

$(BINDIR)/strip2tiled.jpg: $(SRCDIR)/strip2tiled.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) -DUSE_JPEG $(SRCDIR)/strip2tiled.cpp -o $(BINDIR)/strip2tiled.jpg  $(LIBS)

$(BINDIR)/strips2strip: $(SRCDIR)/strips2strip.cpp
	$(CCC) $(CCFLAGS) -DNTHREADS=$(THREADS) $(SRCDIR)/strips2strip.cpp -o $(BINDIR)/strips2strip  $(LIBS)

$(BINDIR)/tiffmerge.first: $(SRCDIR)/tiffmerge.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/tiffmerge.cpp -o $(BINDIR)/tiffmerge.first $(LIBS)

$(BINDIR)/pyramid2deepzoom: $(SRCDIR)/pyramid2deepzoom.cpp
	$(CCC) $(CCFLAGS) $(SRCDIR)/pyramid2deepzoom.cpp -o $(BINDIR)/pyramid2deepzoom $(LIBS)


$(BINDIR)/build_barry_lyndon: $(SRCDIR)/build_barry_lyndon.cpp 
	$(CCC) $(CCFLAGS) -DDEFAULT_FOLDER=\"$(BARRY_FOLDER)\" $(SRCDIR)/build_barry_lyndon.cpp -o $(BINDIR)/build_barry_lyndon  $(LIBS)

$(BINDIR)/build_jacotin_pages: $(SRCDIR)/build_jacotin_pages.cpp 
	$(CCC) $(CCFLAGS) -DDEFAULT_FOLDER=\"$(JACOTIN_FOLDER)\" $(SRCDIR)/build_jacotin_pages.cpp -o $(BINDIR)/build_jacotin_pages  $(LIBS)


$(BINDIR)/build_invader_paris: $(SRCDIR)/build_invader_paris.cpp 
	$(CCC) $(CCFLAGS) $(SRCDIR)/build_invader_paris.cpp -o $(BINDIR)/build_invader_paris  $(LIBS)

$(BINDIR)/build_mosaic: $(SRCDIR)/build_mosaic.cpp 
	$(CCC) $(CCFLAGS) $(SRCDIR)/build_mosaic.cpp -o $(BINDIR)/build_mosaic  $(LIBS)
