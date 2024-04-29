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
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(tifin, tag, &v1, &v2)) TIFFSetField(tifout, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(tifin, tag, &v1, &v2, &v3)) TIFFSetField(tifout, tag, v1, v2, v3)

void emitCassiniInfo(TIFF* tifout) {
    const char* description = "Carte Générale de la France";
    const char* software = "";
    const char* copyright = "©2024 Frédéric Delhoume delhoume@gmail.com";
	const char* datetime = "2024:04:12 00:00:00";
	const char* artist = "César-François Cassini";
    TIFFSetField(tifout, TIFFTAG_IMAGEDESCRIPTION, description);
    TIFFSetField(tifout, TIFFTAG_SOFTWARE, software);
    TIFFSetField(tifout, TIFFTAG_COPYRIGHT, copyright);
    TIFFSetField(tifout, TIFFTAG_DATETIME, datetime);  
	TIFFSetField(tifout, TIFFTAG_ARTIST, artist);
 
}


void emitNormalInfo(TIFF* tifout) {
   const char* description = "Carte Générale de la France";
    const char* software = "Very Large Image Viewer";
    const char* copyright = "©2024 Frédéric Delhoume delhoume@gmail.com";
  TIFFSetField(tifout, TIFFTAG_IMAGEDESCRIPTION, description);
    TIFFSetField(tifout, TIFFTAG_SOFTWARE, software);
    TIFFSetField(tifout, TIFFTAG_COPYRIGHT, copyright);

}


int main(int argc, char* argv[]) {
	TIFFSetWarningHandler(0);
	
	TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
	TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0);// unlimited
    TIFF* tifout = TIFFOpenExt(argv[argc - 1], "w8", opts);
	emitCassiniInfo(tifout);
//	emitNormalInfo(tifout);
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
	  uint16_t bitspersample, samplesperpixel, compression, photometric, shortv, *shortav;
      unsigned int x, y;
      const char* current_file = argv[idx];
      TIFF* tifin = TIFFOpenExt(argv[idx], "rm", opts);
	  if (!tifin) {
		std::cout << "failed to open current file: " << current_file << endl;
		continue;
	  }
        std::cout << "current file: " << current_file << " (tilesize: " << TIFFTileSize(tifin) << ")" << endl;
   tdata_t tilebuf = _TIFFmalloc(TIFFTileSize(tifin) * 2);
 	  
	CopyField(TIFFTAG_TILEWIDTH, tilewidth);
	CopyField(TIFFTAG_TILELENGTH, tileheight);
	CopyField(TIFFTAG_IMAGEWIDTH, imagewidth);
	CopyField(TIFFTAG_IMAGELENGTH, imageheight);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	CopyField(TIFFTAG_COMPRESSION, compression);

    CopyField(TIFFTAG_PHOTOMETRIC, photometric);
	CopyField2(TIFFTAG_EXTRASAMPLES, shortv, shortav);
	CopyField(TIFFTAG_PREDICTOR, shortv);
	CopyField(TIFFTAG_THRESHHOLDING, shortv);
	CopyField(TIFFTAG_FILLORDER, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	CopyField(TIFFTAG_MINSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_MAXSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_PLANARCONFIG, shortv);

	const ttile_t numtiles = TIFFNumberOfTiles(tifin);

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
			 TIFFSetField(tifout, TIFFTAG_JPEGTABLESMODE, 0);
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

	for (unsigned int tidx = 0; tidx < numtiles; ++tidx) {
		//	tsize_t numbytes = TIFFReadRawTile(tifin, tidx, tilebuf, TIFFTileSize(tifin));
			tsize_t numbytes = TIFFReadEncodedTile(tifin, tidx, tilebuf, TIFFTileSize(tifin));
			if (numbytes == -1) 
					std::cout << "Error for " << x << ":" << y << endl;
			if (numbytes > TIFFTileSize(tifin))
					std::cout << "Readbytes " << numbytes <<  " at " << tidx << " is larger than " << TIFFTileSize(tifin) << endl;
//			TIFFWriteRawTile(tifout, tidx, tilebuf, numbytes);
			TIFFWriteEncodedTile(tifout, tidx, tilebuf, numbytes);
		if ((tidx > 0) && (tidx % numtilesx) == 0) {
			std::cout << "   row " << ((tidx / numtilesy) + 1) << "/" << (numtiles / numtilesy) << "                \r" ;
			std::cout.flush();
		}
	}

      TIFFWriteDirectory(tifout);
      TIFFClose(tifin);
      _TIFFfree(tilebuf);    
	}    
	TIFFClose(tifout);
	TIFFOpenOptionsFree(opts);
    std::cout << endl << "Done" << endl;
	return 0;
}
