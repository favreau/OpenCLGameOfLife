/* Copyright (c) 2011-2012, Cyrille Favreau
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 * This file is part of OpenCLGameOfLife
 * <https://github.com/favreau/OpenCLGameOfLife>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <time.h>
#ifdef USE_DIRECTX
#include <CL/cl_d3d10_ext.h>
#endif // USE_DIRECTX

#ifdef ETW_LOGGING
#include <ETWLoggingModule.h>
#include <ETWResources.h>
#else
#define LOG_INFO(msg) std::cout << msg << std::endl;
#define LOG_ERROR(msg) std::cerr << msg << std::endl;
#endif NDEBUG

#include "OpenCLKernel.h"

const long MAX_SOURCE_SIZE = 65535;
const long MAX_DEVICES = 10;

#ifdef USE_DIRECTX
// DirectX
clGetDeviceIDsFromD3D10NV_fn clGetDeviceIDsFromD3D10NV = NULL;
ID3D10Device *g_pd3dDevice = NULL; // Our rendering device
#endif                             // USE_DIRECTX

/*
 * getErrorDesc
 */
std::string getErrorDesc(int err)
{
    switch (err)
    {
    case CL_SUCCESS:
        return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
        return "CL_DEVICE_NOT_FOUND";
    case CL_COMPILER_NOT_AVAILABLE:
        return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
        return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
        return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
        return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
        return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
        return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
        return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
        return "CL_MAP_FAILURE";

    case CL_INVALID_VALUE:
        return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
        return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
        return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
        return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
        return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
        return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
        return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
        return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
        return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
        return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
        return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
        return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
        return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
        return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
        return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
        return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
        return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:
        return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
        return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
        return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
        return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
        return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
        return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
        return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
        return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
        return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
        return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_OPERATION:
        return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
        return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:
        return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
        return "CL_INVALID_MIP_LEVEL";
    default:
        return "UNKNOWN";
    }
}

/*
 * Callback function for clBuildProgram notifications
 */
void pfn_notify(cl_program, void *user_data)
{
    std::stringstream s;
    s << "OpenCL Error (via pfn_notify): " << user_data;
    std::cerr << s.str() << std::endl;
}

/*
 * CHECKSTATUS
 */

#define CHECKSTATUS(stmt)                                        \
    {                                                            \
        int __status = stmt;                                     \
        if (__status != CL_SUCCESS)                              \
        {                                                        \
            std::stringstream __s;                               \
            __s << "==> " #stmt "\n";                            \
            __s << "ERROR : " << getErrorDesc(__status) << "\n"; \
            __s << "<== " #stmt "\n";                            \
            LOG_ERROR(__s.str());                                \
        }                                                        \
    }

/*
 * OpenCLKernel constructor
 */
