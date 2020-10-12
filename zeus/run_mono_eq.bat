@ECHO OFF
SETLOCAL
SET PATH=%PATH%;..\..\nvstitch\binary;..\..\external\cuda;..\..\external\opencv-3.4.5\binary
nvss_sample.exe --input_dir_base ..\..\footage\ --rig_spec sample_calib_rig_spec.xml --image_input image_input.xml --out_file out_stitched.jpg --mono_eq --seam_offset -42