# 移动端算法优化 - CPU 优化技术 - NEON 优化实例

### 运行说明
本代码在Android手机平台编译运行通过。编译运行请按照以下步骤：
1. 使用adb devices和adb shell检查手机连接状况。
2. 设置环境变量${NDK_PATH}为NDK根目录。
3. 修改 adbshell.sh 和 build.sh 中可执行文件保存的路径。
4. README当前目录下，运行./build.sh。

### 高通骁龙888运行示例

Gaussian3x3 None average time = 15.589470 
Gaussian3x3 Neon average time = 3.236440 
Gaussian3x3None and Gaussian3x3Neon result bit matched
gausaian_testcase done! 



