#ifndef STREAM_HPP
#define STREAM_HPP

extern "C" {
#include "../libavutil/opt.h"
#include "../libavcodec/avcodec.h"
#include "../libavutil/channel_layout.h"
#include "../libavutil/common.h"
#include "../libavutil/imgutils.h"
#include "../libavutil/mathematics.h"
#include "../libavutil/samplefmt.h"

#include "../libavformat/avformat.h"
#include "../libavcodec/avcodec.h"
#include "../libavutil/imgutils.h"
#include "../libswscale/swscale.h"
}

#include <string>
#include <opencv2/opencv.hpp>
#include <stdlib.h>

#define posix_memalign(p, a, s) (((*(p)) = _aligned_malloc((s), (a))), *(p) ?0 :errno)


namespace streamer
{


	class Scaler
	{
	public:
		SwsContext *ctx;

		Scaler()
		{
			ctx = nullptr;
		}

		~Scaler()
		{
			if (ctx) {
				sws_freeContext(ctx);
			}
		}

		int init(AVCodecContext *codec_ctx, int src_width, int src_height, int dst_width, int dst_height, int flags)
		{
			ctx = sws_getContext(src_width, src_height, AV_PIX_FMT_RGBA, dst_width, dst_height, ///////importantAV_PIX_FMT_RGBA
				codec_ctx->pix_fmt, flags, nullptr, nullptr, nullptr);
			if (!ctx) {
				fprintf(stderr, "Could not initialize sample scaler!\n");
				return 1;
			}
			return 0;
		}
	};



	class Picture
	{
		static const int align_frame_buffer = 32;
	public:

		AVFrame *frame;
		uint8_t *data;

		int init(enum AVPixelFormat pix_fmt, int width, int height)
		{
			frame = nullptr;
			data = nullptr;
			frame = av_frame_alloc();

			int sz = av_image_get_buffer_size(pix_fmt, width, height, align_frame_buffer);
			int ret = posix_memalign(reinterpret_cast<void**>(&data), align_frame_buffer, sz);

			//void* data = _aligned_malloc(sz, align_frame_buffer);
			if (!data) {
				// OMG: it failed! error is stored in errno.
			}

			av_image_fill_arrays(frame->data, frame->linesize, data, pix_fmt, width, height, align_frame_buffer);
			frame->format = pix_fmt;
			frame->width = width;
			frame->height = height;

			return 1;
		}

		Picture()
		{
			frame = nullptr;
			data = nullptr;
		}


		~Picture()
		{
			if (data) {
				free(data);
				data = nullptr;
			}

			if (frame) {
				av_frame_free(&frame);
			}
		}
	};


	struct StreamerConfig
	{
		int src_width;
		int src_height;
		int dst_width;
		int dst_height;
		int fps;
		int bitrate;
		std::string profile;
		std::string server;

		StreamerConfig()
		{
			dst_width = 0;
			dst_height = 0;
			src_width = 0;
			src_height = 0;
			fps = 0;
			bitrate = 0;
		}

		StreamerConfig(int source_width, int source_height, int stream_width, int stream_height, int stream_fps, int stream_bitrate,
			const std::string &stream_profile,
			const std::string &stream_server)
		{
			src_width = source_width;
			src_height = source_height;
			dst_width = stream_width;
			dst_height = stream_height;
			fps = stream_fps;
			bitrate = stream_bitrate;
			profile = stream_profile;
			server = stream_server;
		}
	};


	class Streamer
	{
		bool network_init_ok;
		bool rtmp_server_conn;
		bool init_ok;

		AVFormatContext *format_ctx;  //rtmp flv fengzhuanqi
		AVCodec *out_codec;           //bianmaqi
		AVStream *out_stream;         
		AVCodecContext *out_codec_ctx; //bian ma qi peizhi

		Scaler scaler;   //chushihua bianmaqi peizhi
		Picture picture;

		void cleanup();
		bool can_stream()
		{
			return network_init_ok && rtmp_server_conn && init_ok;
		}

	public:
		double inv_stream_timebase;
		StreamerConfig config;
		Streamer();
		~Streamer();
		void enable_av_debug_log();
		int init(const StreamerConfig &streamer_config);
		int stream_frame(const cv::Mat &frame);
		int stream_frame(const cv::Mat &image, int64_t frame_duration);
	};

} // namespace streamer
#endif

