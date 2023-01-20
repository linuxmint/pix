# Example

This is an example extension that can be used as a starting point to develop
new extensions.

To create a new extension named my-extension type the following:

    cp -R example ~/my-extension
    cd ~/my-extension
    ./init.sh my-extension

To compile the extension:

    meson build
    ninja -C build

To install:

    sudo ninja -C install

After installing the extension, start gThumb and activate it from the
Preferences dialog.

If you think your extension can be useful to others as well, add it to
the extensions list available at <https://wiki.gnome.org/Apps/Gthumb/extensions>.
