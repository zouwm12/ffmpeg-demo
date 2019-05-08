#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
 
#define MAX_AUDIO_FRAME_SIZE  192000
 
const char *in_file = "./test.aac";
const char *out_file = "./test.pcm";

int main()
{
	int i = 0;
	int ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext  *dec_ctx = NULL;
    AVCodec         *dec   = NULL;
 
    //分配一个avformat
    fmt_ctx = avformat_alloc_context();
    if (fmt_ctx == NULL)
    {
        printf("alloc fail");
		exit(1);
    }
 
    //打开文件，解封装
    if (avformat_open_input(&fmt_ctx, in_file, NULL, NULL) != 0)
    {
        printf("open fail");
		exit(1);
    }
 
    //查找文件的相关流信息
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        printf("find stream fail");
		exit(1);
    }
 
    //输出格式信息
    av_dump_format(fmt_ctx, 0, in_file, 0);
 
    //查找解码信息
    int stream_index = -1;
    for (i = 0; i < fmt_ctx->nb_streams; i++)
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
 
    if (stream_index == -1)
        printf("find stream fail");
 
    //保存解码器
    dec = avcodec_find_decoder(fmt_ctx->streams[stream_index]->codecpar->codec_id);
    if (dec == NULL)
        printf("find codec fail");

	/* create decoding context */
	dec_ctx = avcodec_alloc_context3(dec);
	if (!dec_ctx)
		printf("avcodec_alloc_context3 failed\n");
	avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[stream_index]->codecpar);
 
    if (avcodec_open2(dec_ctx, dec, NULL) < 0)
        printf("can't open codec");
 
    FILE *out_fb = NULL;
    out_fb = fopen(out_file, "wb");
 
    //创建packet,用于存储解码前的数据
    AVPacket *packet = av_packet_alloc();;
    av_init_packet(packet);
 
    //设置转码后输出相关参数
    
    //采样个数
    int out_nb_samples = 2048;
    //采样格式
    enum AVSampleFormat  sample_fmt = AV_SAMPLE_FMT_S16;
    //采样率
    int out_sample_rate = 44100;
    //通道数
    uint64_t out_channel_layout = AV_CH_LAYOUT_MONO;	//采样的布局方式     ，单声道
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

	int buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, sample_fmt, 1);
 
 
    //注意要用av_malloc
    uint8_t *buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
 
 
    //创建Frame，用于存储解码后的数据
    AVFrame *frame = av_frame_alloc();
 
    int64_t in_channel_layout = av_get_default_channel_layout(dec_ctx->channels);
    //打开转码器
    struct SwrContext *convert_ctx = swr_alloc();
    //设置转码参数
	convert_ctx = swr_alloc_set_opts(convert_ctx, out_channel_layout, sample_fmt, out_sample_rate, \
            in_channel_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate, 0, NULL);
    //初始化转码器
    swr_init(convert_ctx);
 
    //while循环，每次读取一帧，并转码
 
    while (av_read_frame(fmt_ctx, packet) >= 0) {
 
        if (packet->stream_index == stream_index) {


			ret = avcodec_send_packet(dec_ctx, packet);
		    if (ret < 0) {
		        fprintf(stderr, "Error sending a packet for decoding\n");
		        break;
		    }

		    while (ret >= 0) {
		        ret = avcodec_receive_frame(dec_ctx, frame);
		        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		            break;
		        else if (ret < 0) {
		            fprintf(stderr, "Error during decoding\n");
		            break;
		        }
		        fflush(stdout);	

				swr_convert(convert_ctx, &buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
//                printf("pts:%10lld\t packet size:%d\n", packet->pts, packet->size);
//				printf("frame->nb_samples:%d-----------------\n",frame->nb_samples);
	
                fwrite(buffer, 1, buffer_size, out_fb);
		    }

		

        }
 
    }
 
    swr_free(&convert_ctx);
 	av_frame_free(&frame);
	av_packet_free(&packet);
    fclose(out_fb);
 
    return 0;
}