OpenCLKernel::OpenCLKernel(int platformId, int deviceId, int nbWorkingItems, int draft)
    : m_hContext(0)
    , m_hQueue(0)
    , m_hBitmap(0)
    , m_hBuffer(0)
    , m_hVideo(0)
    , m_hDepth(0)
    , m_hTextures(0)
    , m_textures(0)
    , m_texturedTransfered(false)
    , m_offset(-1)
    , m_timer(0.f)
{
    int status(0);
    cl_platform_id platforms[MAX_DEVICES];
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    char buffer[MAX_SOURCE_SIZE];
    size_t len(0);

    LOG_INFO("clGetPlatformIDs\n");
    CHECKSTATUS(clGetPlatformIDs(MAX_DEVICES, platforms, &ret_num_platforms));

    std::stringstream s;
    // for( int p(0); p<ret_num_platforms; p++)
    int p = platformId;
    {
        // Platform details
        s << "Platform " << p << ":\n";
        CHECKSTATUS(clGetPlatformInfo(platforms[p], CL_PLATFORM_PROFILE, MAX_SOURCE_SIZE, buffer, &len));
        buffer[len] = 0;
        s << "  Profile    : " << buffer << "\n";
        CHECKSTATUS(clGetPlatformInfo(platforms[p], CL_PLATFORM_VERSION, MAX_SOURCE_SIZE, buffer, &len));
        buffer[len] = 0;
        s << "  Version    : " << buffer << "\n";
        CHECKSTATUS(clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, MAX_SOURCE_SIZE, buffer, &len));
        buffer[len] = 0;
        s << "  Name       : " << buffer << "\n";
        CHECKSTATUS(clGetPlatformInfo(platforms[p], CL_PLATFORM_VENDOR, MAX_SOURCE_SIZE, buffer, &len));
        buffer[len] = 0;
        s << "  Vendor     : " << buffer << "\n";
        CHECKSTATUS(clGetPlatformInfo(platforms[p], CL_PLATFORM_VENDOR, MAX_SOURCE_SIZE, buffer, &len));
        buffer[len] = 0;
        s << "  Extensions : " << buffer << "\n";

        CHECKSTATUS(clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 1, m_hDevices, &ret_num_devices));

        // Devices
        int d = deviceId;
        // for( cl_uint d(0); d<ret_num_devices; d++ )
        {
            s << "  Device " << d << ":\n";

            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL));
            s << "    DEVICE_NAME                        : " << buffer << "\n";
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL));
            s << "    DEVICE_VENDOR                      : " << buffer << "\n";
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL));
            s << "    DEVICE_VERSION                     : " << buffer << "\n";
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL));
            s << "    DRIVER_VERSION                     : " << buffer << "\n";

            cl_uint value;
            cl_uint values[10];
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(value), &value, NULL));
            s << "    DEVICE_MAX_COMPUTE_UNITS           : " << value << "\n";
            CHECKSTATUS(
                clGetDeviceInfo(m_hDevices[d], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(value), &value, NULL));
            s << "    CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS : " << value << "\n";
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(value), &value, NULL));
            s << "    CL_DEVICE_MAX_WORK_GROUP_SIZE      : " << value << "\n";
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(values), &values, NULL));
            s << "    CL_DEVICE_MAX_WORK_ITEM_SIZES      : " << values[0] << ", " << values[1] << ", " << values[2]
              << "\n";
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(value), &value, NULL));
            s << "    CL_DEVICE_MAX_CLOCK_FREQUENCY      : " << value << "\n";

            cl_device_type infoType;
            CHECKSTATUS(clGetDeviceInfo(m_hDevices[d], CL_DEVICE_TYPE, sizeof(infoType), &infoType, NULL));
            s << "    DEVICE_TYPE                        : ";
            if (infoType & CL_DEVICE_TYPE_DEFAULT)
            {
                infoType &= ~CL_DEVICE_TYPE_DEFAULT;
                s << "Default";
            }
            if (infoType & CL_DEVICE_TYPE_CPU)
            {
                infoType &= ~CL_DEVICE_TYPE_CPU;
                s << "CPU";
            }
            if (infoType & CL_DEVICE_TYPE_GPU)
            {
                infoType &= ~CL_DEVICE_TYPE_GPU;
                s << "GPU";
            }
            if (infoType & CL_DEVICE_TYPE_ACCELERATOR)
            {
                infoType &= ~CL_DEVICE_TYPE_ACCELERATOR;
                s << "Accelerator";
            }
            if (infoType != 0)
            {
                s << "Unknown ";
                s << infoType;
            }
        }
        s << "\n";
    }
    std::cout << s.str() << std::endl;
    LOG_INFO(s.str());

    m_hContext = clCreateContext(NULL, ret_num_devices, &m_hDevices[0], NULL, NULL, &status);
    m_hQueue = clCreateCommandQueue(m_hContext, m_hDevices[0], CL_QUEUE_PROFILING_ENABLE, &status);
}

/*
 * compileKernels
 */
