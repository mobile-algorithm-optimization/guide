#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

template <typename T>
void check(T result, char const *const func, const char *const file, int const line)
{
    if (result)
    {
        fprintf(stderr, "CL error at %s:%d code=%d \"%s\" \n", file, line,
                static_cast<unsigned int>(result), func);
        exit(result);
    }
}

#define CHECK_OPENCL_ERROR(val) check((val), #val, __FILE__, __LINE__)

void DataInit(cl_uchar *p_data, int width, int height);
void DataCompare(cl_uchar *src1, cl_uchar *src2, int width, int height);

cl_context CreateContext(cl_device_id *p_device);
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id device);
cl_program CreateProgram(cl_context context, cl_device_id device, char *source);
cl_kernel CreateKernel(cl_program program, const char *kernel_name, cl_device_id device);
void PrintProfilingInfo(cl_event event);

bool CreateMemObject(cl_context context, cl_mem memobject[2], cl_uchar *img_ptr,
                     cl_uint image_size);
void CleanUp(cl_context context, cl_command_queue commandqueue, cl_program program,
             cl_kernel kernel);

int Gaussian3x3Sigma0U8C1(uint8_t *src, int width, int height, int istride,
                          uint8_t *dst, int ostride);
int Gauss3x3Sigma0U8C1RemainData(uint8_t *src, int remain_col_index, int row, int col, int src_pitch,
                                int dst_pitch, uint8_t *dst);
std::string ClReadString(const std::string &filename);
void ClUtilWriteStringToFile(const cl_uchar *text, size_t text_length, char *filename);
void PrintDuration(timeval *start, const char *str, int loop_count);
void CheckClStatus(cl_int ret, const char *failure_msg);

int main()
{
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer_src;
    cl_mem buffer_dst;
    cl_int err_num = CL_SUCCESS;
    cl_uint buffer_size_in_bytes;
    timeval start;

    context = CreateContext(&device);
    if (NULL == context)
    {
        printf("MainError:Create Context Failed!\n");
        return -1;
    }

    command_queue = CreateCommandQueue(context, device);
    if (NULL == command_queue)
    {
        printf("MainError:Create CommandQueue Failed!\n");
        return -1;
    }

    std::string source_name = "gaussian.cl";
    std::string program_source =  ClReadString(source_name);
    char *cl_str   = (char *)program_source.c_str();
    program        = CreateProgram(context, device, cl_str);
    if (NULL == program)
    {
        printf("MainError:Create CommandQueue Failed!\n");
        return -1;
    }

    kernel = CreateKernel(program, "Gauss3x3u8c1Buffer", device);
    if (NULL == kernel)
    {
        printf("MainError:Create Kernel Failed!\n");
        return -1;
    }

    const int c_loop_count = 30;

    int width                          = 4096;
    int height                         = 4096;
    int istride                        = width;
    int ostride                        = width;
    buffer_size_in_bytes               = width * height * sizeof(cl_uchar);
    cl_uchar *host_src_matrix          = (cl_uchar *)malloc(buffer_size_in_bytes);
    cl_uchar *host_gaussian_matrix   = (cl_uchar *)malloc(buffer_size_in_bytes);
    cl_uchar *device_gaussian_matrix = (cl_uchar *)malloc(buffer_size_in_bytes);
    memset(device_gaussian_matrix, 0, buffer_size_in_bytes);
    DataInit(host_src_matrix, width, height);
    printf("Matrix Width =%d Height=%d\n", width, height);
    gettimeofday(&start, NULL);
    for (int i = 0; i < c_loop_count; i++)
    {
        Gaussian3x3Sigma0U8C1(host_src_matrix, width, height, width, host_gaussian_matrix, width);
    }
    PrintDuration(&start, "Cpu Gaussian", c_loop_count);

    buffer_src = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                buffer_size_in_bytes, host_src_matrix, &err_num);
    CheckClStatus(err_num, "Create src buffer");
    buffer_dst = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size_in_bytes, NULL, &err_num);
    CheckClStatus(err_num, "Create dst buffer");

    err_num  = clSetKernelArg(kernel, 0, sizeof(buffer_src), &buffer_src);
    err_num |= clSetKernelArg(kernel, 1, sizeof(height), &height);
    err_num |= clSetKernelArg(kernel, 2, sizeof(width), &width);
    err_num |= clSetKernelArg(kernel, 3, sizeof(istride), &istride);
    err_num |= clSetKernelArg(kernel, 4, sizeof(ostride), &ostride);
    err_num |= clSetKernelArg(kernel, 5, sizeof(buffer_dst), &buffer_dst);
    CheckClStatus(err_num, "Set Kernel Arg");

    size_t local_work_size[3];
