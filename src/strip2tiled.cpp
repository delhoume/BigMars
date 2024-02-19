#include <tiffio.h>
#include <zlib.h>

#include <iostream>
using namespace std;


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

bool debug = false;

unsigned int tilewidth = 512;
unsigned int tileheight = 512;

#define CopyField(tag, v)             \
	if (TIFFGetField(tifin, tag, &v)) \
	TIFFSetField(tifout, tag, v)
#define CopyField2(tag, v1, v2)             \
	if (TIFFGetField(tifin, tag, &v1, &v2)) \
	TIFFSetField(tifout, tag, v1, v2)
#define CopyField3(tag, v1, v2, v3)              \
	if (TIFFGetField(tifin, tag, &v1, &v2, &v3)) \
	TIFFSetField(tifout, tag, v1, v2, v3)
#define CopyField4(tag, v1, v2, v3, v4)               \
	if (TIFFGetField(tifin, tag, &v1, &v2, &v3, &v4)) \
	TIFFSetField(tifout, tag, v1, v2, v3, v4)

static void
cpTag(TIFF *tifin, TIFF *tifout, uint16_t tag, uint16_t count, TIFFDataType type)
{
	switch (type)
	{
	case TIFF_SHORT:
		if (count == 1)
		{
			uint16_t shortv;
			CopyField(tag, shortv);
		}
		else if (count == 2)
		{
			uint16_t shortv1, shortv2;
			CopyField2(tag, shortv1, shortv2);
		}
		else if (count == 4)
		{
			uint16_t *tr, *tg, *tb, *ta;
			CopyField4(tag, tr, tg, tb, ta);
		}
		else if (count == (uint16_t)-1)
		{
			uint16_t shortv1;
			uint16_t *shortav;
			CopyField2(tag, shortv1, shortav);
		}
		break;
	case TIFF_LONG:
	{
		uint32_t longv;
		CopyField(tag, longv);
	}
	break;
	case TIFF_RATIONAL:
		if (count == 1) {
			float floatv;
			CopyField(tag, floatv);
		}
		else if (count == (uint16_t)-1) {
			float *floatav;
			CopyField(tag, floatav);
		}
		break;
	case TIFF_ASCII: {
		char *stringv;
		CopyField(tag, stringv);
	}
	break;
	case TIFF_DOUBLE:
		if (count == 1)
		{
			double doublev;
			CopyField(tag, doublev);
		}
		else if (count == (uint16_t)-1)
		{
			double *doubleav;
			CopyField(tag, doubleav);
		}
		break;
	default:
		TIFFError(TIFFFileName(tifin),
				  "Data type %\"PRIu16\" is not supported, tag %d skipped.",
				  tag, type);
	}
}

