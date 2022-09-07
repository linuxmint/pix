# Building from source
### Option 1 (The easy way - Debian/Ubuntu only):
```
sudo apt install devscripts equivs
git clone https://github.com/linuxmint/pix.git
cd pix

# install dependencies (source repos must be enabled and you need to confirm the installation)
sudo mk-build-deps -ir

# build
dpkg-buildpackage

# install
sudo dpkg -i ../*.deb
```
### Option 2 (Everywhere else):

#### Required dependencies:
- intltool
- libexiv2-dev (>= 0.21)
- flex
- bison
- libtiff-dev
- libpng12-dev or libpng-dev
- libjpeg-dev
- libsm-dev
- libice-dev
- automake
- autoconf
- libtool
- gnome-common
- gsettings-desktop-schemas-dev
- libgtk-3-dev (>= 3.20.0)
- libxml2-dev (>= 2.4.0)
- libglib2.0-dev (>= 2.34.0)
- libcairo2-dev
- libltdl3-dev
- libsoup-gnome2.4-dev (>= 2.36)
- librsvg2-dev (>= 2.34.0)
- zlib1g-dev
- yelp-tools
- libgstreamer1.0-dev
- libsecret-1-dev
- libgstreamer-plugins-base1.0-dev
- libwebp-dev
- libwebkit2gtk-4.0-dev

####  While not mandatory, these libraries greatly increase Pix's basic usefulness:

- exiv2 (embedded metadata support)
- gstreamer (video support)
- libjpeg (jpeg writing support)
- libtiff (tiff writing support)

####  These libraries are optional:
- Enhanced slideshow effects:
    - clutter
    - clutter-gtk
- Partial RAW photo support:
    - libopenraw

##### Install dependencies (these are subject to change - check the first section of`debian/control` if you seem to be missing anything):
```
apt install git dpkg-dev
apt install gobject-introspection libdjvulibre-dev libgail-3-dev          \
            libgirepository1.0-dev libgtk-3-dev libgxps-dev               \
            libkpathsea-dev libpoppler-glib-dev libsecret-1-dev           \
            libspectre-dev libtiff-dev libwebkit2gtk-4.0-dev libxapp-dev  \
            mate-common meson xsltproc yelp-tools
```
##### Download the source-code to your machine:
```
git clone https://github.com/linuxmint/pix.git
cd pix
```
##### Configure the build:
```
# The following configuration installs all binaries,
# libraries, and shared files into /usr/local, and
# enables all available options:

./autogen.sh --prefix=/usr/local
```
##### Build and install (sudo or root is needed for install):
```
make
sudo make install
```
##### Run:
```
/usr/local/bin/pix
```
##### Uninstall:
```
ninja -C debian/build uninstall
```
