pushd /opt
sudo mkdir emsdk
sudo chown $(whoami) emsdk
git clone https://github.com/emscripten-core/emsdk.git
pushd emsdk
git pull
./emsdk install latest
./emsdk activate latest
popd
popd