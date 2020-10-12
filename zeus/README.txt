nvss_video Sample Application
-----------------------------

Overview
--------

Stitches a collection of images specified in the footage XML file into a single mono
panorama BMP image. To create a top-bottom stacked stereo panorama BMP specify --stereo
on the command line. Use --rig_spec to specify the rig parameters. This sample uses
OpenCV to perform the image read and write operations. Full image filenames including
extensions are required to determine the image codec. The overall stitching time is
displayed in the command window in milliseconds.

Usage
-----

nvss_sample --help
            --input_dir_base <BMP path with trailing '/'>
            --rig_spec <rig spec XML file>
            --image_input <footage XML file>
            --pano_width <pano width>                                (Default is 3840)
            --quality <stitch quality (0=high, 1=medium, 2=low)>     (Default is high)
            --out_file <stacked output panorama filename>            (Default is my_stacked_360.BMP)
            --stereo                                                 (Default is to perform a mono stitch)
            --mono                                                   monocular stitch (default)
            --mono_eq                                                newer monocular stitch for equatorial rigs only
            --depth_align                                            align using depth (only for --mono_eq)
            --alpha_composite                                        alpha blend rather than cut between images before multiband blending
            --seam_offset                                            shift the blending seams left or right (only for --mono_eq)
            --roi <left>,<top>,<width>,<height>                      output region of interest defined by rectangle (only for --mono_eq)

-------
Example
-------

nvss_sample --input_dir_base ../../footage/ --rig_spec sample_calib_rig_spec.xml --image_input image_input.xml --out_file out_stitched.jpg --stereo


-----------------------------------------------
run_stereo.bat (Windows), run_stereo.sh (Linux)
-----------------------------------------------

Creates a top-bottom stacked stereo panorama image using the sample images in ../../footage directory with the low level video SDK.
The result will be available to view in ../../samples/nvss_sample/out_stitched.jpg


-------------------------------------------
run_mono.bat (Windows), run_mono.sh (Linux)
-------------------------------------------

Creates a mono panorama image using the sample images in ../../footage directory with the low level video SDK.
The result will be available to view in ../../samples/nvss_sample/out_stitched.jpg


-------------------------------------------------
run_mono_eq.bat (Windows), run_mono_eq.sh (Linux)
-------------------------------------------------

Creates a mono panorama image using the equatorially-distributed sample images in ../../footage directory with the low level video SDK.
This has quality and speed improvements over run_mono, but is not recommended for rigs that have cameras significantly tilted off the equator.
The result will be available to view in ../../samples/nvss_sample/out_stitched.jpg


-------------------------------------------------------------------
run_mono_depth_align.bat (Windows), run_mono_depth_align.sh (Linux)
-------------------------------------------------------------------

Creates a mono panorama image using the equatorially-distributed sample images in ../../footage directory with the low level video SDK,
incorporating depth estimates in order to improve alignment.
This has quality improvements over run_mono_eq, but is not recommended for rigs that have cameras significantly tilted off the equator.
The result will be available to view in ../../samples/nvss_sample/out_stitched.jpg
