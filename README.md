# Pix
![build](https://github.com/linuxmint/pix/actions/workflows/build.yml/badge.svg)

An image viewer and browser utility.
Pix is part of the X-Apps project, which aims at producing cross-distribution and cross-desktop software.
 
## Features

 * Image browser

   + Browse your hard disk showing you thumbnails of image files.
   + Thumbnails are saved in the same database used by Nautilus so you
     don't waste disk space.
   + Automatically update the content of a folder.
   + Copy, move, delete images and folders.
   + Bookmarks of folders and catalogs.

 * Image viewer

   + View single images (including GIF animations).  Supported image
     types are: BMP, JPEG, GIF, PNG, TIFF, TGA, ICO, XPM, JXL, AVIF.
   + Optional support for RAW and HDR (high dynamic range) images.
   + View EXIF data attached to JPEG images.
   + View in fullscreen mode.
   + View images rotated, flipped, mirrored.

 * Image organizer

   + Add comments to images.
   + Organize images in catalogs, catalogs in libraries.
   + Print images and comments.
   + Search for images on you hard disk and save the result as a catalog.
     Search criteria remain attached to the catalog so you can update it
     when you want.

 * Image editor

   + Change image hue, saturation, lightness, contrast and adjust colors.
   + Scale and rotate images.
   + Save images in the following formats: JPEG, PNG, TIFF, TGA.
   + Crop images.
   + Red-eye removal tool.

 * Advanced tools

   + Import images from a digital camera.
   + Slide Shows.
   + Set an image as Desktop background.
   + Create index image.
   + Rename images in series.
   + Convert image format.
   + Change images date and time.
   + JPEG lossless transformations.
   + Find duplicated images.

## Licensing

  This program is released under the terms of the GNU General Public
  License (GNU GPL), either version 2, or (at your option) any later version.

  You can find a copy of the license in the file COPYING.

## Dependencies

  Mandatory libraries:

  * glib >= 2.38.0
  * gtk >= 3.16
  * libpng
  * zlib
  * libjpeg
  * gsettings-desktop-schemas

  While not mandatory, the following libraries greatly increase Pix's basic usefulness:

  * exiv2 - embedded metadata support
  * gstreamer, gstreamer-plugins-base, gstreamer-video - audio/video support
  * libtiff - tiff writing support

  Other optional libraries:

  * libraw - some support for RAW photos
  * librsvg - display SVG images
  * libwebp - display and save WebP images
  * libjxl - display JPEG XL images
  * libheif - display and save AVIF images
  * lcms2, colord - color profile support
  * champlain, champlain-gtk - view the place a photo was taken on a map
  * clutter, clutter-gtk - enhanced slideshow effects
  * libsoup, json-glib, webkit2gtk, libsecret - upload images to and
    download images from some web services such as Facebook, Flickr
  * brasero - write images and comments to CDs
  * bison, flex - web albums

## Installation

    cd pix
    meson build
    ninja -C build
    sudo ninja -C build install

## Credits

  Pix is based on gThumb 3.12.2.
  Many thanks to the original developers and to all the people who contributed to Pix.
