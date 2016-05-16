if [ -z $1 ]; then
  echo "Usage: sh init.sh YOUR-EXTENSION-NAME [ORIGINAL-NAME]"
  echo "Change the name of the extension in YOUR-EXTENSION-NAME"
  echo ""
  echo "  YOUR-EXTENSION-NAME  The new name to use, separate the words with -"
  echo "  [ORIGINAL-NAME]      The original name to substitute (optional, default:example)"
  exit 1
fi

if [ -z $2 ]; then
  originalname=example
else
  originalname=$2
fi
originalname=`echo $originalname | tr '_' '-'`
original_name=`echo $originalname | tr '-' '_'`

extensionname=`echo $1 | tr '_' '-'`
extension_name=`echo $extensionname | tr '-' '_'`

orig_dir=`pwd`

sed -e "s|$originalname|$extensionname|" < autogen.sh > autogen.sh.tmp
mv autogen.sh.tmp autogen.sh
chmod a+x autogen.sh

if [ -e configure.ac.example ]; then
  mv configure.ac.example configure.ac
fi
sed -e "s|gthumb-$originalname|gthumb-$extensionname|" < configure.ac > configure.ac.tmp
mv configure.ac.tmp configure.ac

if [ -e src/$original_name.extension.in.in ]; then
  mv src/$original_name.extension.in.in src/$extension_name.extension.in.in
fi
sed -e "s|$original_name|$extension_name|" < src/Makefile.am > src/Makefile.am.tmp
mv src/Makefile.am.tmp src/Makefile.am

cd po
sh update-potfiles.sh > POTFILES.in

cd $orig_dir
