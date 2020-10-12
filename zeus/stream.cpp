#include "stream.h"

#include <string>
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "windows.h"
#include <iostream>
#include "app.h"
//#include <unistd.h>
using namespace std::chrono;
using namespace std;

namespace streamer
{

#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P

	static int encode_and_write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame)
	{
		AVPacket pkt = { 0 };
		av_init_packet(&pkt);

		int ret = avcodec_send_frame(codec_ctx, frame);
		if (ret < 0)
		{
			fprintf(stderr, "Error sending frame to codec context!\n");
			return ret;
		}

		ret = avcodec_receive_packet(codec_ctx, &pkt);
		if (ret < 0)
		{
			fprintf(stderr, "Error receiving packet from codec context!\n");
			return ret;
		}

		av_interleaved_write_frame(fmt_ctx, &pkt);
		av_packet_unref(&pkt);

		return 0;
	}


	static int set_options_and_open_encoder(AVFormatContext *fctx, AVStream *stream, AVCodecContext *codec_ctx, AVCodec *codec,
		std::string codec_profile, double width, double height,
		int fps, int bitrate, AVCodecID codec_id)
	{
		const AVRational dst_fps = { fps, 1 };

		codec_ctx->codec_tag = 0;
		codec_ctx->codec_id = codec_id;
		codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
		codec_ctx->width = width;
		codec_ctx->height = height;
		codec_ctx->gop_size = 30;                //////////////////////////
		codec_ctx->pix_fmt = STREAM_PIX_FMT;
		codec_ctx->framerate = dst_fps;
		codec_ctx->time_base = av_inv_q(dst_fps);
		//codec_ctx->bit_rate = bitrate;
		//codec_ctx->bit_rate_tolerance = bitrate;
		//codec_ctx->rc_max_rate = bitrate;
		//codec_ctx->rc_min_rate = bitrate;
		//codec_ctx->rc_initial_buffer_occupancy = codec_ctx->rc_buffer_size * 3 / 4;
		//codec_ctx->rc_buffer_size = bitrate;
	   
		if (fctx->oformat->flags & AVFMT_GLOBALHEADER)
		{
			codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

		stream->time_base = codec_ctx->time_base; //will be set afterwards by avformat_write_header to 1/1000

		int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx); //
		if (ret < 0)
		{
			fprintf(stderr, "Could not initialize stream codec parameters!\n");
			return 1;
		}
		
		AVDictionary *codec_options = nullptr;
		av_dict_set(&codec_options, "profile", codec_profile.c_str(), 0);
		av_dict_set(&codec_options, "preset", "fast", 0);
		av_dict_set(&codec_options, "tune", "zerolatency", 0);
	//	av_opt_set(codec_ctx->priv_data,"crf","40000",AV_OPT_SEARCH_CHILDREN);
		av_opt_set(codec_ctx, "b", "2000000", 0); ////////////////////////////////////////////////////////
		// open video encoder
		ret = avcodec_open2(codec_ctx, codec, &codec_options);
		if (ret < 0)
		{
			fprintf(stderr, "Could not open video encoder!\n");
			return 1;
		}
		av_dict_free(&codec_options);////////
		return 0;
	}


	Streamer::Streamer()
	{
		format_ctx = nullptr;
		out_codec = nullptr;
		out_stream = nullptr;
		out_codec_ctx = nullptr;
		rtmp_server_conn = false;
		av_register_all();
		inv_stream_timebase = 30.0;
		network_init_ok = !avformat_network_init();
	}


	void Streamer::cleanup()
	{
		if (out_codec_ctx) {
			avcodec_close(out_codec_ctx);
			avcodec_free_context(&out_codec_ctx);
		}

		if (format_ctx) {
			if (format_ctx->pb) {
				avio_close(format_ctx->pb);
			}
			avformat_free_context(format_ctx);
			format_ctx = nullptr;
		}
	}


	Streamer::~Streamer()
	{
		cleanup();
		avformat_network_deinit();
	}


	int Streamer::stream_frame(const cv::Mat &image)
	{ 
		
		//test.open("E:\\Junproject\\resultt.txt");

		if (can_stream()) {///convert cv:mat to avframe
			const int stride[] = { static_cast<int>(image.step[0]) };
			sws_scale(scaler.ctx, &image.data, stride, 0, image.rows, picture.frame->data, picture.frame->linesize);
			picture.frame->pts += av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base);
			const auto stitch_start = high_resolution_clock::now();
			encode_and_write_frame(out_codec_ctx, format_ctx, picture.frame);
			auto time = std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - stitch_start).count();
			cout << "encoding time is" << time << "ms" << endl;
		//	test << "5	" << time << endl;
			return time;
		}
	}


	int Streamer::stream_frame(const cv::Mat &image, int64_t frame_duration)
	{
		if (can_stream()) {
			const int stride[] = { static_cast<int>(image.step[0]) };
			sws_scale(scaler.ctx, &image.data, stride, 0, image.rows, picture.frame->data, picture.frame->linesize);
			picture.frame->pts += frame_duration; //time of frame in milliseconds
			encode_and_write_frame(out_codec_ctx, format_ctx, picture.frame);
		}
		return 0;
	}


	void Streamer::enable_av_debug_log()
	{
		av_log_set_level(AV_LOG_DEBUG);
	}


	int Streamer::init(const StreamerConfig &streamer_config)
	{
		init_ok = false;
		cleanup();

		avcodec_register_all();
		av_register_all();
		avformat_network_init();

		config = streamer_config;

		if (!network_init_ok) {
			return 1;
		}

		//initialize format context for output with flv and no filename
		avformat_alloc_output_context2(&format_ctx, nullptr, "flv", nullptr);
		if (!format_ctx) {
			return 1;
		}

		//AVIOContext for accessing the resource indicated by url   open output server context 
		if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
			int avopen_ret = avio_open2(&format_ctx->pb, config.server.c_str(),
				AVIO_FLAG_WRITE, nullptr, nullptr);
			if (avopen_ret < 0) {
				fprintf(stderr, "failed to open stream output context, stream will not work\n");
				return 1;
			}
			rtmp_server_conn = true;
		}

		//use selected codec  create new video stream
		AVCodecID codec_id = AV_CODEC_ID_H264;///AV_CODEC_ID_HEVC
		out_codec = avcodec_find_encoder(codec_id);
		if (!(out_codec)) {
			fprintf(stderr, "Could not find encoder for '%s'\n",
				avcodec_get_name(codec_id));
			return 1;
		}

		out_stream = avformat_new_stream(format_ctx, out_codec);//add h264 stream
		if (!out_stream) {
			fprintf(stderr, "Could not allocate stream\n");
			return 1;
		}

		out_codec_ctx = avcodec_alloc_context3(out_codec);//

		if (set_options_and_open_encoder(format_ctx, out_stream, out_codec_ctx, out_codec, config.profile,
			config.dst_width, config.dst_height, config.fps, config.bitrate, codec_id)) {
			return 1;
		}///

		out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;///////////////////
		out_stream->codecpar->extradata = static_cast<uint8_t*>(av_mallocz(out_codec_ctx->extradata_size));///
		memcpy(out_stream->codecpar->extradata, out_codec_ctx->extradata, out_codec_ctx->extradata_size);//////////////////

		av_dump_format(format_ctx, 0, config.server.c_str(), 1);//

		picture.init(out_codec_ctx->pix_fmt, config.dst_width, config.dst_height);
		//printf("decode formate is%d", out_codec_ctx->pix_fmt);
		scaler.init(out_codec_ctx, config.src_width, config.src_height, config.dst_width, config.dst_height, SWS_BILINEAR);//SWS_BICUBI



		if (avformat_write_header(format_ctx, nullptr) < 0)
		{
			fprintf(stderr, "Could not write header!\n");
			return 1;
		}

		printf("stream time base = %d / %d \n", out_stream->time_base.num, out_stream->time_base.den);

		inv_stream_timebase = (double)out_stream->time_base.den / (double)out_stream->time_base.num;

		init_ok = true;
		return 0;
	}

} // namespace streamer


