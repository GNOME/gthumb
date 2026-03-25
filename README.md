# Thumbnails

Image viewer, editor, browser and organizer.

More information can be found at <https://gitlab.gnome.org/GNOME/gthumb/>.

![Image](https://gitlab.gnome.org/GNOME/gthumb/raw/master/data/appdata/viewer.png)

## Features

  - View images: PNG, JPEG, WEBP, SVG, JXL, HEIF, AVIF, TIFF, GIF, RAW.

  - Edit images: scale, rotate and crop images, change contrast, brightness and saturation, as well as other color transformations.

  - View and edit embedded metadata: EXIF, IPTC and XMP.

  - File manager operations: copy, move, rename and delete files and folders, execute scripts on the selected files.

  - Image specific tools: JPEG lossless transformations, image resize, format conversion, slideshow, setting an image as desktop background and others.

  - Add comments and other metadata to images, organize images in catalogs and catalogs in libraries, search for images and save the result as a catalog.

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
  * gstreamer, gstreamer-plugins-base, gstreamer-video: audio/video support
  * gsettings-desktop-schemas

  Other optional libraries:

  * libtiff: display and save TIFF images
  * libraw: some support for RAW photos
  * librsvg: display SVG images
  * libwebp: display and save WebP images
  * libjxl: display and save JPEG XL images
  * libheif: display and save AVIF images
  * libgif: display GIF animation
  * colord: read the monitor color profile

## Download

  Source archives available at <https://download.gnome.org/sources/gthumb/>.

  Clone the Git repository:

    git clone https://gitlab.gnome.org/GNOME/gthumb.git

## Installation

    meson setup builddir
    meson compile -C builddir
    sudo meson install -C builddir
