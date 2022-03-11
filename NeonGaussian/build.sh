rm -rf build_android/
mkdir build_android && cd build_android
cmake -DCONFIG=android_arm64 -DCMAKE_BUILD_TYPE=Release ..
make install
adb shell mkdir -p /data/local/Gaussian/bin/
adb push install/bin/* /data/local/Gaussian/bin/

adb shell mkdir -p /data/local/Gaussian/bin/data/
adb push ../data/* /data/local/Gaussian/bin/data/

adb shell < ../adbshell.sh
