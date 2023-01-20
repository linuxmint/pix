#/usr/bin/sh

if [ -z $1 ]; then
  echo "Usage: ./init.sh NEW_NAME]"
  echo "Change the name of the extension to NEW_NAME."
  echo ""
  echo "  NEW_NAME            The new name to use, separate words with - or _"
  exit 1
fi

extensionname=$1
originalname=example

sed -e "s|$originalname|$extensionname|" < meson.build > meson.build.tmp
mv meson.build.tmp meson.build

sed -e "s|$originalname|$extensionname|" < src/${originalname}.extension.desktop.in.in > src/${extensionname}.extension.desktop.in.in
rm src/${originalname}.extension.desktop.in.in

sed -e "s|$originalname|$extensionname|" < src/meson.build > src/meson.build.tmp
mv src/meson.build.tmp src/meson.build
