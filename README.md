This project is a guide for creating an single TIFF file and a Deep Zoom structure that will allow viewing interactively
a gigantic (42678000 x 2086480 pixels, that's a lot, at 8.9 terapixels).

The final TIFF image weights about 1 Terabyte, JPEG compressed (much more if Deflate is used), includes pre-computed sublevels and is tiled to allow even modest computers
to be able to view, zoom unzoom pan the full image.

This specially crafted pyramidal tiled TIFF can be opened with my Open Source software for Windows https://github.com/delhoume/vliv .
The generated Deep Zoom structure can be served using any HTTP server and visualized with a browser thanks to the OpenSeaDragon project :
https://openseadragon.github.io/

The source for the data is https://murray-lab.caltech.edu/CTX/index.html

Data is available as one file for a part of the surface that covers 4 square degree, a ZIP file containing geo localization, general information and data.
There are 90 x 44 of them for a total of 3960 files.
We will only use the TIFF that holds pixel data (greyscale 1 byte / per pixel), has a 47420x47420 pixel size, and is uncompressed.
A single pixel covers about 5 meters.

They are available at https://murray-lab.caltech.edu/CTX/V01/tiles/ 

Step 1
  
The first step towards the final image is to download the 3960 zip files.
This requires about 8 terabytes of disk space, I bought a 12To WD MyBook to store them.

I use a 2020 Mac Mini M1, but source code and scripts can be easily adapted or compiled for Windows and Linux.

```scripts/download_all.sh``` will download all, thanks to their HTTP server supporting ```Accept-Ranges```.
You can stop at any moment and downloading will resume when you relaunch it.
The download is split in 4 parts because I did not find how to create a single cUrl command for the files pattern.

Files are named using longitudes from -180 to 176  and latitudes from -88 to 84 in 4 increment

```
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E[000-176:4]_N[00-84:4].zip" -O
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E[000-176:4]_N-[04-88:4].zip" -O
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E-[004-180:4]_N[00-84:4].zip" -O
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E-[004-180:4]_N-[04-88:4].zip" -O
```

After a while, depending on your Internet connection speed, you should have the 3960 files. It took me about one month.

Step 2 (extract TIFF files) 

Each ZIP file contains a 47420 x 47420 uncompressed 1 byte per pixel TIFF image that weights 2254080140 bytes.

So once decompressed the total would be enormous at 80 To, not counting the ZIP files..

Keeping these files uncompressed makes no sense unless you have a lot of disk space.

So I have a command that will decompress the ZIP one by one, create a Deflate compressed TIFF in another folder and once done,
delete the uncompressed temporary file.

The root folder for this project is BigMars

I keep the downloaded ZIPs in OriginalData folder, extracted uncomressed TIFFSs in OriginalTiffs and compressed TIFFs in ZippedTiffs.
Sources are in src, compiled binaries in bin.

Dependencies can be installed with the brew command (https://brew.sh/).

You will need 
```
brew install gcc 
brew install libtiff
brew install jpeg-turbo
brew install zlib-ng
brew install zstd
brew install lzma
brew install curl
brew install sevenzip
brew install parallel
```

then type ```make``` at the root level, this should build all necessary custom programs

The first program we will run is ```bin/check_all``` that checks for presence of 3960 processed TIFFs in ZippedTiffs
and outputs a list of commands to obtain them if not.

Typical output is
```
ZippedTiffs/MurrayLab_CTX_V01_E000_N00_Mosaic.tif  (1935 / 3960) missing
cd OriginalData
curl -C - "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E000_N00.zip" -O
cd ..
7zz e OriginalData/MurrayLab_GlobalCTXMosaic_V01_E000_N00.zip -oOriginalTiffs "*.tif" -r -aos
tiffcp -s -r 1 -c zip:p9 OriginalTiffs/MurrayLab_CTX_V01_E000_N00_Mosaic.tif ZippedTiffs/MurrayLab_CTX_V01_E000_N00_Mosaic.tif
rm -rf OriginalTiffs/MurrayLab_CTX_V01_E000_N00_Mosaic.tif
...
```
```bin/check_all > process.sh```

Note that TIFF files have a slightly different naming than ZIP files.

Depending on your available disk space you might want to comment the ```rm```command lines.
Or ad an rm for OriginalData  once ZippedTiffs are done.
You may also change the ```curl``` or the ```7zz``` parts if you keep all files.

Launch the ```process.sh``` script using ```source process.sh```

Processing all 3960 source files can take a considerable amount of times (in days).

You can speed up parts of this process leveraging modern multicore processors with the
```parallel```command.
For exemple, if you have extracted all TIFFs on OriginalTiffs folder you may convert them to Deflate using

```ls -a OriginalTiffs/*.tif | parallel tiffcp -s -r 1 -c zip:p9 {} ZippedTiffs/{/}```

This should be N times faster. Same for 7zz extraction.

You should at this moment have 3960 smaller TIFFs in ZippedTiffs, in a format suitable for the next step.
They take about 5.5 To of disk space.

If you want to view the ZippedTiffs, you will have to convert them to  tiled format  that Vliv (Windows only) can open.
As is it is still possible to open them using normal viewers but it might be slow and ultimately fail because of the 47420x47420 size.

```bin/strip2tiled.jpg MurrayLab_CTX_V01_E000_N-00_Mosaic.tif center.tif```
open center.tif in Vliv you can pan as you whish in this already large image. Celebrate !

Next step will see us generate images 90 times larger and from 2 to 44  times higher...