#if defined(QCOM_DEVICE)
    local_work_size[0] = 32;
    local_work_size[1] = 32;
#elif defined(MTK_DEVICE)
    local_work_size[0] = 16;
    local_work_size[1] = 16;
#else
    local_work_size[0] = 16;
    local_work_size[1] = 16;
#endif
    local_work_size[2] = 0;

    int cl_process_col = ((width - 2) >> 2) - 1;
    int cl_process_row = height;
    int remain_col_index = cl_process_col << 2;
    size_t global_work_size[2] = {(size_t)(cl_process_col), (size_t)(cl_process_row)};

    printf("global_work_size=(%zu,%zu)\n", global_work_size[0], global_work_size[1]);
    printf("local_work_size=(%zu,%zu)\n", local_work_size[0], local_work_size[1]);
    gettimeofday(&start, NULL);
    for (int i = 0; i < c_loop_count; i++)
    {
        cl_event kernel_event = NULL;
        err_num = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_work_size,
                                         local_work_size, 0, NULL, &kernel_event);
        CheckClStatus(err_num, "ClEnqueueNDRangeKernel");
        err_num = clWaitForEvents(1, &kernel_event);
        CheckClStatus(err_num, "ClWaitForEvents");
        PrintProfilingInfo(kernel_event);
        clReleaseEvent(kernel_event);
    }
    PrintDuration(&start, "OpenCL Gaussian", c_loop_count);

    err_num = clEnqueueReadBuffer(command_queue, buffer_dst, CL_TRUE, 0, buffer_size_in_bytes,
                                  device_gaussian_matrix, 0, NULL, NULL);
    CheckClStatus(err_num, "clEnqueueReadBuffer");
    err_num = Gauss3x3Sigma0U8C1RemainData(host_src_matrix, remain_col_index, height, width, istride, ostride, device_gaussian_matrix);
    CheckClStatus(err_num, "Gauss3x3Sigma0U8C1RemainData");

    DataCompare(host_gaussian_matrix, device_gaussian_matrix, width, height);

    free(host_src_matrix);
    free(host_gaussian_matrix);
    free(device_gaussian_matrix);
    clReleaseMemObject(buffer_src);
    clReleaseMemObject(buffer_dst);

    CleanUp(context, command_queue, program, kernel);
    return 0;
}

void DataInit(cl_uchar *p_data, int width, int height)
{
    cl_uchar cnt = 0;
    for (int i = 0; i < width * height; i++)
    {
        *p_data = cnt;
        cnt++;
        p_data++;
    }
}

int Gaussian3x3Sigma0U8C1(uint8_t *src, int width, int height, int istride,
                          uint8_t *dst, int ostride)
{
    if ((NULL == src) || (NULL == dst))
    {
        printf("input param invalid!\n");
        return -1;
    }

    for (int row = 0; row < height; row++)
    {
        int last = (row == 0) ? 1 : -1;
        int next  = (row == height - 1) ? -1 : 1;
        uint8_t *src0 = src + (row + last) * istride;
        uint8_t *src1 = src + row * istride;
        uint8_t *src2 = src + (row + next) * istride;

        uint8_t *p_dst = dst + row * ostride;
        for (int col = 0; col < width; col++)
        {
            int left  = (col == 0) ? 1 : ((col == width - 1) ? width - 2 : col - 1);
            int right = (col == 0) ? 1 : ((col == width - 1) ? width - 2 : col + 1);
            uint16_t acc = 0;
            acc += src0[left] + src0[right] + src0[col] * 2;
            acc += (src1[left] + src1[right]) * 2 + src1[col] * 4;
            acc += src2[left] + src2[right] + src2[col] * 2;

            p_dst[col] = ((acc + (1 << 3)) >> 4) & 0xFF;
        }
    }

    return 0;
}

