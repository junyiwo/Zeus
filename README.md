# Zeus
This README file contains the instructions to build our prototype Zeus. The source code is distributed under the terms of the MIT license.

Jun Yi, Md Reazul Islam, Shivang Aggarwal, Dimitrios Koutsonikolas, Y. Charlie Hu and Zhisheng Yan. An Analysis of Delay in Live 360° Video Streaming Systems. In Proceedings of the 28th ACM International Conferenceon Multimedia (MM ’20), 2020. 

# What is Zeus

Zeus is a prototype used to measure the delay of live 360 video streaming systems. Zeus will first capture regular video stream using six GoPro cameras, and stitch them into a 360 video using a laptop. After that, the 360 video will be transferred to the video server Nginx. The clients can send request to the video server to watch the live video. The system pipeline of Zeus can be divided into different tasks, and it allows developers to measure the time consumption of each task.  Below is the figure shows our prototype,

![Prototype](https://github.com/junyiwo/Zeus/blob/master/Image/prototype.png)



# Paper abstract and Contributions
While live 360° video streaming provides an enriched viewing experience, it is challenging to guarantee the user experience against the negative effects introduced by start-up delay, event-to-eye delay, and low frame rate. It is therefore imperative to understand how different computing tasks of a live 360° streaming system contribute
to these three delay metrics. Although prior works have studied commercial live 360° video streaming systems, none of them has dug into the end-to-end pipeline and explored how the task-level time consumption affects the user experience. In this paper, we conduct the first in-depth measurement study of task-level time consumption for five system components in live 360° video streaming. We first identify the subtle relationship between the time consumption breakdown across the system pipeline and the three delay metrics. We then build a prototype Zeus to measure this relationship. Our findings indicate the importance of CPU-GPU transfer at the camera and the server initialization as well as the negligible effect of 360° video stitching on the delay metrics. We finally validate that our results are representative of real world systems by comparing them with those obtained with a commercial system.

 Our contributions in this project are:

- We identify the diverse relationship between the time consumption breakdown across the system pipeline and the three delay metrics in live 360° video streaming. 
- We build an open research prototype Zeus using publicly available hardware and software to enable task-level delay measurement. The methodology for building Zeus can be utilized in future 360° video research.
- We leverage Zeus to perform a comprehensive measurement study to dissect the time consumption in live 360° video streaming and understand how each task affects different delay metrics.
- We perform a comparison of Zeus against a commercial live 360° video streaming system built on Ricoh Theta Vand YouTube and validate that our measurement results are representative of real world systems 

# Hardware configuration
Camera: GoPro Hero 6 using HDMI output. 

Data converter: USB Capture 4K Plus. The data converter is used to convert the HDMI output of the camera into USB, otherwise, the cameras cannot connect with the laptop used for stitching.  

USB Hub: USB Type-C Multi-Adapter. Due to the limited USB data interfaces of the laptop, we also need to use the type-c interfaces to receive the USB data. 

Charger: USB Wall Charger. It is used to charge the camera while working

HDMI cable: UGREEN Micro HDMI Cable

# Requirement & Instructions 

Before the camera transfer the video stream to the server, make sure that the server is running and has been configured correctly. 

For the **camera component**,

1.  Download [VRWorks 360 Video SDK](https://developer.nvidia.com/nvidia-developer-zone).

2. Download Microsoft Visual Studio.

The primary code is included in the folder "zeus". They are mainly for video capture, stitching, and encoding. After building the project,

>> First, open the command prompt

>> Second, redirect the path to the executed file using command "cd X:\..\Release" (X is the disk name, "X:\..\Release" is the path of the executed file)

>> Third, execute the command "project_name.exe --input_dir_base ..\..\ --rig_spec rig_spec_try.xml --image_input image_input.xml --out_file out_stitched.jpg" to upload the 360 video to the server.

For the **server**,

>> First, download Nginx-1.16.1 and [Nginx-http-flv-module](https://github.com/winshining/nginx-http-flv-module), and add the module to Nginx when installing the server. 

>> Second, configure the "nginx.conf" file to enable the live streaming. You can take the "nginx.conf" in the "Server" folder as a reference. Please modify the IP address in the file. 
>> Third, restart the Nginx server using command "./installation path of Nginx/nginx -s reload"



For the **client component**,

All the files are included in the folder "Player", the "player.js" is a webpage player. It will play the requested 360 video after double clicked it. 

# Citation
To cite our paper, please use this Bibtex code:
```
@inproceedings{yi2020analysis,
  title={An Analysis of Delay in Live 360 Video Streaming Systems},
  author={Yi, Jun and Islam, Md Reazul and Aggarwal, Shivang and Koutsonikolas, Dimitrios and Hu, Y Charlie and Yan, Zhisheng},
  booktitle={Proceedings of the 28th ACM International Conference on Multimedia},
  pages={982--990},
  year={2020}
}

```


# License
This project is licensed under the terms of the MIT license.

# Contact
If you have any general doubt about our work, please use the public issues section on this github. Alternatively, drop us an e-mail at <mailto:jyi39@student.gsu.edu> or <mailto:zyan@gsu.edu>
