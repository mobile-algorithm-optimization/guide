#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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

void CpuTranspose(cl_uchar *src, cl_uchar *dst, int src_width, int src_height);
void PrintMatrix(cl_uchar *matrix, int width, int height);

char *ClUtilReadFileToString(const char *filename);
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

    char *device_source_str = ClUtilReadFileToString("kerneltest.cl");
    program                 = CreateProgram(context, device, device_source_str);
    if (NULL == program)
    {
        printf("MainError:Create CommandQueue Failed!\n");
        return -1;
    }

    kernel = CreateKernel(program, "TransposeKernel", device);
    if (NULL == kernel)
    {
        printf("MainError:Create Kernel Failed!\n");
        return -1;
    }

    const int c_loop_count = 30;

    int width                          = 4096;
    int height                         = 4096;
    buffer_size_in_bytes               = width * height * sizeof(cl_uchar);
    cl_uchar *host_src_matrix          = (cl_uchar *)malloc(buffer_size_in_bytes);
    cl_uchar *host_transposed_matrix   = (cl_uchar *)malloc(buffer_size_in_bytes);
    cl_uchar *device_transposed_matrix = (cl_uchar *)malloc(buffer_size_in_bytes);
    memset(device_transposed_matrix, 0, buffer_size_in_bytes);
    DataInit(host_src_matrix, width, height);
    printf("Matrix Width =%d Height=%d\n", width, height);
    gettimeofday(&start, NULL);
    for (int i = 0; i < c_loop_count; i++)
    {
        CpuTranspose(host_src_matrix, host_transposed_matrix, width, height);
    }
    PrintDuration(&start, "Cpu Transpose", c_loop_count);

    buffer_src = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                buffer_size_in_bytes, host_src_matrix, &err_num);
    CheckClStatus(err_num, "Create src buffer");
    buffer_dst = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size_in_bytes, NULL, &err_num);
    CheckClStatus(err_num, "Create dst buffer");

    err_num = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_src);
    err_num |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_dst);
    err_num |= clSetKernelArg(kernel, 2, sizeof(int), &width);
    err_num |= clSetKernelArg(kernel, 3, sizeof(int), &height);
    CheckClStatus(err_num, "Set Kernel Arg");

    size_t global_work_size[3];
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

    global_work_size[0] =
        (width + local_work_size[0] - 1) / local_work_size[0] * local_work_size[0];
    global_work_size[1] =
        (height + local_work_size[1] - 1) / local_work_size[1] * local_work_size[1];
    global_work_size[2] = 0;

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
    PrintDuration(&start, "OpenCL Transpose", c_loop_count);

    err_num = clEnqueueReadBuffer(command_queue, buffer_dst, CL_TRUE, 0, buffer_size_in_bytes,
                                  device_transposed_matrix, 0, NULL, NULL);

    DataCompare(host_transposed_matrix, device_transposed_matrix, width, height);

    free(device_source_str);
    free(host_src_matrix);
    free(host_transposed_matrix);
    free(device_transposed_matrix);
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

void PrintMatrix(cl_uchar *matrix, int width, int height)
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            printf("%d ", matrix[i * width + j]);
        }
        printf("\n");
    }
}

void CpuTranspose(cl_uchar *src, cl_uchar *dst, int src_width, int src_height)
{
    for (int src_row = 0; src_row < src_height; src_row++)
    {
        for (int src_col = 0; src_col < src_width; src_col++)
        {
            dst[src_col * src_height + src_row] = src[src_row * src_width + src_col];
        }
    }
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

cl_program CreateProgram(cl_context context, cl_device_id device, char *source)
{
    cl_int err_num;
    cl_program program;

    program = clCreateProgramWithSource(context, 1, (const char **)&source, NULL, NULL);
    if (NULL == program)
    {
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

char *ClUtilReadFileToString(const char *filename)
{

    FILE *fp;
    char *fileData;
    long fileSize;

    /* Open the file */
    fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Could not open file: %s\n", filename);
        exit(-1);
    }

    /* Determine the file size */
    if (fseek(fp, 0, SEEK_END))
    {
        printf("Error reading the file\n");
        exit(-1);
    }
    fileSize = ftell(fp);
    if (fileSize < 0)
    {
        printf("Error reading the file\n");
        exit(-1);
    }
    if (fseek(fp, 0, SEEK_SET))
    {
        printf("Error reading the file\n");
        exit(-1);
    }

    /* Read the contents */
    fileData = (char *)malloc(fileSize + 1);
    if (!fileData)
    {
        exit(-1);
    }
    if (fread(fileData, fileSize, 1, fp) != 1)
    {
        printf("Error reading the file\n");
        exit(-1);
    }

    /* Terminate the string */
    fileData[fileSize] = '\0';

    /* Close the file */
    if (fclose(fp))
    {
        printf("Error closing the file\n");
        exit(-1);
    }

    return fileData;
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