static const struct cpTag
{
	uint16_t tag;
	uint16_t count;
	TIFFDataType type;
} tags[] = {
	{TIFFTAG_SUBFILETYPE, 1, TIFF_LONG},
	{TIFFTAG_THRESHHOLDING, 1, TIFF_SHORT},
	{TIFFTAG_DOCUMENTNAME, 1, TIFF_ASCII},
	{TIFFTAG_IMAGEDESCRIPTION, 1, TIFF_ASCII},
	{TIFFTAG_MAKE, 1, TIFF_ASCII},
	{TIFFTAG_MODEL, 1, TIFF_ASCII},
	{TIFFTAG_MINSAMPLEVALUE, 1, TIFF_SHORT},
	{TIFFTAG_MAXSAMPLEVALUE, 1, TIFF_SHORT},
	{TIFFTAG_XRESOLUTION, 1, TIFF_RATIONAL},
	{TIFFTAG_YRESOLUTION, 1, TIFF_RATIONAL},
	{TIFFTAG_PAGENAME, 1, TIFF_ASCII},
	{TIFFTAG_XPOSITION, 1, TIFF_RATIONAL},
	{TIFFTAG_YPOSITION, 1, TIFF_RATIONAL},
	{TIFFTAG_RESOLUTIONUNIT, 1, TIFF_SHORT},
	{TIFFTAG_SOFTWARE, 1, TIFF_ASCII},
	{TIFFTAG_DATETIME, 1, TIFF_ASCII},
	{TIFFTAG_ARTIST, 1, TIFF_ASCII},
	{TIFFTAG_HOSTCOMPUTER, 1, TIFF_ASCII},
	{TIFFTAG_WHITEPOINT, (uint16_t)-1, TIFF_RATIONAL},
	{TIFFTAG_PRIMARYCHROMATICITIES, (uint16_t)-1, TIFF_RATIONAL},
	{TIFFTAG_HALFTONEHINTS, 2, TIFF_SHORT},
	{TIFFTAG_INKSET, 1, TIFF_SHORT},
	{TIFFTAG_DOTRANGE, 2, TIFF_SHORT},
	{TIFFTAG_TARGETPRINTER, 1, TIFF_ASCII},
	{TIFFTAG_SAMPLEFORMAT, 1, TIFF_SHORT},
	{TIFFTAG_YCBCRCOEFFICIENTS, (uint16_t)-1, TIFF_RATIONAL},
	{TIFFTAG_YCBCRSUBSAMPLING, 2, TIFF_SHORT},
	{TIFFTAG_YCBCRPOSITIONING, 1, TIFF_SHORT},
	{TIFFTAG_REFERENCEBLACKWHITE, (uint16_t)-1, TIFF_RATIONAL},
	{TIFFTAG_EXTRASAMPLES, (uint16_t)-1, TIFF_SHORT},
	{TIFFTAG_SMINSAMPLEVALUE, 1, TIFF_DOUBLE},
	{TIFFTAG_SMAXSAMPLEVALUE, 1, TIFF_DOUBLE},
	{TIFFTAG_STONITS, 1, TIFF_DOUBLE},
};
#define NTAGS (sizeof(tags) / sizeof(tags[0]))

#define CopyTag(tag, count, type) cpTag(tifin, tifout, tag, count, type)

#define Z_BEST_COMPRESSION 9
#define Z_NORMAL_COMPRESSION 4

int main(int argc, char *argv[]) {
	TIFF *tifin = TIFFOpen(argv[1], "r");
	if (!tifin)
		return 0;
	unsigned int imagewidth;
	unsigned int imageheight;
	unsigned int rows_per_strip;

	unsigned int numtilesx;
	unsigned int numtilesy;

	unsigned int full_tile_width;

	unsigned char *full_tile_data;
	unsigned char *tile_data;

	uint16_t bitspersample, samplesperpixel, photometric, extrasamples;
	uint16_t shortv;

	// TIFFSetWarningHandler(0);
	TIFF *tifout = TIFFOpen(argv[2], "w8");

	CopyField(TIFFTAG_IMAGEWIDTH, imagewidth);
	CopyField(TIFFTAG_IMAGELENGTH, imageheight);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);

	TIFFGetField(tifin, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);

	unsigned int notaccepted = 0;
	if (TIFFIsTiled(tifin)) notaccepted = 1;
	else if (rows_per_strip != 1) notaccepted = 2;

	if (notaccepted) {
		switch (notaccepted) {
			case 1: std::cout << "input is already tiled"<< std::endl; break;
			case 2: std::cout << "rows per strip: " << rows_per_strip << " must be 1" << endl; break;
			default: std::cout << "not suitable" << std::endl;
		}
		TIFFClose(tifin);
		return 0;
	}
	cout << "size: " << imagewidth << " x " << imageheight << endl
		 << "samples per pixel: " << samplesperpixel << endl
		 << "bits per sample: " << bitspersample << endl
	     << "rows per strip: " <<  rows_per_strip << endl;

	TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	CopyField(TIFFTAG_PHOTOMETRIC, photometric);
	CopyTag(TIFFTAG_COLORMAP, 4, TIFF_SHORT);

	if (samplesperpixel == 4 && photometric == PHOTOMETRIC_RGB)	{
		// transparency mask support
		uint16_t sampleinfo[1];
		// unassociated alpha data is transparency information
		sampleinfo[0] = EXTRASAMPLE_UNASSALPHA;
		TIFFSetField(tifout, TIFFTAG_EXTRASAMPLES, 1, sampleinfo);
	}

	TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
	TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);
	
	numtilesx = (imagewidth + tilewidth-1)/tilewidth;
	numtilesy = (imageheight + tileheight-1)/tileheight;

	fprintf(stderr, "final size: %d x %d\n", imagewidth, imageheight);
	fprintf(stderr, "tile size: %d x %d\n", tilewidth, tileheight);
	fprintf(stderr, "num tiles: %d x %d\n", numtilesx, numtilesy);
