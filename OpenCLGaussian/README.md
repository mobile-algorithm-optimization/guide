# 移动端算法优化 - OpenCL kernel 开发

### 运行说明
本代码在高通8450平台编译运行通过。编译运行请按照以下步骤：
1. 使用adb devices和adb shell检查手机连接状况。
2. 设置环境变量${NDK_PATH}为NDK根目录。
3. 修改qcom/mtk_test.sh和qcom/mtk_adbshell.sh中可执行文件和cl文件保存的路径。
4. Readme当前目录下，高通平台运行./qcom_test.sh，MTK平台运行./mtk_test.sh。

### 高通骁龙8450运行示例
```text
Kernel Gauss3x3u8c1Buffer max workgroup size=1024
Kernel Gauss3x3u8c1Buffer perferred workgroup size multiple=128
Matrix Width =4096 Height=4096
Cpu Gaussian consume average time: 35203 us
global_work_size=(1022,4096)
local_work_size=(32,32)
queue -> submit : 22.016000us
submit -> start : 8.960000us
start -> end : 1647.104000us
end -> finish : 0.000000us
OpenCL Gaussian consume average time: 1959 us
A and B match!
```
