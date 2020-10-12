/*
* Copyright 1993-2017 NVIDIA Corporation.  All rights reserved.
*
* NOTICE TO LICENSEE:
*
* This source code and/or documentation ("Licensed Deliverables") are
* subject to NVIDIA intellectual property rights under U.S. and
* international Copyright laws.
*
* These Licensed Deliverables contained herein is PROPRIETARY and
* CONFIDENTIAL to NVIDIA and is being provided under the terms and
* conditions of a form of NVIDIA software license agreement by and
* between NVIDIA and Licensee ("License Agreement") or electronically
* accepted by Licensee.  Notwithstanding any terms or conditions to
* the contrary in the License Agreement, reproduction or disclosure
* of the Licensed Deliverables to any third party without the express
* written consent of NVIDIA is prohibited.
*
* NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
* LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
* SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
* PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
* NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
* DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
* NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
* NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
* LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
* SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
* DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
* WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
* OF THESE LICENSED DELIVERABLES.
*
* U.S. Government End Users.  These Licensed Deliverables are a
* "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
* 1995), consisting of "commercial computer software" and "commercial
* computer software documentation" as such terms are used in 48
* C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
* only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
* 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
* U.S. Government End Users acquire the Licensed Deliverables with
* only those rights set forth herein.
*
* Any use of the Licensed Deliverables in individual and commercial
* software must include, in the user documentation and internal
* comments to the code, the above Disclaimer and U.S. Government End
* Users Notice.
*/
#include "stream.h"
using namespace streamer;
#include "app.h"
#include "windows.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <memory>
#include "cuda.h"
#include "cuda_runtime.h"
#include "vector_types.h"

//#include <opencv2/gpu/gpu.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include<opencv2\opencv.hpp>

#include "image_io_util.hpp"
#define  camera_num   (6)
#define cam_width    (1024)    //720P//1440*900 1080P
#define cam_length    (576)
#define encbitrate    (2000000)
using std::chrono::milliseconds;
using std::chrono::high_resolution_clock;
int apiBackend = cv::CAP_DSHOW;    //caputre the video stream from multiple cameras
#define cap_frame_width  (1024)     //need to be modified when change the stitching result resolution//to be streamed 1920*960  1024*512
#define cap_frame_height (576)
#define stream_fps_value       (30)
using namespace std;
ofstream test;
class MovingAverage
{
	int size;
	int pos;
	bool crossed;
	std::vector<double> v;

public:
	explicit MovingAverage(int sz)
	{
		size = sz;
		v.resize(size);
		pos = 0;
		crossed = false;
	}

	void add_value(double value)
	{
		v[pos] = value;
		pos++;
		if (pos == size) {
			pos = 0;
			crossed = true;
		}
	}

	double get_average()
	{
		double avg = 0.0;
		int last = crossed ? size : pos;
		int k = 0;
		for (k = 0; k < last; k++) {
			avg += v[k];
		}
		return avg / (double)last;
	}
};

void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

static void add_delay(size_t streamed_frames, size_t fps, double elapsed, double avg_frame_time)
{
	//compute min number of frames that should have been streamed based on fps and elapsed
	double dfps = fps;
	size_t min_streamed = (size_t)(dfps*elapsed);
	size_t min_plus_margin = min_streamed + 2;

	if (streamed_frames > min_plus_margin) {
		size_t excess = streamed_frames - min_plus_margin;
		double dexcess = excess;

		//add a delay ~ excess*processing_time
//#define SHOW_DELAY
#ifdef SHOW_DELAY
		double delay = dexcess * avg_frame_time*1000000.0;
		printf("frame %07lu adding delay %.4f\n", streamed_frames, delay);
		printf("avg fps = %.2f\n", streamed_frames / elapsed);
#endif
		usleep(dexcess*avg_frame_time*1000000.0);
	}
}

void process_frame(const cv::Mat &in, cv::Mat &out)
{
	in.copyTo(out);
}