int Gauss3x3Sigma0U8C1RemainData(uint8_t *src, int remain_col_index, int row, int col, int src_pitch,
                                int dst_pitch, uint8_t *dst)
{
    if ((NULL == src) || (NULL == dst))
    {
        printf("Gauss3x3Sigma0U8C1RemainData null ptr\n");
        return -1;
    }

    const uint8_t *p_prev_row = NULL;
    const uint8_t *p_curr_row = NULL;
    const uint8_t *p_next_row = NULL;
    uint8_t *p_out  = dst;

    int i, j;
    unsigned short acc = 0;
    //process first col
    for (i = 0; i < row; i++)
    {
        p_curr_row = src + i * src_pitch;
        p_prev_row = p_curr_row + ((i - 1) < 0 ? 1 : -1) * src_pitch;
        p_next_row = p_curr_row + ((i + 1) > (row - 1) ? -1 : 1) * src_pitch;

        acc = ((p_prev_row[0] + p_prev_row[1]) << 1) +
              ((p_curr_row[0] + p_curr_row[1]) << 2) +
              ((p_next_row[0] + p_next_row[1]) << 1);
        p_out[i * dst_pitch] = ((acc + (1 << 3)) >> 4) & 0xFF;
    }

    //process last col
    for (i = 0; i < row; i++)
    {
        p_curr_row = src + i * src_pitch + col - 1;
        p_prev_row = p_curr_row + ((i - 1) < 0 ? 1 : -1) * src_pitch;
        p_next_row = p_curr_row + ((i + 1) > (row - 1) ? -1 : 1) * src_pitch;

        acc = ((p_prev_row[0] + p_prev_row[-1]) << 1) +
              ((p_curr_row[0] + p_curr_row[-1]) << 2) +
              ((p_next_row[0] + p_next_row[-1]) << 1);
        p_out[i * dst_pitch + col - 1] = ((acc + (1 << 3)) >> 4) & 0xFFFF;
    }

    for (j = 0; j < row; j++)// height
    {
        p_curr_row = src + j * src_pitch;
        p_prev_row = p_curr_row + ((j - 1) < 0 ? 1 : -1) * src_pitch;
        p_next_row = p_curr_row + ((j + 1) > (row - 1) ? -1 : 1) * src_pitch;
        p_out  = dst + j * dst_pitch;
        for (i = remain_col_index; i < col - 1; i++)   // width
        {
            acc = 0;
            acc += p_prev_row[i - 1] + p_prev_row[i + 1] + (p_prev_row[i + 0] << 1) +
                  ((p_curr_row[i - 1] + p_curr_row[i + 1] + (p_curr_row[i + 0] << 1)) << 1) +
                   p_next_row[i - 1] + p_next_row[i + 1] + (p_next_row[i + 0] << 1);
            p_out[i] = ((acc + (1 << 3)) >> 4) & 0xFF;
        }
    }
    return 0;
}

cl_context CreateContext(cl_device_id *p_device)
{
    cl_int err_num;
    cl_uint num_platform;
    cl_platform_id platform_id;
    cl_context context = NULL;
    err_num            = clGetPlatformIDs(1, &platform_id, &num_platform);
    if (CL_SUCCESS != err_num || num_platform <= 0)
    {
        printf("failed to find any opencl platform. \n");
        return NULL;
    }
    err_num = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, p_device, NULL);
    if (CL_SUCCESS != err_num)
    {
        printf("there is no gpu.\n");
        return NULL;
    }
    context = clCreateContext(NULL, 1, p_device, NULL, NULL, &err_num);
    if (CL_SUCCESS != err_num)
    {
        printf("create context error.\n");
        return NULL;
    }
    return context;
}

cl_command_queue CreateCommandQueue(cl_context context, cl_device_id device)
{
    cl_command_queue_properties queue_prop[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    cl_command_queue command_queue           = NULL;
    command_queue = clCreateCommandQueueWithProperties(context, device, queue_prop, NULL);
    if (NULL == command_queue)
    {
        printf("create command queue failed.\n");
    }
    return command_queue;
}

static void PrintBuildProgramInfo(cl_device_id device, cl_program program, const char *name, const char *type)
{
    if ((NULL == program) || (NULL == name) || (NULL == type))
    {
        return;
    }

    cl_int ret = 0;
    size_t log_size = 0;

    ret = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    if (ret != CL_SUCCESS)
    {
        printf("clGetProgramBuildInfo failed.\n ");
        return;
    }

    std::vector<char> log_buf(log_size);

    ret = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log_buf.data(), NULL);
    if (ret != CL_SUCCESS)
    {
        printf("clGetProgramBuildInfo failed.\n ");
        return;
    }

    fprintf(stdout, "program(%s %s) : %s\n", name, type, log_buf.data());
    return;
}

