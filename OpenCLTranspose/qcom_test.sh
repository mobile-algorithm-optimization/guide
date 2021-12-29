cd build_android
rm -r *
cmake -DANDROID=android -DQCOM=qcom -DCMAKE_BUILD_TYPE=Release ..
make install
adb push install/bin/* /data/local/Transpose/bin/
adb shell < ../qcom_adbshell.sh