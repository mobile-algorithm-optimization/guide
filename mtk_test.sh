cd build_android
rm -r *
cmake -DANDROID=android -DMTK=mtk -DCMAKE_BUILD_TYPE=Release ..
make install
adb push install/bin/* /data/local/Transpose/bin/
adb shell < ../mtk_adbshell.sh