cl_program CreateProgram(cl_context context, cl_device_id device, char *source)
{
    cl_int err_num;
    cl_program program;

    program = clCreateProgramWithSource(context, 1, (const char **)&source, NULL, NULL);
    if (NULL == program)
    {
        PrintBuildProgramInfo(device, program, "program", "build from source");
        printf("create program failed.\n ");
        return NULL;
    }
    err_num = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (CL_SUCCESS != err_num)
    {
        clReleaseProgram(program);
        printf("build program failed.\n ");
        return NULL;
    }
    return program;
}

cl_kernel CreateKernel(cl_program program, const char *kernel_name, cl_device_id device)
{
    int err_num;
    cl_kernel kernel;
    kernel = clCreateKernel(program, kernel_name, &err_num);
    if (err_num != CL_SUCCESS)
    {
        printf("create kernel failed.\n ");
        return NULL;
    }
    size_t max_work_group_size;
    size_t perferred_work_group_size_multiple;
    err_num = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t),
                                       &max_work_group_size, NULL);
    err_num |=
        clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &perferred_work_group_size_multiple, NULL);
    if (err_num != CL_SUCCESS)
    {
        printf("Get kernel info failed.\n ");
        return NULL;
    }
    printf("Kernel %s max workgroup size=%zu\n", kernel_name, max_work_group_size);
    printf("Kernel %s perferred workgroup size multiple=%zu\n", kernel_name,
           perferred_work_group_size_multiple);
    return kernel;
}

void PrintProfilingInfo(cl_event event)
{
    cl_ulong t_queued;
    cl_ulong t_submitted;
    cl_ulong t_started;
    cl_ulong t_ended;
    cl_ulong t_completed;

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &t_queued, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &t_submitted,
                            NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &t_started, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &t_ended, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_COMPLETE, sizeof(cl_ulong), &t_completed,
                            NULL);

    printf("queue -> submit : %fus\n", (t_submitted - t_queued) * 1e-3);
    printf("submit -> start : %fus\n", (t_started - t_submitted) * 1e-3);
    printf("start -> end : %fus\n", (t_ended - t_started) * 1e-3);
    printf("end -> finish : %fus\n", (t_completed - t_ended) * 1e-3);
}

void CleanUp(cl_context context, cl_command_queue commandqueue, cl_program program,
             cl_kernel kernel)
{

    if (NULL != kernel)
    {
        clReleaseKernel(kernel);
    }
    if (NULL != program)
    {
        clReleaseProgram(program);
    }
    if (NULL != commandqueue)
    {
        clReleaseCommandQueue(commandqueue);
    }
    if (NULL != context)
    {
        clReleaseContext(context);
    }
}

void CheckClStatus(cl_int ret, const char *failure_msg)
{
    if (ret != CL_SUCCESS)
    {
        fprintf(stderr, "Error %d with %s\n", ret, failure_msg);
        exit(ret);
    }
    return;
}

void PrintDuration(timeval *begin, const char *function_name, int loop_count)
{
    timeval current;
    gettimeofday(&current, NULL);
    uint64_t time_in_microseconds =
        (current.tv_sec - begin->tv_sec) * 1e6 + (current.tv_usec - begin->tv_usec);
    printf("%s consume average time: %ld us\n", function_name, time_in_microseconds / loop_count);
    return;
}

std::string ClReadString(const std::string &filename)
{
    std::ifstream fs(filename);
    if(!fs.is_open())
    {
        std::cout << "open " << filename << "fail!" << std::endl;
    }
    return std::string((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
}

void ClUtilWriteStringToFile(const cl_uchar *text, size_t text_length, char *filename)
{
    FILE *fp = fopen(filename, "wt+");
    if (NULL == fp)
        return;
    fwrite(text, 1, text_length, fp);
    fclose(fp);
}

void DataCompare(cl_uchar *src1, cl_uchar *src2, int width, int height)
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = i * width + j;
            if (src1[idx] != src2[idx])
            {
                printf("Mismatch at (%d,%d), A= %d,B= %d\n", i, j, src1[idx], src2[idx]);
                return;
            }
        }
    }
    printf("A and B match!\n");
    return;
}
