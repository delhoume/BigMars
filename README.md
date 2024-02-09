This project is a guide for creating a **single TIFF file** and a Deep Zoom structure that will allow viewing interactively
a gigantic (**42678000 x 2086480 pixels**, that's a lot, at 8.9 terapixels) imaage of Mars surface.

The final TIFF image weights about 1 Terabyte, JPEG compressed (much more if Deflate is used), includes pre-computed sublevels and is tiled to allow even modest computers
to be able to view (zoom, unzoom, pan).

This specially crafted pyramidal tiled TIFF can be opened with my Open Source software for Windows https://github.com/delhoume/vliv .
The generated Deep Zoom structure can be served using any HTTP server and visualized with a browser thanks to the OpenSeaDragon project :
https://openseadragon.github.io/

The source for the data is https://murray-lab.caltech.edu/CTX/index.html

Data is available as one file for a part of the surface that covers 4 square degree, a ZIP file containing geo localization, general information and data.
There are 90 x 44 of them for a total of **3960** files.
We will only use the TIFF that holds pixel data (greyscale 1 byte per pixel), holds **47420x47420 pixels**, and is uncompressed.
A single pixel covers about 5 meters.

They are available at https://murray-lab.caltech.edu/CTX/V01/tiles/ 

## Step 1 Downloading the source data
  
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

## Step 2 Preparing the data 

Each ZIP file contains a 47420x47420 uncompressed 1 byte per pixel TIFF image that weights **2254080140** bytes.

So once decompressed the total would be enormous at 80 To, not counting the ZIP files..

Keeping these files uncompressed makes no sense unless you have a lot of disk space.

So I have a command that will decompress the ZIP one by one, create a Deflate compressed TIFF in another folder and once done,
delete the uncompressed temporary file.

The root folder for this project is ```BigMars```

I keep the downloaded ZIPs in ```OriginalData```, extracted uncompressed TIFFSs in ```OriginalTiffs``` and compressed TIFFs in ```ZippedTiffs```.
Sources are in src, compiled binaries in bin.

Dependencies can be installed with the ```brew``` command (https://brew.sh/).
In a terminal (I recommend **iTerm**), in the BigMars clone repository folder, type:

```brew install gcc libtiff jpeg-turbo zlib-ng zstd lzma curl sevenzip parallel```

once all is installed

```make```

this should build all necessary custom programs

The first program we will run is 

```bin/check_all``` 

that checks for presence of 3960 processed TIFFs in ZippedTiffs and outputs a list of commands to obtain them if not.

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
Or add an ```rm``` for OriginalData  once ZippedTiffs are done.
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
It is still possible to open them using normal viewers but it might be slow and ultimately fail because of the 47420x47420 size.
Vliv can load-on-demand strips as well as tiles TIFFs, but in this case strips are very large and it is not optimal at all.

```bin/strip2tiled.jpg MurrayLab_CTX_V01_E000_N-00_Mosaic.tif center.tif```

**open center.tif in Vliv** you can pan as you want very smoothly in this already large image. **Celebrate !**
It is expected you cannot zoom out yet...

Next step swill see us generate images 90 times larger and from 2 to 44  times higher...

## Step 3 Building a 189680x189680 pyramidal image

When you deal with such large images (or data) you have to mitigate the fact that you cannot load it in memory (by far).

While it is possible to write code that generates a tiled TIFF directly from the TIFFs generated at step 2, my strategy is to generate first a 
one pixel per strip full image, then convert it to tiled.
The command ```bin/buildmarsimage <out.tif> <cols> <rows>``` will generate a moisaic given a number of rows and columns.
 you can start with a modest ```bin/buildmarsimage fourbyfour.tif 4 4```  that will only take less than one hour.

 You can change the folder where ZippedTiffs is to be found when building the commands.
 See vaiable FOLDER in the Makefile 

 As for the single tile TIFFS, you will have to convert it to tiled format to display with Vliv.
```bin/strip2tiled.jpg fourbyfour.tif 0.tif```

In order to zoom and unzoom in this very large image without loading it into memory, we will create a so called pyramid, successive
images with half width and height from previous one, until you reach a screen viewable size (or a 1x1 pixel image).
Even on ginormous TIFFs, if they are tiled, this can be done using almost no memory (5 times a single tile).

```
bin/hafltiff_stb 0.tif 1.tif
bin/hafltiff_stb 1.tif 2.tif
bin/hafltiff_stb 2.tif 3.tif
bin/hafltiff_stb 3.tif 4.tif
bin/hafltiff_stb 4.tif 5.tif
bin/hafltiff_stb 5.tif 6.tif
bin/hafltiff_stb 6.tif 7.tif
bin/hafltiff_stb 7.tif 8.tif
```

The last image should be 740x740 pixels, all intermediate levels TIFFs can be opened in Vliv.

The final step is to assemble all levels into a single TIFF:

```bin/tiffmerge.first 0.tif 1.tif 2.tif 3.tif 4.tif 5.tif 6.tif 7.tif 8.tif mars_final_fourbyfour.tif```

![Vliv displaying various regions and zooms for mars_final_fourbyfour.tif ](images/mosaic.png)

It can be opened in Vliv and you can navigate through levels using the mouse wheel, giving the illusion of zoom. Note also that Vliv supports a joystick
for panning and (un)zooming.


The final pyramidal TIFF should weight no more than **1.33 times the full size image** thanks to mathematics (1 + 1/4 + 1/16 + ...)

## Step 4 Building a the full multi-terapixel pyramidal image of Mars surface

Once this is done, you may want to build the real deal, the full Mars surface image with
```bin/buildmarsimage 90 44```

I have yet to finish a complete build, so far it is running for 3 days and estimated remaining time is 2 days.
Final running  time is notoriously hard to predict (just think about a Windows copy dialog with a large number of various sized files,
the estimation for completion can vary very much).
I display 2 estimates, one that, given the number of completed rows since the start, computes an average per row time and applys it to remaining rows, the other
based on instant row performance if sustained for the remaining rows.
Depending on your machine load, disk drives speed, memory, they can differ much but should converge at the very end...

Once you have the full 90x44 ```mars_full_rgb_strip.tif``` you follow the same process than for the 10x10 one, converting to tiled, generating sublevels, merging.
Processing time will be significantly higher as the full image is about 40 times larger...

The final **42678000x2086480 pixels** for the full resolution TIFF is divided into **8840x4076 **512x512 tiles**, takes about 6 Terabytes on disk,
 and yet **can be instantly opened in Vliv**

**Please notify me if you got this far !**


You can tweak the programs to change compression type or level, or whatever you can think of.
You cannot for example have JPEG compressed strip TIFF with less than 8 rows per strip, and limit for JPEG is well below the width of the full Mars image
(JPEG limits are 65535x65535 I think).
Given the enormous amount of data, (de)compression levels can have a significant impact on processing time.
I switch to JPEG TIFFs for tiled ones, giving a good peformance and much reduced disk usage than Deflate ones.

In the next step, we will see how to create a Deep Zoom layout, that will allow interactive (and impressive) visualization with a simple
Web browser








