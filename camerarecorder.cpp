#include "camerarecorder.h"

CameraRecorder::CameraRecorder(QSharedPointer<Config> c)
{
    config = c;
}

void CameraRecorder::start()
{
    qDebug() << "Cam recorder thread started";
    updSettings();
    //selectCamera();
}

void CameraRecorder::selectCamera()
{
#ifdef __linux__
    SwsContext *img_convert_ctx;
    AVFormatContext* format_ctx = avformat_alloc_context();
    AVCodecContext* codec_ctx = NULL;
    int video_stream_index;
    if (avformat_open_input(&format_ctx, "http://195.196.36.242/mjpg/video.mjpg",
                            NULL, NULL) != 0) {
        qDebug() << "Cannot open RTSP stream";
    }
    else
        qDebug() << "RTSP stream opened with great success";

    av_register_all();
    avformat_network_init();

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        qDebug() << "No stream found in RTSP";
    }

    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream_index = i;
    }
    AVPacket packet;
    av_init_packet(&packet);

    //open output file
    AVFormatContext* output_ctx = avformat_alloc_context();

    AVStream* stream = NULL;
    int cnt = 0;

    //start reading packets from stream and write them to file
    av_read_play(format_ctx);    //play RTSP

    // Get the codec
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        exit(1);
    }

    // Add this to allocate the context by codec
    codec_ctx = avcodec_alloc_context3(codec);

    avcodec_get_context_defaults3(codec_ctx, codec);
    avcodec_copy_context(codec_ctx, format_ctx->streams[video_stream_index]->codec);

    if (avcodec_open2(codec_ctx, codec, NULL) < 0)
        exit(1);

    img_convert_ctx = sws_getContext(codec_ctx->width, codec_ctx->height,
                                     codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
                                     SWS_BICUBIC, NULL, NULL, NULL);

    int size = avpicture_get_size(AV_PIX_FMT_YUV420P, codec_ctx->width,
                                  codec_ctx->height);
    uint8_t* picture_buffer = (uint8_t*) (av_malloc(size));
    AVFrame* picture = av_frame_alloc();
    AVFrame* picture_rgb = av_frame_alloc();
    int size2 = avpicture_get_size(AV_PIX_FMT_RGB24, codec_ctx->width,
                                   codec_ctx->height);
    uint8_t* picture_buffer_2 = (uint8_t*) (av_malloc(size2));
    avpicture_fill((AVPicture *) picture, picture_buffer, AV_PIX_FMT_YUV420P,
                   codec_ctx->width, codec_ctx->height);
    avpicture_fill((AVPicture *) picture_rgb, picture_buffer_2, AV_PIX_FMT_RGB24,
                   codec_ctx->width, codec_ctx->height);

    while (av_read_frame(format_ctx, &packet) >= 0 && cnt < 1000) { //read ~ 1000 frames

        qDebug()  << "1 Frame: " << cnt;
        if (packet.stream_index == video_stream_index) {    //packet is video
            qDebug()  << "2 Is Video";
            if (stream == NULL) {    //create stream in file
                qDebug()  << "3 create stream";
                stream = avformat_new_stream(output_ctx,
                                             format_ctx->streams[video_stream_index]->codec->codec);
                avcodec_copy_context(stream->codec,
                                     format_ctx->streams[video_stream_index]->codec);
                stream->sample_aspect_ratio =
                        format_ctx->streams[video_stream_index]->codec->sample_aspect_ratio;
            }
            int check = 0;
            packet.stream_index = stream->id;
            qDebug() << "4 decoding";
            int result = avcodec_decode_video2(codec_ctx, picture, &check, &packet);
            qDebug() << "Bytes decoded " << result << " check " << check;
            if (cnt > 100)    //cnt < 0)
            {
                sws_scale(img_convert_ctx, picture->data, picture->linesize, 0,
                          codec_ctx->height, picture_rgb->data, picture_rgb->linesize);
                QString file_name = "test.ppm";
                QFile output_file(file_name);
                output_file.open(QIODevice::WriteOnly);
                QDataStream output(&output_file);

                output << "P3 " << codec_ctx->width << " " << codec_ctx->height
                            << " 255\n";
                for (int y = 0; y < codec_ctx->height; y++) {
                    for (int x = 0; x < codec_ctx->width * 3; x++)
                        output
                                << (int) (picture_rgb->data[0]
                                   + y * picture_rgb->linesize[0])[x] << " ";
                }
                output_file.close();
            }
            cnt++;
        }
        av_free_packet(&packet);
        av_init_packet(&packet);
    }
    av_free(picture);
    av_free(picture_rgb);
    av_free(picture_buffer);
    av_free(picture_buffer_2);

    av_read_pause(format_ctx);
    avio_close(output_ctx->pb);
    avformat_free_context(output_ctx);
#endif
}

void CameraRecorder::startRecorder()
{
}

void CameraRecorder::stopRecorder()
{
}

void CameraRecorder::updSettings()
{
    usbCamName = config->getCameraName();
    rtspStream = config->getCameraStreamAddress();
    doRecord = config->getCameraDeviceTrigger();
    recordTime = config->getCameraRecordTime();
}
