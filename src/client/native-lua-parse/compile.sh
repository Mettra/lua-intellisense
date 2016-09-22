#for OS in linux mac win; do 
    #GYP_DEFINES="-DOS=$OS" CC=gcc-$OS CXX=g++-$OS node-gyp rebuild --verbose --arch=ia32 --target=0.37.6 --dist-url=https://atom.io/download/atom-shell
#done

export AR="ar"
export CC="gcc"
export CXX="g++"
export LINK="g++"
export GYP_CROSSCOMPILE=1
export GYP_DEFINES="OS=linux"

node-gyp configure
node-gyp rebuild --verbose --arch=ia32 --target=0.37.6 --dist-url=https://atom.io/download/atom-shell