nvstitchResult
app::RunZeus(appParams *params)
{
	int num_gpus;
	bool from_camera = false;
	nvstitchImageBuffer_t input_image[6];
	auto address = input_image;
	//unique_ptr<unsigned char[]> retval[6];
	cudaGetDeviceCount(&num_gpus);
	SYSTEMTIME sendingtime;
	wchar_t szTime[100];
	

	std::chrono::high_resolution_clock clk;
	std::chrono::high_resolution_clock::time_point time_start = clk.now();
	std::chrono::high_resolution_clock::time_point time_stop = time_start;
	
	//std::chrono::high_resolution_clock::time_point time_display = time_start;
	std::chrono::high_resolution_clock::time_point time_process = time_start;
	std::chrono::high_resolution_clock::time_point time_prev = time_start;
	std::chrono::high_resolution_clock::time_point time_prev_display = time_start;
	std::chrono::high_resolution_clock::time_point time_prev_process = time_start;
	std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_start);
	static int total_elapse_time = 0;
	std::chrono::duration<double> frame_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_prev);
	int stream_fps = stream_fps_value;  // we can setup the value here
	int bitrate = encbitrate;  ////////////

	ofstream myfile, testfile;
	myfile.open("E:\\Junproject\\result.txt", ofstream::app);
	
	testfile.open("E:\\Junproject\\microc.txt", ofstream::app);
	
	//myfile << "test\n";
	//myfile.close();
	Streamer stream;
	StreamerConfig streamer_config(cap_frame_width, cap_frame_height,
		cap_frame_width, cap_frame_height,
		stream_fps, bitrate, "high444", "rtmp://128.205.39.252:1935/live/test");  //need to be replaced with your URL
	stream.enable_av_debug_log();
	stream.init(streamer_config);

	size_t streamed_frames = 0;


	MovingAverage moving_average(10);
	double avg_frame_time;
	VideoCapture capone(apiBackend + 0);
	
	capone.set(CV_CAP_PROP_FRAME_WIDTH, cam_width);
	capone.set(CV_CAP_PROP_FRAME_HEIGHT, cam_length);
	VideoCapture captwo(apiBackend + 1); 
	
	captwo.set(CV_CAP_PROP_FRAME_WIDTH, cam_width);
	captwo.set(CV_CAP_PROP_FRAME_HEIGHT, cam_length);
	VideoCapture capthree(apiBackend + 2);
	
	capthree.set(CV_CAP_PROP_FRAME_WIDTH, cam_width);
	capthree.set(CV_CAP_PROP_FRAME_HEIGHT, cam_length);
	VideoCapture capfour(apiBackend + 3);
	
	capfour.set(CV_CAP_PROP_FRAME_WIDTH, cam_width);
	capfour.set(CV_CAP_PROP_FRAME_HEIGHT, cam_length);
	VideoCapture capfive(apiBackend + 4);
	
	capfive.set(CV_CAP_PROP_FRAME_WIDTH, cam_width);
    capfive.set(CV_CAP_PROP_FRAME_HEIGHT, cam_length);
	VideoCapture capsix(apiBackend + 5);
	//capsix.open(CV_CAP_DSHOW);
	capsix.set(CV_CAP_PROP_FRAME_WIDTH, cam_width);
	capsix.set(CV_CAP_PROP_FRAME_HEIGHT, cam_length);
	
	Mat tempimage[6];
	Mat frametest[6];
	
	
	vector<uchar> memimage;          //to store the image into memory

	vector<int> encodingpara = vector<int> (2);
	encodingpara[0] = CV_IMWRITE_JPEG_QUALITY;
	encodingpara[1] = 95;
	static int final_copy = 0;
	
	double fpsone = 0.0;
	double fpstwo = 0.0;
	int framewidth = 0;
	int frameheight = 0;
	std::vector<int> gpus;
	gpus.reserve(num_gpus);
	char imagepath[20];
	int count = 0;
	int encodingtime;
///////////////


