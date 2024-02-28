/* 
 64 bit
 cl /EHsc /O2 /MD /nologo /Iinclude /Iinclude\tiff-4.3.0\libtiff tiffmerge.cpp libs64\libtiff.lib libs64\zlibstatic.lib libs64\zlibstatic-ng.lib libs64\turbojpeg-static.lib libs64\libwebp.lib
*/

#include <iostream>
#include <cstring>
using namespace std;

extern "C" {
#include <tiffio.h>
}

#define	CopyField(tag, v) \
    if (TIFFGetField(tifin, tag, &v)) TIFFSetField(tifout, tag, v)


int main(int argc, char* argv[]) {
	TIFFSetWarningHandler(0);
 
    TIFF* tifout = TIFFOpen(argv[argc - 1], "w8");
    const char* description = "";
    const char* software = "Very Large Image Viewer";
    const char* copyright = "Frederic Delhoume delhoume@gmail.com";
    TIFFSetField(tifout, TIFFTAG_IMAGEDESCRIPTION, description);
    TIFFSetField(tifout, TIFFTAG_SOFTWARE, software);
    TIFFSetField(tifout, TIFFTAG_COPYRIGHT, copyright);
    std::cout << "Creating multiple image file..." << endl
	 << "destination file: " << argv[argc - 1] << endl;
 	unsigned int fullresfirst = true;
#if defined(FULL_RES_LAST)
	fullresfirst = false;
#endif

	unsigned int nimages = argc - 1;
    for (unsigned int idx = 1; idx < nimages; ++idx) {
      uint32_t imagewidth, imageheight;
      uint32_t tilewidth, tileheight;
	  uint16_t bitspersample, samplesperpixel, compression, photometric, shortv;
      unsigned int x, y;
      const char* current_file = argv[idx];
      TIFF* tifin = TIFFOpen(argv[idx], "rm");
	  if (!tifin) {
		std::cout << "failed to open current file: " << current_file << endl;
		continue;
	  }
      tdata_t tilebuf = _TIFFmalloc(TIFFTileSize(tifin));
      std::cout << "current file: " << current_file << " (tilesize: " << TIFFTileSize(tifin) << ")" << endl;
	  
	CopyField(TIFFTAG_TILEWIDTH, tilewidth);
	CopyField(TIFFTAG_TILELENGTH, tileheight);
	CopyField(TIFFTAG_IMAGEWIDTH, imagewidth);
	CopyField(TIFFTAG_IMAGELENGTH, imageheight);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	CopyField(TIFFTAG_COMPRESSION, compression);

    CopyField(TIFFTAG_PHOTOMETRIC, photometric);
	CopyField(TIFFTAG_EXTRASAMPLES, shortv);
	CopyField(TIFFTAG_PREDICTOR, shortv);
	CopyField(TIFFTAG_THRESHHOLDING, shortv);
	CopyField(TIFFTAG_FILLORDER, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	CopyField(TIFFTAG_MINSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_MAXSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_PLANARCONFIG, shortv);

	if (samplesperpixel == 4 && photometric == PHOTOMETRIC_RGB) {
		// transparency mask support
		uint16_t sampleinfo[1]; 
		// unassociated alpha data is transparency information
		sampleinfo[0] = EXTRASAMPLE_UNASSALPHA;
		TIFFSetField(tifout, TIFFTAG_EXTRASAMPLES, 1, sampleinfo);
	 }
	unsigned int numtilesx = imagewidth / tilewidth;
	if (imagewidth % tilewidth)
		++numtilesx;

	unsigned int numtilesy = imageheight / tileheight;
	if (imageheight % tileheight)
		++numtilesy;

	 std::cout << "size: " << imagewidth << "x" << imageheight << endl;
	 std::cout << "tiles size: " << tilewidth << "x" << tileheight << endl;	 
	 std::cout << "tiles: " << numtilesx << "x" << numtilesy << endl;
	 std::cout << "image tiled size: " << (numtilesx * tilewidth) << " x " << (numtilesy * tileheight)<< std::endl;
 
	  std::cout << idx << " ";

      if (((idx == 1) && fullresfirst) || ((idx == nimages) && !fullresfirst)) {
		std::cout << "full image" << endl;
 	} else {
 		  std::cout << "reduced image" << endl;
		  TIFFSetField(tifout, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
    }
      switch (compression) {
		  case COMPRESSION_JPEG: {
			  uint32_t count;
			  void* table = NULL;
			  if (TIFFGetField(tifin, TIFFTAG_JPEGTABLES, &count, &table) && (count > 0) && table) {
				TIFFSetField(tifout, TIFFTAG_JPEGTABLES, count, table);
			  }
			  std::cout << "jpeg compression" << endl;
			  break;
		  }
		  case COMPRESSION_DEFLATE:
		  case COMPRESSION_ADOBE_DEFLATE: {
			  std::cout << "deflate compression" << endl;
		  }
		  break;
	}

#if 1
	for (unsigned int y = 0; y < numtilesy; ++y) {
		for (unsigned int x = 0; x < numtilesx; ++x) {
			uint32_t tilenum = TIFFComputeTile(tifin, x * tilewidth, y * tileheight, 0, 0);
		//	tsize_t numbytes = TIFFReadRawTile(tifin, tilenum, tilebuf, TIFFTileSize(tifin));
			tsize_t numbytes = TIFFReadEncodedTile(tifin, tilenum, tilebuf, TIFFTileSize(tifin));
			if (numbytes == -1) 
					std::cout << "Error for " << x << ":" << y << endl;
			if (numbytes > TIFFTileSize(tifin))
					std::cout << "Readbytes " << numbytes <<  " at " << x << "x" << y  << "is larger than " << TIFFTileSize(tifin) << endl;
			tilenum = TIFFComputeTile(tifout, x * tilewidth, y * tileheight, 0, 0);
			//TIFFWriteRawTile(tifout, tilenum, tilebuf, numbytes);
			TIFFWriteEncodedTile(tifout, tilenum, tilebuf, numbytes);
		}
		std::cout << "   row " << (y + 1) << "/" << numtilesy << "                \r" ;
		std::cout.flush();
	}
#else
	unsigned int current_row = 0;
	unsigned int numrows = imageheight / tileheight;
	if (imageheight % tileheight) numrows++;
      for(y = 0; y < imageheight; y += tileheight, current_row++) {
		  std::cout << "   row " << (current_row + 1) << "/" << numrows << "                \r" ;
		  std::cout.flush();
		  for (x = 0; x < imagewidth; x += tilewidth) {
			  tsize_t numbytes = TIFFReadRawTile(tifin, 
							 TIFFComputeTile(tifin, x, y, 0, 0), 
							 tilebuf, 
							 TIFFTileSize(tifin));
			if (numbytes == -1) 
  					std::cout << "Error for " << x << ":" << y << endl;
			if (numbytes > TIFFTileSize(tifin))
					std::cout << "Readbytes " << numbytes <<  " at " << x << "x" << y  << "is larger than " << TIFFTileSize(tifin) << endl;
			  TIFFWriteRawTile(tifout, TIFFComputeTile(tifout, x, y, 0, 0), tilebuf, numbytes);
		  }
      }
#endif
      TIFFWriteDirectory(tifout);
      TIFFClose(tifin);
      _TIFFfree(tilebuf);    
	}    
	TIFFClose(tifout);
    std::cout << endl << "Done" << endl;
	return 0;
}