void OpenCLKernel::compileKernels(const KernelSourceType sourceType, const std::string &source,
                                  const std::string &ptxFileName, const std::string &options)
{
    try
    {
        int status(0);
        cl_program hProgram(0);
        clUnloadCompiler();

        const char *source_str;
        size_t len(0);
        switch (sourceType)
        {
        case kst_file:
            if (source.length() != 0)
            {
                source_str = loadFromFile(source, len);
            }
            break;
        case kst_string:
        {
            source_str = source.c_str();
            len = source.length();
        }
        break;
        }

        LOG_INFO("clCreateProgramWithSource\n");
        hProgram = clCreateProgramWithSource(m_hContext, 1, (const char **)&source_str, (const size_t *)&len, &status);
        CHECKSTATUS(status);

        LOG_INFO("clCreateProgramWithSource\n");
        hProgram = clCreateProgramWithSource(m_hContext, 1, (const char **)&source_str, (const size_t *)&len, &status);
        CHECKSTATUS(status);

        LOG_INFO("clBuildProgram\n");
        CHECKSTATUS(clBuildProgram(hProgram, 0, NULL, options.c_str(), NULL, NULL));

        if (sourceType == kst_file)
        {
            delete[] source_str;
            source_str = NULL;
        }

        clUnloadCompiler();

        LOG_INFO("clCreateKernel(main_kernel)\n");
        m_hMainKernel = clCreateKernel(hProgram, "main_kernel", &status);
        CHECKSTATUS(status);

        // if( m_computeUnits == 0 )
        {
            clGetKernelWorkGroupInfo(m_hMainKernel, m_hDevices[0], CL_KERNEL_WORK_GROUP_SIZE, sizeof(m_computeUnits),
                                     &m_computeUnits, NULL);
            std::cout << "CL_KERNEL_WORK_GROUP_SIZE=" << m_computeUnits << std::endl;
        }

        clGetKernelWorkGroupInfo(m_hMainKernel, m_hDevices[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(m_preferredWorkGroupSize), &m_preferredWorkGroupSize, NULL);
        std::cout << "CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE=" << m_preferredWorkGroupSize << std::endl;

        char buffer[MAX_SOURCE_SIZE];
        LOG_INFO("clGetProgramBuildInfo\n");
        CHECKSTATUS(clGetProgramBuildInfo(hProgram, m_hDevices[0], CL_PROGRAM_BUILD_LOG, MAX_SOURCE_SIZE * sizeof(char),
                                          &buffer, &len));

        if (buffer[0] != 0)
        {
            buffer[len] = 0;
            std::stringstream s;
            s << buffer;
            LOG_INFO(s.str());
            std::cout << s.str() << std::endl;
        }
#if 0
		// Generate Binaries!!!
		// Obtain the length of the binary data that will be queried, for each device
		size_t ret_num_devices = 1;
		size_t binaries_sizes[MAX_DEVICES];
		CHECKSTATUS(clGetProgramInfo(
			hProgram,
			CL_PROGRAM_BINARY_SIZES,
			ret_num_devices * sizeof(size_t),
			binaries_sizes,
			0));

		char **binaries = new char*[MAX_DEVICES];
		for (size_t i = 0; i < ret_num_devices; i++)
			binaries[i] = new char[binaries_sizes[i] + 1];

		CHECKSTATUS(clGetProgramInfo(
			hProgram,
			CL_PROGRAM_BINARIES,
			MAX_DEVICES * sizeof(size_t),
			binaries,
			NULL));

		for (size_t i = 0; i < ret_num_devices; i++) {
			binaries[i][binaries_sizes[i]] = '\0';
			char name[255];
			sprintf_s(name, 255, "kernel%d.ptx", i);
			FILE* fp = NULL;
			fopen_s(&fp, name, "w");
			fwrite(binaries[i], 1, binaries_sizes[i], fp);
			fclose(fp);
		}

		for (size_t i = 0; i < ret_num_devices; i++)
			delete[] binaries[i];
		delete[] binaries;
#endif // 0

        if (ptxFileName.length() != 0)
        {
            // Open the ptx file and load it
            // into a char* buffer
            std::ifstream myReadFile;
            std::string str;
            std::string line;
            std::ifstream myfile(ptxFileName.c_str());
            if (myfile.is_open())
            {
                while (myfile.good())
                {
                    std::getline(myfile, line);
                    str += '\n' + line;
                }
                myfile.close();
            }

            size_t lSize = str.length();
            char *buffer = new char[lSize + 1];
            strcpy_s(buffer, lSize, str.c_str());

            // Build the rendering kernel
            int errcode(0);
            hProgram = clCreateProgramWithBinary(m_hContext, 1, &m_hDevices[0], &lSize, (const unsigned char **)&buffer,
                                                 &status, &errcode);
            CHECKSTATUS(errcode);

            CHECKSTATUS(clBuildProgram(hProgram, 0, NULL, "", NULL, NULL));
            m_hMainKernel = clCreateKernel(hProgram, "main_kernel", &status);
            CHECKSTATUS(status);

            delete[] buffer;
        }

        LOG_INFO("clReleaseProgram\n");
        CHECKSTATUS(clReleaseProgram(hProgram));
        hProgram = 0;
    }
    catch (...)
    {
        LOG_ERROR("Unexpected exception\n");
    }
}

void OpenCLKernel::initializeDevice(int width, int height)
{
    int status(0);

    // Setup device memory
    LOG_INFO("Setup device memory\n");
    m_hBitmap = clCreateBuffer(m_hContext, CL_MEM_WRITE_ONLY, width * height * sizeof(BYTE) * gColorDepth, 0, NULL);
    m_hBuffer = clCreateBuffer(m_hContext, CL_MEM_READ_WRITE, 2 * width * height * sizeof(cl_float4), 0, NULL);
    m_hTextures = clCreateBuffer(m_hContext, CL_MEM_READ_ONLY,
                                 gTextureWidth * gTextureHeight * gTextureDepth * sizeof(BYTE), 0, NULL);
    m_hVideo = clCreateBuffer(m_hContext, CL_MEM_READ_ONLY, gVideoWidth * gVideoHeight * gKinectColorVideo, 0, NULL);
    m_hDepth = clCreateBuffer(m_hContext, CL_MEM_READ_ONLY, gDepthWidth * gDepthHeight * gKinectColorDepth, 0, NULL);

    // Setup World
    m_textures = new BYTE[gTextureWidth * gTextureHeight * gColorDepth];
}

void OpenCLKernel::releaseDevice()
{
    LOG_INFO("Release device memory\n");
    if (m_hTextures)
        CHECKSTATUS(clReleaseMemObject(m_hTextures));

    if (m_hBitmap)
        CHECKSTATUS(clReleaseMemObject(m_hBitmap));
    if (m_hBuffer)
        CHECKSTATUS(clReleaseMemObject(m_hBuffer));
    if (m_hVideo)
        CHECKSTATUS(clReleaseMemObject(m_hVideo));
    if (m_hDepth)
        CHECKSTATUS(clReleaseMemObject(m_hDepth));

    if (m_hMainKernel)
        CHECKSTATUS(clReleaseKernel(m_hMainKernel));

    if (m_hQueue)
        CHECKSTATUS(clReleaseCommandQueue(m_hQueue));
    if (m_hContext)
        CHECKSTATUS(clReleaseContext(m_hContext));

    delete m_textures;
}

/*
 * runKernel
 */
void OpenCLKernel::render(const unsigned width, const unsigned int height, BYTE *bitmap, const float value)
{
    int status(0);

    BYTE *video(0);
    BYTE *depth(0);

    // Textures
    if (!m_texturedTransfered)
    {
        CHECKSTATUS(clEnqueueWriteBuffer(m_hQueue, m_hTextures, CL_TRUE, 0,
                                         gTextureDepth * gTextureWidth * gTextureHeight, m_textures, 0, NULL, NULL));
        m_texturedTransfered = true;
    }

    // Kinect stuff
    if (video)
        CHECKSTATUS(clEnqueueWriteBuffer(m_hQueue, m_hVideo, CL_TRUE, 0, gKinectColorVideo * gVideoWidth * gVideoHeight,
                                         video, 0, NULL, NULL));
    if (depth)
        CHECKSTATUS(clEnqueueWriteBuffer(m_hQueue, m_hDepth, CL_TRUE, 0, gKinectColorDepth * gDepthWidth * gDepthHeight,
                                         depth, 0, NULL, NULL));

    // Setting kernel arguments
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 0, sizeof(cl_int), (void *)&width));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 1, sizeof(cl_int), (void *)&height));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 2, sizeof(cl_mem), (void *)&m_hBitmap));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 3, sizeof(cl_mem), (void *)&m_hBuffer));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 4, sizeof(cl_mem), (void *)&m_hVideo));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 5, sizeof(cl_mem), (void *)&m_hDepth));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 6, sizeof(cl_mem), (void *)&m_hTextures));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 7, sizeof(cl_int), (void *)&m_offset));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 8, sizeof(cl_float), (void *)&value));
    CHECKSTATUS(clSetKernelArg(m_hMainKernel, 9, sizeof(cl_float), (void *)&m_timer));

    size_t globalWorkSize[] = {width, height};
    // run initial kernel
    CHECKSTATUS(clEnqueueNDRangeKernel(m_hQueue, m_hMainKernel, 2, NULL, globalWorkSize, 0, 0, 0, 0));

    // ------------------------------------------------------------
    // Read back the results
    // ------------------------------------------------------------
    // Bitmap
    if (bitmap != 0)
    {
        CHECKSTATUS(clEnqueueReadBuffer(m_hQueue, m_hBitmap, CL_FALSE, 0, width * height * sizeof(BYTE) * gColorDepth,
                                        bitmap, 0, NULL, NULL));
    }

    CHECKSTATUS(clFlush(m_hQueue));
    CHECKSTATUS(clFinish(m_hQueue));

    if (m_offset == -1)
        m_offset = 1;
    m_offset = (m_offset == 0) ? 1 : 0;

    m_timer += 0.1f;
}