//////////
	

		for (int gpu = 0; gpu < num_gpus; ++gpu)
	{
		cudaDeviceProp prop;
		cudaGetDeviceProperties(&prop, gpu);

		if (prop.major > 5 || (prop.major == 5 && prop.minor >= 2))
		{
			gpus.push_back(gpu);

			// Multi-GPU not yet supported for mono, so just take the first GPU
			if (NVSTITCH_STITCHER_PIPELINE_MONO == params->pipeline)
				break;
		}
	}

	nvssVideoStitcherProperties_t stitcher_props{ 0 };
	stitcher_props.version = NVSTITCH_VERSION;
	stitcher_props.pano_width = params->pano_width;
	stitcher_props.quality = params->quality;
	stitcher_props.num_gpus = gpus.size();
	stitcher_props.ptr_gpus = gpus.data();
	stitcher_props.pipeline = params->pipeline;
	stitcher_props.projection = params->projection;
	stitcher_props.output_roi = params->output_roi;
	std::cout << "quality is" << stitcher_props.quality << endl;
	std::cout << "width is " << stitcher_props.pano_width << endl;
	

	switch (params->pipeline)
	{
	case NVSTITCH_STITCHER_PIPELINE_MONO:
		stitcher_props.feather_width = 2.0f;
		break;
	case NVSTITCH_STITCHER_PIPELINE_STEREO:
		stitcher_props.stereo_ipd = 6.3f;
		break;
	case NVSTITCH_STITCHER_PIPELINE_MONO_EQ:
		if (params->enable_depth_align)
		{
			stitcher_props.mono_flags |= NVSTITCH_MONO_FLAGS_ENABLE_DEPTH_ALIGNMENT;
		}
		if (params->enable_alpha_composite)
		{
			stitcher_props.mono_flags |= NVSTITCH_MONO_FLAGS_ENABLE_ALPHA_COMPOSITING;
		}
		break;
	}
	
	// Initialize stitcher instance
	nvssVideoHandle stitcher;
	
	//cout << "return nvss error is " << test << endl;
    RETURN_NVSS_ERROR(nvssVideoCreateInstance(&stitcher_props, &params->rig_properties, &stitcher));
	// << "return nvss error is " << test << endl;
  
	framewidth = capfive.get(CAP_PROP_FRAME_WIDTH);
	frameheight = capfive.get(CAP_PROP_FRAME_HEIGHT);
	
	
	cv::Mat::setDefaultAllocator(cv::cuda::HostMem::getAllocator(cv::cuda::HostMem::AllocType::PAGE_LOCKED));
	Mat outImgRgbtest;
	
	unsigned char *out_stacked;
	
	nvstitchImageBuffer_t output_image;
	nvssVideoGetOutputBuffer(stitcher, nvstitchEye(2), &output_image);
	cudaHostAlloc((void**)&out_stacked, output_image.row_bytes * output_image.height, cudaHostAllocDefault);
	while(1)
	{  
		
		GetSystemTime(&sendingtime);
		myfile << streamed_frames << "  frame" << sendingtime.wMinute << "  " << sendingtime.wSecond << "  " << sendingtime.wMilliseconds << endl;
		const auto final_stream_base = high_resolution_clock::now();
		
		
	    /*caputre the frame*/
		capone >> frametest[2];                 
		capsix >> frametest[4];                    
		captwo >> frametest[1];      
		capfive >> frametest[5];       
		capthree >> frametest[0]; 
		capfour >> frametest[3];   
	
		const auto frame_input_time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - final_stream_base).count();
		//cout << "the frame input time is " << frame_input_time <<"ms"<< endl;
		
		testfile << "1	" << frame_input_time << endl;
		
		/*****store the image to memory and copy them to the gpu***/
		

		for (int count = 0; count < camera_num; count++)
		{
		//	const auto each_convert_base = high_resolution_clock::now();
			nvstitchImageBuffer_t input_image;
			RETURN_NVSS_ERROR(nvssVideoGetInputBuffer(stitcher, count, &input_image));
			cvtColor(frametest[count], outImgRgbtest, CV_BGRA2RGBA);
		
			
			//const auto copy_start = high_resolution_clock::now();
			if (cudaMemcpy2D(input_image.dev_ptr, input_image.pitch,
				outImgRgbtest.data, input_image.row_bytes,
				input_image.row_bytes, input_image.height,
				cudaMemcpyHostToDevice) != cudaSuccess)   //5ms
			{
				std::cout << "Error copying RGBA image bitmap to CUDA buffer" << std::endl;
				return NVSTITCH_ERROR_GENERAL;
			}
		
		}
		auto copy_input_time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - final_stream_base).count();
		const auto cptime = copy_input_time - frame_input_time;
		std::cout << "the copy to gpu time is " << cptime << "ms" << endl;
		const auto stitch_start11 = high_resolution_clock::now();
		testfile << "2	" << cptime << endl;
		auto time1 = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - stitch_start11).count();
		//cout << "writing time is " << time1 << endl;
		
		// Move seams by seam_offset
		if (params->seam_offset != 0)  //0ms
		{
			uint32_t num_overlaps;
			if (NVSTITCH_SUCCESS != nvssVideoGetOverlapCount(stitcher, &num_overlaps))
			{
				std::cout << "Movable seams not supported by this pipeline; try MONO_EQ\n";
			}
			else
			{
				for (auto i = 0u; i < num_overlaps; ++i)
				{
					// Get the current seam and overlap info
					nvstitchSeam_t seam;
					nvstitchOverlap_t overlap;
					RETURN_NVSS_ERROR(nvssVideoGetOverlapInfo(stitcher, i, &overlap, &seam));

					if (seam.seam_type != NVSTITCH_SEAM_TYPE_VERTICAL)
					{
						std::cout << "Expected vertical seam - skipping\n";
					}
					else
					{
						// Shift seam by the offset and clamp to [0, width]
						const auto width = int(overlap.overlap_rect.width);
						const auto offset = int(seam.properties.vertical.x_offset) + params->seam_offset;
						seam.properties.vertical.x_offset = (uint32_t)((std::max)(0, (std::min)(width, offset)));
						RETURN_NVSS_ERROR(nvssVideoSetSeam(stitcher, i, &seam));
					}
				}
			}
		}
		

		// Synchronize CUDA before snapping start time
		cudaStreamSynchronize(cudaStreamDefault);     //2ms do we need it?

		const auto stitch_start = high_resolution_clock::now();

		// Stitch
		RETURN_NVSS_ERROR(nvssVideoStitch(stitcher));

		// Synchronize CUDA before snapping end time 
        cudaStreamSynchronize(cudaStreamDefault);

		// Report stitch time
		auto time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - stitch_start).count();
		//std::cout << "Stitch Time: " << time << " ms" << std::endl;
		testfile << "3	" << time << endl;
		size_t out_offset = 0;

		std::vector<nvstitchEye> eyes;
		
		eyes.push_back(NVSTITCH_EYE_MONO);
		
		for (auto eye : eyes)
		{
			RETURN_NVSS_ERROR(nvssVideoGetOutputBuffer(stitcher, nvstitchEye(eye), &output_image));
			cudaStream_t out_stream = nullptr;
			RETURN_NVSS_ERROR(nvssVideoGetOutputStream(stitcher, nvstitchEye(eye), &out_stream));
			const auto copyout1 = high_resolution_clock::now();
			
			if (cudaMemcpy2D(out_stacked + out_offset, output_image.row_bytes,
				output_image.dev_ptr, output_image.pitch,   
				output_image.row_bytes, output_image.height,
				cudaMemcpyDeviceToHost) != cudaSuccess)  ///copy_out_time 5ms
			{
				std::cout << "Error copying output stacked panorama from CUDA buffer" << std::endl;
				return NVSTITCH_ERROR_GENERAL;
			}
			
			auto copytime = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - copyout1).count();
			cout << "the copyout time is " << copytime << endl;
		/*	if (cudaStreamSynchronize(out_stream) != cudaSuccess)
			{
				std::cout << "Error synchronizing with the output CUDA stream" << std::endl;
				return NVSTITCH_ERROR_GENERAL;
			}*/

			out_offset += output_image.height * output_image.row_bytes;
		}
		
		const auto other_time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - final_stream_base).count();
		const auto other_process_time = other_time - frame_input_time - cptime;
		testfile << "4	" << other_process_time - time << endl;

		//const auto display_start = high_resolution_clock::now();

		/******start stream****************/
		//const auto stream_start = high_resolution_clock::now();
		  Mat img = cv::Mat(output_image.height, output_image.width, CV_8UC4, out_stacked);
		  //imwrite("e://image//camera1.jpg", img);
		  
		 
	//	namedWindow("videocapture", 0);//////////
	//	cv::imshow("videocapture", img);//used to show the stitched video before streaming
		
	
		if (!from_camera) {
			
			GetSystemTime(&sendingtime);
			myfile <<sendingtime.wMinute<<"  " << sendingtime.wSecond <<"  "<< sendingtime.wMilliseconds << endl;
			const auto stitch_start1 = high_resolution_clock::now();
			encodingtime = stream.stream_frame(img);
			cout << "success" << endl;
			auto timetest = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - stitch_start1).count();
			testfile << "5	" << encodingtime << endl;
			//auto timetest = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - stitch_start1).count();
			testfile<< "6	" << timetest-encodingtime << std::endl;
		}
		else {
			stream.stream_frame(img, frame_time.count()*stream.inv_stream_timebase);
			//cout << "testfrom_camera" << endl;
		}

	  if (!from_camera) {
		 streamed_frames++;
		 // cout << "had already sent" << streamed_frames << endl;
			moving_average.add_value(frame_time.count());
			avg_frame_time = moving_average.get_average();
			//cout << "the average frame time is" << avg_frame_time << endl;
			add_delay(streamed_frames, stream_fps, elapsed_time.count(), avg_frame_time);
			
		}
	 // auto stream_time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - final_stream_base).count();
	//  const auto frame_stream_time = stream_time - cptime - other_process_time - frame_input_time;
	  //testfile << "6	" << frame_stream_time-encodingtime <<std::endl;
	//  myfile<< std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - final_stream_base).count() <<endl;


	//	const auto test_stream_time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - final_stream_base).count();
	//	std::cout << "each loop Time: " << test_stream_time << " ms" << std::endl;
	 // cudaFreeHost(out_stacked);
	  
		 char a = waitKey(2);
		if (a == 27)
		{	
			myfile.close();
			break;
		}
		//break;
	}
	


	
	
}
