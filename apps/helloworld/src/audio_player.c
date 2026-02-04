#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>

static pthread_t g_thr;
static volatile int g_running = 0;
static volatile int g_started = 0;

static int setup_pcm(snd_pcm_t **ppcm, unsigned rate, unsigned channels)
{
    int rc;
    snd_pcm_t *pcm = NULL;
    const char *dev = getenv("PGS_SND_CARD");
    if (!dev) dev = "default"; 

    if ((rc = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "ALSA: cannot open device %s: %s\n", dev, snd_strerror(rc));
        return rc;
    }

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm, params);
    snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, params, channels);
    snd_pcm_hw_params_set_rate(pcm, params, rate, 0);

    snd_pcm_uframes_t period = 1024;
    snd_pcm_hw_params_set_period_size(pcm, params, period, 0);
    snd_pcm_uframes_t buffer = period * 4;
    snd_pcm_hw_params_set_buffer_size(pcm, params, buffer);

    if ((rc = snd_pcm_hw_params(pcm, params)) < 0) {
        fprintf(stderr, "ALSA: cannot set params: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        return rc;
    }

    if ((rc = snd_pcm_prepare(pcm)) < 0) {
        fprintf(stderr, "ALSA: prepare failed: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        return rc;
    }

    *ppcm = pcm;
    return 0;
}

static void *player_thread(void *arg)
{
    const char *arg_str = (const char *)arg;
    snd_pcm_t *pcm = NULL;
    int rc;

    unsigned dst_rate = 48000;
    unsigned dst_channels = 2;

    rc = setup_pcm(&pcm, dst_rate, dst_channels);
    if (rc < 0) return NULL;

    AVFormatContext *fmt = NULL;
    AVCodecContext *dec = NULL;
    SwrContext *swr = NULL;
    int stream_index = -1;

    av_log_set_level(AV_LOG_ERROR);

    // 解析播放列表（以 ':' 分隔）；如果没有 ':' 则作为单文件循环播放
    char *list_dup = NULL;
    int list_count = 0;
    char **list_items = NULL;
    if (arg_str && strchr(arg_str, ':')) {
        list_dup = strdup(arg_str);
        char *saveptr = NULL;
        for (char *tok = strtok_r(list_dup, ":", &saveptr); tok; tok = strtok_r(NULL, ":", &saveptr)) {
            list_items = (char **)realloc(list_items, sizeof(char *) * (list_count + 1));
            list_items[list_count++] = strdup(tok);
        }
    }

    g_running = 1;
    g_started = 1;

    int loop = 1; // 默认循环播放
    while (g_running && loop) {
        if (list_count > 0) {
            for (int idx = 0; g_running && idx < list_count; ++idx) {
                const char *path = list_items[idx];

                fmt = NULL; dec = NULL; swr = NULL; stream_index = -1;
                if (avformat_open_input(&fmt, path, NULL, NULL) < 0 || !fmt) {
                    fprintf(stderr, "FFmpeg: open input failed: %s\n", path);
                    goto next_track;
                }
                if (avformat_find_stream_info(fmt, NULL) < 0) goto next_track;
                for (unsigned i = 0; i < fmt->nb_streams; ++i) {
                    if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { stream_index = (int)i; break; }
                }
                if (stream_index < 0) goto next_track;

                {
                    AVCodecParameters *par = fmt->streams[stream_index]->codecpar;
                    const AVCodec *codec = avcodec_find_decoder(par->codec_id);
                    if (!codec) goto next_track;
                    dec = avcodec_alloc_context3(codec);
                    if (!dec) goto next_track;
                    if (avcodec_parameters_to_context(dec, par) < 0) goto next_track;
                    if (avcodec_open2(dec, codec, NULL) < 0) goto next_track;

                    int64_t src_ch_layout = par->channel_layout ? par->channel_layout : av_get_default_channel_layout(par->channels);
                    enum AVSampleFormat src_fmt = dec->sample_fmt;
                    int src_rate = dec->sample_rate;

                    swr = swr_alloc_set_opts(NULL,
                                             AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, (int)dst_rate,
                                             src_ch_layout,      src_fmt,            src_rate,
                                             0, NULL);
                    if (!swr || swr_init(swr) < 0) goto next_track;
                }

                {
                    AVPacket *pkt = av_packet_alloc();
                    AVFrame *frame = av_frame_alloc();
                    int out_linesize = 0; uint8_t *out_buf = NULL; int out_nb_samples_alloc = 2048;
                    av_samples_alloc(&out_buf, &out_linesize, (int)dst_channels, out_nb_samples_alloc, AV_SAMPLE_FMT_S16, 0);

                    while (g_running) {
                        if (av_read_frame(fmt, pkt) < 0) break; // track结束
                        if (pkt->stream_index != stream_index) { av_packet_unref(pkt); continue; }
                        if (avcodec_send_packet(dec, pkt) == 0) {
                            while (avcodec_receive_frame(dec, frame) == 0) {
                                int out_nb = swr_convert(swr, &out_buf, out_nb_samples_alloc,
                                                         (const uint8_t **)frame->extended_data, frame->nb_samples);
                                if (out_nb > 0) {
                                    int frames = out_nb; int16_t *samples = (int16_t *)out_buf;
                                    while (frames > 0 && g_running) {
                                        rc = (int)snd_pcm_writei(pcm, samples, frames);
                                        if (rc == -EPIPE) { snd_pcm_recover(pcm, rc, 1); continue; }
                                        else if (rc < 0) { fprintf(stderr, "ALSA: write error: %s\n", snd_strerror(rc)); break; }
                                        frames -= rc; samples += rc * dst_channels;
                                    }
                                }
                            }
                        }
                        av_packet_unref(pkt);
                    }

                    if (out_buf) av_freep(&out_buf);
                    if (frame) av_frame_free(&frame);
                    if (pkt) av_packet_free(&pkt);
                }

            next_track:
                if (dec) avcodec_free_context(&dec);
                if (fmt) avformat_close_input(&fmt);
                if (swr) swr_free(&swr);
            }
            // 一轮播放完成后继续循环
            continue;
        } else {
            // 单文件循环
            const char *path = arg_str;
            fmt = NULL; dec = NULL; swr = NULL; stream_index = -1;
            if (!path || avformat_open_input(&fmt, path, NULL, NULL) < 0 || !fmt) {
                fprintf(stderr, "FFmpeg: open input failed: %s\n", path ? path : "(null)");
                break;
            }
            if (avformat_find_stream_info(fmt, NULL) < 0) break;
            for (unsigned i = 0; i < fmt->nb_streams; ++i) {
                if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { stream_index = (int)i; break; }
            }
            if (stream_index < 0) break;

            AVCodecParameters *par = fmt->streams[stream_index]->codecpar;
            const AVCodec *codec = avcodec_find_decoder(par->codec_id);
            if (!codec) break;
            dec = avcodec_alloc_context3(codec);
            if (!dec) break;
            if (avcodec_parameters_to_context(dec, par) < 0) break;
            if (avcodec_open2(dec, codec, NULL) < 0) break;

            int64_t src_ch_layout = par->channel_layout ? par->channel_layout : av_get_default_channel_layout(par->channels);
            enum AVSampleFormat src_fmt = dec->sample_fmt;
            int src_rate = dec->sample_rate;

            swr = swr_alloc_set_opts(NULL,
                                     AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, (int)dst_rate,
                                     src_ch_layout,      src_fmt,            src_rate,
                                     0, NULL);
            if (!swr || swr_init(swr) < 0) break;

            AVPacket *pkt = av_packet_alloc();
            AVFrame *frame = av_frame_alloc();
            int out_linesize = 0; uint8_t *out_buf = NULL; int out_nb_samples_alloc = 2048;
            av_samples_alloc(&out_buf, &out_linesize, (int)dst_channels, out_nb_samples_alloc, AV_SAMPLE_FMT_S16, 0);

            while (g_running) {
                if (av_read_frame(fmt, pkt) < 0) {
                    // 单曲：回到开头继续
                    av_seek_frame(fmt, stream_index, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(dec);
                    continue;
                }
                if (pkt->stream_index != stream_index) { av_packet_unref(pkt); continue; }
                if (avcodec_send_packet(dec, pkt) == 0) {
                    while (avcodec_receive_frame(dec, frame) == 0) {
                        int out_nb = swr_convert(swr, &out_buf, out_nb_samples_alloc,
                                                 (const uint8_t **)frame->extended_data, frame->nb_samples);
                        if (out_nb > 0) {
                            int frames = out_nb; int16_t *samples = (int16_t *)out_buf;
                            while (frames > 0 && g_running) {
                                rc = (int)snd_pcm_writei(pcm, samples, frames);
                                if (rc == -EPIPE) { snd_pcm_recover(pcm, rc, 1); continue; }
                                else if (rc < 0) { fprintf(stderr, "ALSA: write error: %s\n", snd_strerror(rc)); break; }
                                frames -= rc; samples += rc * dst_channels;
                            }
                        }
                    }
                }
                av_packet_unref(pkt);
            }

            if (out_buf) av_freep(&out_buf);
            if (frame) av_frame_free(&frame);
            if (pkt) av_packet_free(&pkt);
            if (dec) avcodec_free_context(&dec);
            if (fmt) avformat_close_input(&fmt);
            if (swr) swr_free(&swr);
        }
    }

out:
    if (pcm) snd_pcm_close(pcm);
    if (list_items) {
        for (int i = 0; i < list_count; ++i) free(list_items[i]);
        free(list_items);
    }
    if (list_dup) free(list_dup);
    g_started = 0;
    return NULL;
}

int audio_player_start(const char *path)
{
    if (g_started) return 0; // 已在运行，避免重复启动
    char *p = strdup(path ? path : "");
    if (!p) return -1;
    if (pthread_create(&g_thr, NULL, player_thread, p) != 0) {
        free(p);
        return -1;
    }
    pthread_detach(g_thr);
    return 0;
}

int audio_player_stop(void)
{
    if (!g_started) return 0;
    g_running = 0;
    // 线程是分离态，简单等待片刻结束
    for (int i = 0; i < 50 && g_started; ++i) usleep(20000);
    return 0;
}

int audio_player_is_running(void)
{
    return g_started;
}

int audio_player_start_list(const char *list_paths)
{
    if (g_started) return 0; // 已在运行，避免重复启动
    char *p = strdup(list_paths ? list_paths : "");
    if (!p) return -1;
    if (pthread_create(&g_thr, NULL, player_thread, p) != 0) {
        free(p);
        return -1;
    }
    pthread_detach(g_thr);
    return 0;
}