#if defined(USE_JPEG)
	unsigned int quality = 75;
	TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
	TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, quality);
	fprintf(stderr, "jpeg quality: %d\n", quality);

#else
	unsigned int compression = Z_BEST_COMPRESSION;
	TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
	TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, compression);
	fprintf(stderr, "deflate compression: %d\n", compression);
#endif
    full_tile_width = imagewidth;
	full_tile_data = (unsigned char *)malloc(tileheight * full_tile_width * samplesperpixel);
	if (full_tile_data == 0) {
		fprintf(stderr, "bad alloc: %ld bytes\n", tileheight * full_tile_width * samplesperpixel);
		return 0;
	}
	tile_data = (unsigned char *)malloc(tilewidth * tileheight * samplesperpixel);
	unsigned int current_strip = 0;
	char buffer[128];


	std::cout << "Saving " << to_string(numtilesy) << "rows"; 

	for (unsigned int row = 0; row < numtilesy; ++row) {
		std::cout << "  " << (row + 1) << " / " << numtilesy ;
		std::cout.flush();

//		memset(full_tile_data, 0, tileheight * full_tile_width * samplesperpixel);
		for (unsigned int tiley = 0; tiley < tileheight; tiley++) {
			if (current_strip < imageheight) {
				// Read one strip
				TIFFReadEncodedStrip(tifin, current_strip, full_tile_data + full_tile_width * tiley * samplesperpixel, imagewidth * samplesperpixel);
			}
			current_strip++;
		}
		// done reading
		std::cout << "r";
		std::cout.flush();

		if (debug) {
			sprintf(buffer, "debug/row_%d.jpg", row);
			stbi_write_jpg(buffer,(int) full_tile_width, (int)tileheight, samplesperpixel, full_tile_data, 80);
		}


		for (unsigned int col = 0; col < numtilesx; ++col) {
			// now copy
			unsigned char *startx = full_tile_data + col * tilewidth * samplesperpixel;
			unsigned int offset = full_tile_width * samplesperpixel;

			for (unsigned int crow = 0; crow < tileheight; ++crow) {
				memcpy(tile_data + samplesperpixel * tilewidth * crow, startx + offset * crow, tilewidth * samplesperpixel);
			}
			if (debug) {
				sprintf(buffer, "debug/row_%d_col_%d.jpg", row, col);
				stbi_write_jpg(buffer, (int)tilewidth, (int)tileheight, samplesperpixel, tile_data, 80);
			}
			//write the tile
			TIFFWriteEncodedTile(tifout,
								 TIFFComputeTile(tifout,
												 col * tilewidth,
												 row * tileheight,
												 0, 0),
								 tile_data, samplesperpixel * tilewidth * tileheight);
		}
		// all current row tiles written
		std::cout << "w                      \r";
		std::cout.flush();
	}
	std::cout << endl;
	TIFFClose(tifout);
	TIFFClose(tifin);
	free(full_tile_data);
	free(tile_data);
}
