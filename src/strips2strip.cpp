#include <tiffio.h>
#include <zlib.h>

#include <iostream>
using namespace std;

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>


/* 
 cl /EHsc /O2 /MD /nologo /Iinclude /Iinclude\tiff-4.3.0\libtiff strips2strip.cpp libs64\libtiff.lib libs64\zlib.lib libs64\turbojpeg-static.lib libs64\libwebp.lib
*/

#define	CopyField(tag, v) \
    if (TIFFGetField(tifin, tag, &v)) TIFFSetField(tifout, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(tifin, tag, &v1, &v2)) TIFFSetField(tifout, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(tifin, tag, &v1, &v2, &v3)) TIFFSetField(tifout, tag, v1, v2, v3)
#define	CopyField4(tag, v1, v2, v3, v4) \
    if (TIFFGetField(tifin, tag, &v1, &v2, &v3, &v4)) TIFFSetField(tifout, tag, v1, v2, v3, v4)

static void
cpTag(TIFF* tifin, TIFF* tifout, uint16_t tag, uint16_t count, TIFFDataType type)
{
	switch (type) {
	case TIFF_SHORT:
		if (count == 1) {
			uint16_t shortv;
			CopyField(tag, shortv);
		} else if (count == 2) {
			uint16_t shortv1, shortv2;
			CopyField2(tag, shortv1, shortv2);
		} else if (count == 4) {
			uint16_t *tr, *tg, *tb, *ta;
			CopyField4(tag, tr, tg, tb, ta);
		} else if (count == (uint16_t) -1) {
			uint16_t shortv1;
			uint16_t* shortav;
			CopyField2(tag, shortv1, shortav);
		}
		break;
	case TIFF_LONG:
		{ uint32_t longv;
		  CopyField(tag, longv);
		}
		break;
	case TIFF_RATIONAL:
		if (count == 1) {
			float floatv;
			CopyField(tag, floatv);
		} else if (count == (uint16_t) -1) {
			float* floatav;
			CopyField(tag, floatav);
		}
		break;
	case TIFF_ASCII:
		{ char* stringv;
		  CopyField(tag, stringv);
		}
		break;
	case TIFF_DOUBLE:
		if (count == 1) {
			double doublev;
			CopyField(tag, doublev);
		} else if (count == (uint16_t) -1) {
			double* doubleav;
			CopyField(tag, doubleav);
		}
		break;
	default:
		TIFFError(TIFFFileName(tifin),
		    "Data type %\"PRIu16\" is not supported, tag %d skipped.",
		    tag, type);
	}
}

static const struct cpTag {
	uint16_t tag;
	uint16_t count;
	TIFFDataType type;
} tags[] = {
	{ TIFFTAG_SUBFILETYPE,		1, TIFF_LONG },
	{ TIFFTAG_THRESHHOLDING,	1, TIFF_SHORT },
	{ TIFFTAG_DOCUMENTNAME,		1, TIFF_ASCII },
	{ TIFFTAG_IMAGEDESCRIPTION,	1, TIFF_ASCII },
	{ TIFFTAG_MAKE,			1, TIFF_ASCII },
	{ TIFFTAG_MODEL,		1, TIFF_ASCII },
	{ TIFFTAG_MINSAMPLEVALUE,	1, TIFF_SHORT },
	{ TIFFTAG_MAXSAMPLEVALUE,	1, TIFF_SHORT },
	{ TIFFTAG_XRESOLUTION,		1,                 TIFF_RATIONAL },
	{ TIFFTAG_YRESOLUTION,		1,                 TIFF_RATIONAL },
	{ TIFFTAG_PAGENAME,		1,                    TIFF_ASCII },
	{ TIFFTAG_XPOSITION,		1,                   TIFF_RATIONAL },
	{ TIFFTAG_YPOSITION,		1,                   TIFF_RATIONAL },
	{ TIFFTAG_RESOLUTIONUNIT,	1,                  TIFF_SHORT },
	{ TIFFTAG_SOFTWARE,		1,                    TIFF_ASCII },
	{ TIFFTAG_DATETIME,		1,                    TIFF_ASCII },
	{ TIFFTAG_ARTIST,		1,                      TIFF_ASCII },
	{ TIFFTAG_HOSTCOMPUTER,		1,                TIFF_ASCII },
	{ TIFFTAG_WHITEPOINT,		(uint16_t) -1,      TIFF_RATIONAL },
	{ TIFFTAG_PRIMARYCHROMATICITIES,(uint16_t) -1,   TIFF_RATIONAL },
	{ TIFFTAG_HALFTONEHINTS,	2,                   TIFF_SHORT },
	{ TIFFTAG_INKSET,		1,                      TIFF_SHORT },
	{ TIFFTAG_DOTRANGE,		2,                    TIFF_SHORT },
	{ TIFFTAG_TARGETPRINTER,	1,                   TIFF_ASCII },
	{ TIFFTAG_SAMPLEFORMAT,		1,                TIFF_SHORT },
	{ TIFFTAG_YCBCRCOEFFICIENTS,	(uint16_t) -1,   TIFF_RATIONAL },
	{ TIFFTAG_YCBCRSUBSAMPLING,	2,                TIFF_SHORT },
	{ TIFFTAG_YCBCRPOSITIONING,	1,                TIFF_SHORT },
	{ TIFFTAG_REFERENCEBLACKWHITE,	(uint16_t) -1, TIFF_RATIONAL },
	{ TIFFTAG_EXTRASAMPLES,		(uint16_t) -1,    TIFF_SHORT },
	{ TIFFTAG_SMINSAMPLEVALUE,	1,                 TIFF_DOUBLE },
	{ TIFFTAG_SMAXSAMPLEVALUE,	1,                 TIFF_DOUBLE },
	{ TIFFTAG_STONITS,		1,                     TIFF_DOUBLE },
};
#define	NTAGS	(sizeof (tags) / sizeof (tags[0]))