/*
 *
 */
OpenCLKernel::~OpenCLKernel()
{
    // Clean up
    releaseDevice();
}

// ---------- Textures ----------
void OpenCLKernel::setTexture(int index, BYTE *texture)
{
    BYTE *idx = m_textures + index * gTextureWidth * gTextureHeight * gTextureDepth;
    int j(0);
    for (int i(0); i < gTextureWidth * gTextureHeight * gColorDepth; i += gColorDepth)
    {
        idx[j] = texture[i + 2];
        idx[j + 1] = texture[i + 1];
        idx[j + 2] = texture[i];
        j += gTextureDepth;
    }
}

/*
 *
 */
char *OpenCLKernel::loadFromFile(const std::string &filename, size_t &length)
{
    // Load the kernel source code into the array source_str
    FILE *fp = 0;
    char *source_str = 0;

    fopen_s(&fp, filename.c_str(), "r");
    if (fp == 0)
    {
        std::cout << "Failed to load kernel " << filename.c_str() << std::endl;
    }
    else
    {
        source_str = (char *)malloc(MAX_SOURCE_SIZE);
        length = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);
    }
    return source_str;
}

// ---------- Kinect ----------
long OpenCLKernel::addTexture(const std::string &filename)
{
    FILE *filePtr(0);                  // our file pointer
    BITMAPFILEHEADER bitmapFileHeader; // our bitmap file header
    unsigned char *bitmapImage;        // store image data
    BITMAPINFOHEADER bitmapInfoHeader;
    DWORD imageIdx = 0;    // image index counter
    unsigned char tempRGB; // our swap variable

    // open filename in read binary mode
    fopen_s(&filePtr, filename.c_str(), "rb");
    if (filePtr == NULL)
    {
        return 1;
    }

    // read the bitmap file header
    fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

    // verify that this is a bmp file by check bitmap id
    if (bitmapFileHeader.bfType != 0x4D42)
    {
        fclose(filePtr);
        return 1;
    }

    // read the bitmap info header
    fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

    // move file point to the begging of bitmap data
    fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

    // allocate enough memory for the bitmap image data
    bitmapImage = (unsigned char *)malloc(bitmapInfoHeader.biSizeImage);

    // verify memory allocation
    if (!bitmapImage)
    {
        free(bitmapImage);
        fclose(filePtr);
        return 1;
    }

    // read in the bitmap image data
    fread(bitmapImage, bitmapInfoHeader.biSizeImage, 1, filePtr);

    // make sure bitmap image data was read
    if (bitmapImage == NULL)
    {
        fclose(filePtr);
        return NULL;
    }

    // swap the r and b values to get RGB (bitmap is BGR)
    for (imageIdx = 0; imageIdx < bitmapInfoHeader.biSizeImage; imageIdx += 3)
    {
        tempRGB = bitmapImage[imageIdx];
        bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
        bitmapImage[imageIdx + 2] = tempRGB;
    }

    // close file and return bitmap image data
    fclose(filePtr);

    memcpy(m_textures, bitmapImage, bitmapInfoHeader.biSizeImage);

    free(bitmapImage);
    return 1;
}
