# Thumbnails

Image viewer, editor, browser and organizer.

More information can be found at <https://gitlab.gnome.org/GNOME/gthumb/>.

![Image](https://gitlab.gnome.org/GNOME/gthumb/raw/master/data/appdata/ss-viewer.png)

## Features

  Supported formats: PNG, JPEG, WEBP, SVG, JXL, HEIF, AVIF, TIFF, GIF.
  View metadata types embedded inside images such as EXIF, IPTC and XMP.

  Scale, rotate and crop the images; change the saturation, lightness,
  contrast as well as other color transformations.

  Perform the common operations of a file manager such as copy, move and
  delete files and folders, plus a series of image specific tools such as JPEG
  lossless transformations; image resize; format conversion; slideshow;
  setting an image as desktop background and others.

  Add comments and other metadata to images; organize images in catalogs
  and catalogs in libraries; search  for images and save the result as
  a catalog.

## Licensing

  This program is released under the terms of the GNU General Public
  License (GNU GPL), either version 2, or (at your option) any later version.

  You can find a copy of the license in the file COPYING.

## Dependencies

  Mandatory libraries:

  * glib >= 2.84
  * gtk >= 4.18
  * libpng
  * zlib
  * libjpeg
  * exiv2: embedded metadata support
  * lcms2: color profile support
  * gsettings-desktop-schemas

  Other optional libraries:

  * libtiff: display and save TIFF images
  * libraw: some support for RAW photos
  * librsvg: display SVG images
  * libwebp: display and save WebP images
  * libjxl: display and save JPEG XL images
  * libheif: display and save AVIF images
  * libgif: display GIF animation
  * gstreamer, gstreamer-plugins-base, gstreamer-video: audio/video support
  * colord: read the monitor color profile

## Download

  Source archives available at <https://download.gnome.org/sources/gthumb/>.

  Clone the Git repository:

    git clone https://gitlab.gnome.org/GNOME/gthumb.git

## Installation

    cd gthumb
    meson build
    ninja -C build
    sudo ninja -C build install
