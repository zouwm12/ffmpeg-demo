#include <stdio.h>
#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


void saveFrame(AVFrame* pFrame, int width, int height, int iFrame,const char *outname)
{
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, outname, iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for (y = 0; y < height; y++)
        fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, pFile);

    // Close file
    fclose(pFile);
}


static void video_decode_example(const char *outfilename, const char *filename)
{
	int i = 0;
	int ret = 0;
    AVFormatContext* pFormatCtx = NULL;
	AVFrame* pFrame = NULL;
    AVFrame* pFrameTranslate = NULL;
	int numBytes = 0;
    uint8_t* buffer = NULL;
	enum AVPixelFormat translate_format = AV_PIX_FMT_RGB24;


	/**
	 * 解封装
	 */
	 
    //step 1:open file,get format info from file header
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0){
        fprintf(stderr,"avformat_open_input");
        return;
    }
    //step 2:get stread info
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        fprintf(stderr,"avformat_find_stream_info");
        return; 
    }
    //打印相关信息
    av_dump_format(pFormatCtx, 0, filename, 0);
    int videoStream = -1;
    //step 3:查找video在AVFormatContext的哪个stream中
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1){
        fprintf(stderr,"find video stream error");
        return;
    }
	
    AVCodecContext* pCodecCtx = NULL;
    AVCodec* pCodec = NULL;

	/**
	 * 解码
	 */
	
    //step 4:find  decoder
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);

    if (!pCodec){
        fprintf(stderr,"avcodec_find_decoder error");
        return;
    }
    //step 5:get one instance of AVCodecContext,decode need it.
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
	
    //step 6: open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        fprintf(stderr,"avcodec_open2 error");
        return;
    }
    

    pFrame = av_frame_alloc();
    pFrameTranslate = av_frame_alloc();
	numBytes = av_image_get_buffer_size(translate_format, pCodecCtx->width, pCodecCtx->height,1);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	av_image_fill_arrays(pFrameTranslate->data,pFrameTranslate->linesize,buffer, translate_format, pCodecCtx->width, pCodecCtx->height,1);

    struct SwsContext* sws_ctx = NULL;
	//图像色彩空间转换（初始化）
    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, translate_format, SWS_BILINEAR, NULL, NULL, NULL);

    AVPacket packet;
    i = 0;
    //step 7:read frame
    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        int frameFinished = 0;
		if (packet.stream_index == videoStream)			//处理视频
		{			
			ret = avcodec_send_packet(pCodecCtx, &packet);
		    if (ret < 0) {
		        fprintf(stderr, "Error sending a packet for decoding\n");
		        break;
		    }

		    while (ret >= 0) {
		        ret = avcodec_receive_frame(pCodecCtx, pFrame);
		        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		            break;
		        else if (ret < 0) {
		            fprintf(stderr, "Error during decoding\n");
		            break;
		        }
		        fflush(stdout);	
				//转换
				//如果srcSliceY=0，srcSliceH=height，表示一次性处理完整个图像,也可多线程分别处理
	            sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0,
	                pFrame->height, pFrameTranslate->data, pFrameTranslate->linesize);

	            if (++i <= 5)
	            {
	                //step 8:save frame
	                saveFrame(pFrameTranslate, pCodecCtx->width, pCodecCtx->height, i,outfilename);
	            }
		    }
		}
    }
	
    //release resource
    av_packet_unref(&packet);
    av_free(buffer);
    av_frame_free(&pFrameTranslate);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
//    avcodec_close(pCodecCtxOrg);
    avformat_close_input(&pFormatCtx);
}

int main(int argc, char **argv)
{
    const char *output_type;

    if (argc < 2) {
        printf("usage: %s input_file\n"
               "example: ./demo test.flv",
               argv[0]);
        return 1;
    }
    video_decode_example("hello%02d.ppm", argv[1]);

    return 0;
}
