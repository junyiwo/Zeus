# Zeus
This README file contains the instructions to build our Zeus prototype:

Jun Yi, Md Reazul Islam, Shivang Aggarwal, Dimitrios Koutsonikolas, Y. Charlie Hu and Zhisheng Yan. An Analysis of Delay in Live 360° Video Streaming Systems. In Proceedings of the 28th ACM International Conferenceon Multimedia (MM ’20), 2020. 

The source code is distributed under the terms of the MIT license. Our contributions in this project are:

- We identify the diverse relationship between the time con-sumption breakdown across the system pipeline and thethree delay metrics in live 360° video streaming. 
- We build an open research prototypeZeususing publiclyavailable hardware and software to enable task-level delaymeasurement. The methodology for buildingZeuscan beutilized in future 360° video research.
- We leverageZeusto perform a comprehensive measurementstudy to dissect the time consumption in live 360°videostreaming and understand how each task affects differentdelay metrics.
- We perform a comparison ofZeusagainst a commerciallive 360°video streaming system built on Ricoh Theta Vand YouTube and validate that our measurement results arerepresentative of real world systems 

To cite our paper, use this Bibtex code:
```
> @inproceedings{junyi and zhisheng yan,
  title={An Analysis of Delay in Live 360° Video Streaming Systems}, 
  author={Yi, Jun and Islam, Md Reazul and Aggarwal, Shivang and Koutsonikolas, Dimitrios and Hu, Y Charlie and Yan, Zhisheng},
  booktitle={Proceedings of the 28th ACM International Conferenceon Multimedia (MM '20)},
  year={2020},
  organization={ACM}
}
```

# Paper abstract
While live 360° video streaming provides an enriched viewing experience, it is challenging to guarantee the user experience against the negative effects introduced by start-up delay, event-to-eye delay, and low frame rate. It is therefore imperative to understand how different computing tasks of a live 360° streaming system contribute
to these three delay metrics. Although prior works have studied commercial live 360° video streaming systems, none of them has dug into the end-to-end pipeline and explored how the task-level time consumption affects the user experience. In this paper, we conduct the first in-depth measurement study of task-level time consumption for five system components in live 360° video streaming. We first identify the subtle relationship between the time consumption breakdown across the system pipeline and the three delay metrics. We then build a prototype Zeus to measure this relationship. Our findings indicate the importance of CPU-GPU transfer at the camera and the server initialization as well as the negligible effect of 360° video stitching on the delay metrics. We finally validate that our results are representative of real world systems by comparing them with those obtained with a commercial system.

# Hardware configureation
Camera: GoPro Hero 6 using HDMI ouput
Data converter: USB Capture 4K Plus
USB HubUSB Type-C Multi-Adapter
Charger: USB Wall Charger
HDMI cable: UGREEN Micro HDMI Cable

# Requirement & installation 

# License
This project is licensed under the terms of the MIT license.

# Contact
If you have any general doubt about our work, please use the public issues section on this github. Alternatively, drop us an e-mail at <mailto:jyi39@student.gsu.edu> or <mailto:zyan@gsu.edu>
