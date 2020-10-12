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

#include <stdint.h>
#include <iostream>
#include <string>

#include "CmdArgsMap.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include<opencv2\opencv.hpp>

#include "app.h"
#include "stream.h"
#include "xml_util/xml_utility_video.h"
#define ouput_width  (1024)



extern "C"
{
#include "avcodec.h"
#include "avformat.h"
#include "swscale.h"
#include "avdevice.h"
}



using namespace cv;
using namespace std;






nvstitchRect_t
ParseROI(const std::string& str)
{
    std::vector<uint32_t> split{};
    std::istringstream in_stream(str);
    std::string token;
    while (std::getline(in_stream, token, ','))
    {
        split.push_back(strtoul(token.c_str(), nullptr, 0));
    }

    if (split.size() == 4)
    {
        return nvstitchRect_t{ split[0], split[1], split[2], split[3] };
    }
    else
    {
        return nvstitchRect_t{};
    }
}



int
main(int argc, char *argv[])
{
    app myApp;
    appParams myAppParams{};

    bool show_help = false;
    std::string rig_spec_name;
    std::string image_input_name;
	myAppParams.pano_width = ouput_width;
    std::string projectionType = "equirect";
    myAppParams.quality = NVSTITCH_STITCHER_QUALITY_LOW;//NVSTITCH_STITCHER_QUALITY_MEDIUM //////////////////////////////
    myAppParams.out_file = "stacked_360.bmp";
    myAppParams.enable_depth_align = false;
    myAppParams.enable_alpha_composite = false;
    bool mono_flag = false;// set the default stitching as mono
    bool stereo_flag = false;
    bool mono_eq_flag = false;
    std::string roi_string = "0,0,0,0";	//research of interest



    int pano_width_arg = myAppParams.pano_width;
    int quality_arg = myAppParams.quality;
    // Process command line arguments
    CmdArgsMap cmdArgs = CmdArgsMap(argc, argv, "--")
        ("help", "Produce help message", &show_help)
        ("input_dir_base", "Base directory for input MP4 files", &myAppParams.input_base_dir, myAppParams.input_base_dir)
        ("rig_spec", "XML file containing rig specification", &rig_spec_name, rig_spec_name) //important
        ("image_input", "XML file containing footage files", &image_input_name, image_input_name) // important
        ("pano_width", "Width of the output panorama", &pano_width_arg, pano_width_arg) 
        ("quality", "Stitch quality (0=high, 1=medium, 2=low)", &quality_arg, quality_arg)
        ("out_file", "Stacked output panorama", &myAppParams.out_file, myAppParams.out_file)
        ("mono", "Mono pipeline (default)", &mono_flag)
        ("stereo", "Stereo pipeline", &stereo_flag)
        ("mono_eq", "Mono_eq pipeline", &mono_eq_flag)
        ("depth_align", "Mono_eq + depth alignment", &myAppParams.enable_depth_align)
        ("alpha_composite", "Mono_eq + alpha composite", &myAppParams.enable_alpha_composite)
        ("seam_offset", "Mono_eq: move seams horizontally by offset", &myAppParams.seam_offset, 0)
        ("roi", "Mono_eq: output ROI (left, top, width, height)", &roi_string, roi_string);

    // Only allow one of {mono, stereo, mono_eq}
    const int flag_count = mono_flag + stereo_flag + mono_eq_flag;
    if (flag_count > 1)
    {
        std::cout << "Only one of {mono, stereo, mono_eq} may be specified\n";
        exit(0);
    }
    else if (mono_flag)
    {
        myAppParams.pipeline = NVSTITCH_STITCHER_PIPELINE_MONO;  //this is the default pattern
    }
    else if (stereo_flag)
    {
        myAppParams.pipeline = NVSTITCH_STITCHER_PIPELINE_STEREO;
    }
    else if (mono_eq_flag)
    {
        myAppParams.pipeline = NVSTITCH_STITCHER_PIPELINE_MONO_EQ;
    }
    else if (myAppParams.enable_depth_align || myAppParams.enable_alpha_composite)
    {
        // if mono-specific flags are used on their own, default to mono_eq pipeline
        myAppParams.pipeline = NVSTITCH_STITCHER_PIPELINE_MONO_EQ;
    }
    else
    {
        // otherwise default to mono pipeline
        myAppParams.pipeline = NVSTITCH_STITCHER_PIPELINE_MONO;
    }

    // Projection Type
    if (projectionType == "equirect")
    {
        myAppParams.projection = nvstitchPanoramaProjectionType::NVSTITCH_PANORAMA_PROJECTION_EQUIRECTANGULAR;
    }
    else
    {
        std::cout << "Unknown projection type specified '" << projectionType <<
            "' Valid types are: equirect" << std::endl;
        return 1;
    }

    if (show_help || rig_spec_name.empty())
    {
        std::cout << "Low-Level Video Stitch Sample Application" << std::endl;
        std::cout << cmdArgs.help();
        return 1;
    }

    if (image_input_name.empty())
    {
        image_input_name = rig_spec_name;
    }

    if (pano_width_arg > 0)
    {
        myAppParams.pano_width = pano_width_arg;
    }
    else
    {
        std::cout << "Invalid panorama width - must be greater than zero.\n";
        exit(0);
    }

    if (quality_arg < 0 || quality_arg > 2)
    {
        std::cout << "Invalid quality_preset: 0=high(default), 1=medium, 2=low\n";
        std::cout << cmdArgs.help();
        exit(0);
    }
    myAppParams.quality = (nvstitchStitcherQuality)quality_arg;

    myAppParams.output_roi = ParseROI(roi_string);

    if (!myAppParams.input_base_dir.empty())
    {
        switch (myAppParams.input_base_dir[myAppParams.input_base_dir.size() - 1])
        {
#ifdef _MSC_VER
        case '\\':
#endif // _MSC_VER
        case '/':
            break;
        default:
            myAppParams.input_base_dir += '/';
            break;
        }
    }

    // Fetch rig parameters from XML file. input parameters into rig_properties 
    if (!xmlutil::readCameraRigXml(myAppParams.input_base_dir + rig_spec_name, myAppParams.cam_properties, &myAppParams.rig_properties))
    {
        std::cout << std::endl << "Failed to retrieve rig paramters from XML file." << std::endl;
        return 1;
    }

    // Fetch input media feeds from XML file.
    if (!xmlutil::readInputMediaFeedFilenamesXml(myAppParams.input_base_dir + image_input_name, myAppParams.filenames))
    {
        std::cout << std::endl << "Failed to retrieve input media feeds from XML file." << std::endl;
        return 1;
    }

	
    if (myApp.RunZeus(&myAppParams) != NVSTITCH_SUCCESS)
    {
        std::cout << "Stitcher failure." << std::endl;
        return 1;
    }

    return 0;
}