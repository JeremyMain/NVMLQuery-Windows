/*
 * MIT License
 * 
 * Copyright (c) 2016 Jeremy Main (jmain.jp@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// nvmlquery.cpp : Demonstrate how to load the NVML API and query GPU utilization metrics

#include <Windows.h>
#include <stdio.h>
// The NVML.h header file can be downloaded as part of the NVML SDK from the NVIDIA web site
// ref: https://developer.nvidia.com/nvidia-management-library-nvml
// 
#include "nvml/nvml.h"

// The default path to the NVML DLL
#define NVMLQUERY_DEFAULT_NVML_DLL_PATH "C:\\Program Files\\NVIDIA Corporation\\NVSMI\\NVML.DLL"

// NVML Function pointer prototypes
typedef nvmlReturn_t (*PFNnvmlInit)(void);
typedef nvmlReturn_t (*PFNnvmlShutdown)(void);
typedef char*        (*PFNnvmlErrorString)(nvmlReturn_t result);
typedef nvmlReturn_t (*PFNnvmlDeviceGetCount)(unsigned int* deviceCount);
typedef nvmlReturn_t (*PFNnvmlDeviceGetHandleByIndex)(unsigned int index, nvmlDevice_t* device);
typedef nvmlReturn_t (*PFNnvmlDeviceGetUtilizationRates)(nvmlDevice_t device, nvmlUtilization_t* utilization);
typedef nvmlReturn_t (*PFNnvmlDeviceGetEncoderUtilization)(nvmlDevice_t device, unsigned int* utilization, unsigned int* samplingPeriodUs);
typedef nvmlReturn_t (*PFNnvmlDeviceGetDecoderUtilization)(nvmlDevice_t device, unsigned int* utilization, unsigned int* samplingPeriodUs);
typedef nvmlReturn_t (*PFNnvmlDeviceGetMemoryInfo)(nvmlDevice_t device, nvmlMemory_t* memory);
typedef nvmlReturn_t (*PFNnvmlDeviceGetName)(nvmlDevice_t device, char* name, unsigned int length);

// NVML Function pointer instances
PFNnvmlInit pfn_nvmlInit = NULL ;   
PFNnvmlShutdown pfn_nvmlShutdown = NULL ;
PFNnvmlErrorString pfn_nvmlErrorString = NULL ;
PFNnvmlDeviceGetCount pfn_nvmlDeviceGetCount = NULL ;    
PFNnvmlDeviceGetHandleByIndex pfn_nvmlDeviceGetHandleByIndex = NULL ;
PFNnvmlDeviceGetUtilizationRates pfn_nvmlDeviceGetUtilizationRates = NULL ;
PFNnvmlDeviceGetEncoderUtilization pfn_nvmlDeviceGetEncoderUtilization = NULL ;
PFNnvmlDeviceGetDecoderUtilization pfn_nvmlDeviceGetDecoderUtilization = NULL ;
PFNnvmlDeviceGetMemoryInfo pfn_nvmlDeviceGetMemoryInfo = NULL ;
PFNnvmlDeviceGetName pfn_nvmlDeviceGetName = NULL ;

// display information about the calling function and related error
void ShowErrorDetails(const nvmlReturn_t nvRetVal, const char* pFunctionName)
{
	char* pErrorDescription = NULL ;
	pErrorDescription = pfn_nvmlErrorString(nvRetVal) ;
	printf("[%s] - %s\r\n", pFunctionName, pErrorDescription) ;
}

// Application entry point
int main(int argc, char* argv[])
{
	int iRetValue = -1 ;
	nvmlReturn_t nvRetValue = NVML_ERROR_UNINITIALIZED ;

	// Load the NVML DLL using the default NVML DLL install path
	// NOTE: This DLL is included in the NVIDIA driver installation by default
	HINSTANCE hDLLhandle = NULL ;
	hDLLhandle = LoadLibrary(NVMLQUERY_DEFAULT_NVML_DLL_PATH) ;

	// if the DLL can not be found, exit
	if (NULL == hDLLhandle)
	{
		printf("NVML DLL is not installed or not found at the default path.\r\n") ;
		return iRetValue ;
	}

	// Get the function pointers from the DLL
    pfn_nvmlInit = (PFNnvmlInit)GetProcAddress(hDLLhandle, "nvmlInit") ;   
    pfn_nvmlShutdown = (PFNnvmlShutdown)GetProcAddress(hDLLhandle, "nvmlShutdown") ;   
	pfn_nvmlErrorString = (PFNnvmlErrorString)GetProcAddress(hDLLhandle,  "nvmlErrorString") ;
	pfn_nvmlDeviceGetCount = (PFNnvmlDeviceGetCount)GetProcAddress(hDLLhandle, "nvmlDeviceGetCount") ;    
	pfn_nvmlDeviceGetHandleByIndex = (PFNnvmlDeviceGetHandleByIndex)GetProcAddress(hDLLhandle, "nvmlDeviceGetHandleByIndex") ;
	pfn_nvmlDeviceGetName = (PFNnvmlDeviceGetName)GetProcAddress(hDLLhandle, "nvmlDeviceGetName") ;
	pfn_nvmlDeviceGetUtilizationRates = (PFNnvmlDeviceGetUtilizationRates)GetProcAddress(hDLLhandle, "nvmlDeviceGetUtilizationRates") ;
	pfn_nvmlDeviceGetEncoderUtilization = (PFNnvmlDeviceGetEncoderUtilization)GetProcAddress(hDLLhandle, "nvmlDeviceGetEncoderUtilization") ;
	pfn_nvmlDeviceGetDecoderUtilization = (PFNnvmlDeviceGetDecoderUtilization)GetProcAddress(hDLLhandle, "nvmlDeviceGetDecoderUtilization") ;
	pfn_nvmlDeviceGetMemoryInfo = (PFNnvmlDeviceGetMemoryInfo)GetProcAddress(hDLLhandle, "nvmlDeviceGetMemoryInfo") ;
	

	// Before any of the NVML functions can be used nvmlInit() must be called
	nvRetValue = pfn_nvmlInit() ;

	if (NVML_SUCCESS != nvRetValue)
	{
		// Can not call the NVML specific error string handler if the initialization failed
		printf ("[%s] error code :%d\r\n", "nvmlInit", nvRetValue) ;
		FreeLibrary(hDLLhandle) ;
		hDLLhandle = NULL ;
		return iRetValue ;
	}

	// Now that NVML has been initalized, before exiting normally or when handling 
	// an error condition, ensure nvmlShutdown() is called

	// For each of the GPUs detected by NVML, query the device name, GPU, GPU memory, video encoder and decoder utilization

	// Get the number of GPUs
	unsigned int uiNumGPUs = 0 ;
	nvRetValue = pfn_nvmlDeviceGetCount(&uiNumGPUs) ;

	if (NVML_SUCCESS != nvRetValue)
	{
		ShowErrorDetails(nvRetValue, "nvmlDeviceGetCount") ;
		pfn_nvmlShutdown() ;
		FreeLibrary(hDLLhandle) ;
		hDLLhandle = NULL ;
		return iRetValue ;
	}

	// In the case that no GPUs were detected
	if (0 == uiNumGPUs)
	{
		printf("No NVIDIA GPUs were detected.\r\n") ;
		pfn_nvmlShutdown() ;
		FreeLibrary(hDLLhandle) ;
		hDLLhandle = NULL ;
		return iRetValue ;
	}

	// Flags to denote unsupported queries
	bool bGPUUtilSupported = true ;
	bool bEncoderUtilSupported = true ;
	bool bDecoderUtilSupported = true ;

	// Print out a header for the utilization output
	printf("Device #, Name, GPU(%%), Frame Buffer(%%), Video Encode(%%), Video Decode(%%)\r\n") ;

	// Iterate through all of the GPUs
	for (unsigned int iDevIDX = 0; iDevIDX < uiNumGPUs; iDevIDX++)
	{
		// Get the GPU device handle
		nvmlDevice_t nvGPUDeviceHandle = NULL ;
		nvRetValue = pfn_nvmlDeviceGetHandleByIndex(iDevIDX, &nvGPUDeviceHandle) ;

		if (NVML_SUCCESS != nvRetValue)
		{
			ShowErrorDetails(nvRetValue, "nvmlDeviceGetHandleByIndex") ;
			pfn_nvmlShutdown() ;
			FreeLibrary(hDLLhandle) ;
			hDLLhandle = NULL ;
			return iRetValue ;
		}

		// Get the device name
		char cDevicename[NVML_DEVICE_NAME_BUFFER_SIZE] = {'\0'} ;
		nvRetValue = pfn_nvmlDeviceGetName(nvGPUDeviceHandle,  cDevicename,  NVML_DEVICE_NAME_BUFFER_SIZE) ;

		if (NVML_SUCCESS != nvRetValue)
		{
			ShowErrorDetails(nvRetValue, "nvmlDeviceGetName") ;
			pfn_nvmlShutdown() ;
			FreeLibrary(hDLLhandle) ;
			hDLLhandle = NULL ;
			return iRetValue ;
		}

		// NOTE: nvUtil.memory is the memory controller utilization not the frame buffer utilization
		nvmlUtilization_t nvUtilData ;
		nvRetValue  = pfn_nvmlDeviceGetUtilizationRates(nvGPUDeviceHandle, &nvUtilData) ;
		if (NVML_SUCCESS != nvRetValue)
		{
			// Where the GPU utilization is not supported, the query will return an "Not Supported", handle it and continue
			if (NVML_ERROR_NOT_SUPPORTED == nvRetValue)
			{
				bGPUUtilSupported = false ;
			}
			else
			{
				ShowErrorDetails(nvRetValue, "nvmlDeviceGetUtilizationRates") ;
				pfn_nvmlShutdown() ;
				FreeLibrary(hDLLhandle) ;
				hDLLhandle = NULL ;
				return iRetValue ;
			}
		}

		// Get the GPU frame buffer memory information
		nvmlMemory_t GPUmemoryInfo ;
        ZeroMemory(&GPUmemoryInfo,  sizeof(GPUmemoryInfo)) ;
        nvRetValue = pfn_nvmlDeviceGetMemoryInfo(nvGPUDeviceHandle, &GPUmemoryInfo) ;
		if (NVML_SUCCESS != nvRetValue)
		{
			ShowErrorDetails(nvRetValue, "nvmlDeviceGetMemoryInfo") ;
			pfn_nvmlShutdown() ;
			FreeLibrary(hDLLhandle) ;
			hDLLhandle = NULL ;
			return iRetValue ;
		}

		// compute the amount of frame buffer memory that has been used
		unsigned long long ullFrameBufferUsedBytes = 0L;
		ullFrameBufferUsedBytes = GPUmemoryInfo.total - GPUmemoryInfo.free ;

		unsigned long ulFrameBufferTotalKBytes = 0L;
		unsigned long ulFrameBufferUsedKBytes = 0L;

		// verify that the unsigned long long to unsigned long cast will not result in lost data
		if (ULONG_MAX < GPUmemoryInfo.total)
		{
			printf("ERROR: GPU memory size exceeds variable limit\r\n") ;
			pfn_nvmlShutdown() ;
			FreeLibrary(hDLLhandle) ;
			hDLLhandle = NULL ;
			return iRetValue ;
		}

		// convert the frame buffer value to KBytes
		ulFrameBufferTotalKBytes = (unsigned long)(GPUmemoryInfo.total / 1024L) ;
		ulFrameBufferUsedKBytes = (unsigned long)(ulFrameBufferTotalKBytes - (GPUmemoryInfo.free / 1024L)) ; 

		// calculate the frame buffer memory utilization
		double dMemUtilzation = (((double)ulFrameBufferUsedKBytes / (double)ulFrameBufferTotalKBytes) * 100.0) ;

		// Get the video encoder utilization (where supported)
		unsigned int uiVidEncoderUtil = 0u ;
		unsigned int uiVideEncoderLastSample = 0u ;
		nvRetValue = pfn_nvmlDeviceGetEncoderUtilization(nvGPUDeviceHandle, &uiVidEncoderUtil, &uiVideEncoderLastSample) ;
		if (NVML_SUCCESS != nvRetValue)
		{
			// Where the video encoder utilization is not supported, the query will return an "Not Supported", handle it and continue
			if (NVML_ERROR_NOT_SUPPORTED == nvRetValue)
			{
				bEncoderUtilSupported = false ;
			}
			else
			{
				ShowErrorDetails(nvRetValue, "nvmlDeviceGetEncoderUtilization") ;
				pfn_nvmlShutdown() ;
				FreeLibrary(hDLLhandle) ;
				hDLLhandle = NULL ;
				return iRetValue ;
			}
		}

		// Get the video decoder utilization (where supported)
		unsigned int uiVidDecoderUtil = 0u ;
		unsigned int uiVidDecoderLastSample = 0u ;
		nvRetValue = pfn_nvmlDeviceGetDecoderUtilization(nvGPUDeviceHandle, &uiVidDecoderUtil, &uiVidDecoderLastSample) ;
		if (NVML_SUCCESS != nvRetValue)
		{
			// Where the video decoder utilization is not supported, the query will return an "Not Supported", handle it and continue
			if (NVML_ERROR_NOT_SUPPORTED == nvRetValue)
			{
				bDecoderUtilSupported = false ;
			}
			else
			{
				ShowErrorDetails(nvRetValue, "nvmlDeviceGetDecoderUtilization") ;
				pfn_nvmlShutdown() ;
				FreeLibrary(hDLLhandle) ;
				hDLLhandle = NULL ;
				return iRetValue ;
			}
		}

		// Output the utilization results depending on which of the counters has data available
		// I have opted to display "-" to denote an unsupported value rather than simply display "0"
		// to clarify that the GPU/driver does not support the query. 
		if (!bEncoderUtilSupported || !bDecoderUtilSupported)
		{
			if (!bGPUUtilSupported)
			{
				printf("Device %d, %s, -, %.0f, -, -\r\n",  iDevIDX,  cDevicename, dMemUtilzation) ;
			}
			else
			{
				printf("Device %d, %s, %d, %.0f, -, -\r\n",  iDevIDX,  cDevicename, nvUtilData.gpu, dMemUtilzation) ;
			}
		}
		else
		{
			if (!bGPUUtilSupported)
			{
				printf("Device %d, %s, -, %.0f, %d, %d\r\n",  iDevIDX,  cDevicename, dMemUtilzation, uiVidEncoderUtil, uiVidDecoderUtil) ;
			}
			else
			{
				printf("Device %d, %s, %d, %.0f, %d, %d\r\n",  iDevIDX,  cDevicename, nvUtilData.gpu, dMemUtilzation, uiVidEncoderUtil, uiVidDecoderUtil) ;
			}
		}
	}

	// Shutdown NVML
	nvRetValue = pfn_nvmlShutdown() ;
	if (NVML_SUCCESS != nvRetValue)
	{
		ShowErrorDetails(nvRetValue,  "nvmlShutdown") ;
	}

	// release the DLL handle
	FreeLibrary(hDLLhandle) ;
	hDLLhandle = NULL ;
	return iRetValue ;

	iRetValue = (NVML_SUCCESS == nvRetValue) ? 0 : -1 ;  
	return iRetValue;
}

