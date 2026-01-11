cd /opt
sudo mkdir emsdk
sudo chown $(whoami) emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest
cd ../
cd ../