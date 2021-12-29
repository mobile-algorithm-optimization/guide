# 移动端算法优化 - GPU 优化技术-OpenCL 运行时 API 介绍

### 运行说明
本代码在高通865平台和MTK820平台编译运行通过。编译运行请按照以下步骤：
1. 使用adb devices和adb shell检查手机连接状况。
2. 设置环境变量${NDK_PATH}为NDK根目录。
3. 修改qcom/mtk_test.sh和qcom/mtk_adbshell.sh中可执行文件和cl文件保存的路径。
4. Readme当前目录下，高通平台运行./qcom_test.sh，MTK平台运行./mtk_test.sh。

### 高通骁龙865运行示例
```text
Kernel TransposeKernel max workgroup size=1024
Kernel TransposeKernel perferred workgroup size multiple=128
Matrix Width =4096 Height=4096
Cpu Transpose consume average time: 142126 us
global_work_size=(4096,4096)
local_work_size=(32,32)
OpenCL Transpose consume average time: 11620 us
A and B match!
```

### MTK天玑820运行示例
```test
Kernel TransposeKernel max workgroup size=512
Kernel TransposeKernel perferred workgroup size multiple=16
Matrix Width =4096 Height=4096
Cpu Transpose consume average time: 426811 us
global_work_size=(4096,4096)
local_work_size=(16,16)
OpenCL Transpose consume average time: 13208 us
A and B match!
```