#define	CopyTag(tag, count, type)	cpTag(tifin, tifout, tag, count, type)


struct TIFFCmap {
    uint16_t rmap;
    uint16_t* gmap;
    uint16_t* bmap;
};

#define CVT(x) (unsigned char)((((x) * 255) / ((1L << 16) - 1)))


int main(int argc, char* argv[]) {
	TIFFSetWarningHandler(0);
 
    TIFF* tifin = TIFFOpen(argv[1], "r");
    TIFF* tifout = TIFFOpen(argv[2], "w");
 
    unsigned int imagewidth;
    unsigned int imageheight;
    unsigned int rows_per_strip;

    unsigned int x, y;
    unsigned int yy = 0;
    unsigned int y2;
    unsigned char* stripdata;
    unsigned char* linedata;
    unsigned int last, rest;
    
     uint16_t bitspersample, samplesperpixel, photometric; 
     uint16_t shortv;

    CopyField(TIFFTAG_IMAGEWIDTH, imagewidth);
	CopyField(TIFFTAG_IMAGELENGTH, imageheight);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
    CopyField(TIFFTAG_PHOTOMETRIC, photometric);

     if (samplesperpixel == 4 && photometric == PHOTOMETRIC_RGB) {
		// transparency mask support
		uint16_t sampleinfo[1]; 
		// unassociated alpha data is transparency information
		sampleinfo[0] = EXTRASAMPLE_UNASSALPHA;
		TIFFSetField(tifout, TIFFTAG_EXTRASAMPLES, 1, sampleinfo);
	 }
    TIFFGetField(tifin, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
    
    cout << "image : " << endl
	 << "  size:" << imagewidth << "x" << imageheight << endl
	 << "  rows per strip:" << rows_per_strip << endl; 
    if (rows_per_strip == 1) {
	    cout << "no conversion needed, copying contents..." << endl;
//	return 1;
    } else {
	    cout << "converting to one row per strip..." << endl;
    }
 
    TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
     if (shortv == PHOTOMETRIC_PALETTE) {
        cout << "exporting palette..." << endl;
        uint16_t* rmap = NULL;
		uint16_t* gmap = NULL;
		uint16_t* bmap = NULL;

        TIFFGetField(tifin, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap);
        unsigned char* palette = (unsigned char*)malloc(3 * 256);
        for (uint16_t c = 0; c < 256; ++c) {
            palette[c * 3 + 0] = CVT(rmap[c]);
            palette[c * 3 + 1] = CVT(gmap[c]);
            palette[c * 3 + 2] = CVT(bmap[c]);
        }
        stbi_write_png("palette.png", 1, 256, 3, palette, 0);
        free(palette);
    }
	CopyTag(TIFFTAG_COLORMAP, 4, TIFF_SHORT);

    TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE); 
	// temporary images only...
    TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, Z_NO_COMPRESSION);

    TIFFSetField(tifout, TIFFTAG_ROWSPERSTRIP, 1);

    stripdata = new unsigned char[samplesperpixel * rows_per_strip * imagewidth];
     
    last = imageheight - (imageheight % rows_per_strip);
    rest = imageheight % rows_per_strip;
    int strip = 0;
    for (y = 0; y < imageheight; y += rows_per_strip, ++strip) {
		if ((y % 1000) == 0) {
			cout << "|" << y << "|";
		} else if ((y % 100) == 0) {
			cout << ".";
		}
	    TIFFReadEncodedStrip(tifin, strip, stripdata, samplesperpixel * rows_per_strip * imagewidth);
        for (y2 = 0; y2 < rows_per_strip; ++y2, ++yy) {
            if (yy < imageheight) {
                TIFFWriteEncodedStrip(tifout, yy, stripdata + y2 * samplesperpixel * imagewidth, samplesperpixel * imagewidth);
            }
        }
    }
    cout << endl << "Done" << endl;
    TIFFClose(tifout);
    TIFFClose(tifin);
    delete [] stripdata;
}
    
