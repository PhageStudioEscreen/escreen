#include "av_player.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../../lvgl/lvgl.h"

#if !defined(__linux__)

av_player_handle av_player_create(struct lv_obj_t * parent, const char * path, bool loop)
{
    (void)parent;
    (void)path;
    (void)loop;
    return NULL;
}

struct lv_obj_t * av_player_get_obj(av_player_handle handle)
{
    (void)handle;
    return NULL;
}

int av_player_start(av_player_handle handle)
{
    (void)handle;
    return -1;
}

int av_player_stop(av_player_handle handle)
{
    (void)handle;
    return -1;
}

void av_player_destroy(av_player_handle handle)
{
    (void)handle;
}

#else

#include <pthread.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2
#define VIDEO_SYNC_EARLY_SEC 0.03
#define VIDEO_DROP_LATE_SEC 0.15

struct av_player_instance {
    lv_obj_t * img;
    lv_image_dsc_t imgdsc;
    lv_timer_t * timer;
    pthread_t thread;
    pthread_mutex_t lock;

    int running;
    int started;
    bool loop;
    char * path;

    uint8_t * frame_buf[2];
    int frame_front;
    int frame_ready;
    size_t frame_size;

    double audio_clock;
    int audio_clock_valid;
    int64_t samples_written;

    FILE * log;
};

static int setup_pcm(snd_pcm_t ** ppcm, unsigned rate, unsigned channels)
{
    int rc;
    snd_pcm_t * pcm = NULL;
    const char * dev = getenv("PGS_SND_CARD");
    if(!dev) {
        dev = "default";
    }

    if((rc = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        return rc;
    }

    snd_pcm_hw_params_t * params;
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

    if((rc = snd_pcm_hw_params(pcm, params)) < 0) {
        snd_pcm_close(pcm);
        return rc;
    }

    if((rc = snd_pcm_prepare(pcm)) < 0) {
        snd_pcm_close(pcm);
        return rc;
    }

    *ppcm = pcm;
    return 0;
}

static double get_audio_clock(struct av_player_instance * inst, snd_pcm_t * pcm)
{
    if(!inst->audio_clock_valid) {
        return 0.0;
    }

    snd_pcm_sframes_t delay_frames = 0;
    if(pcm) {
        snd_pcm_delay(pcm, &delay_frames);
        if(delay_frames < 0) delay_frames = 0;
    }

    double played = (double)(inst->samples_written - delay_frames) / (double)AUDIO_SAMPLE_RATE;
    return inst->audio_clock + played;
}

static int av_interrupt_cb(void * ctx)
{
    struct av_player_instance * inst = (struct av_player_instance *)ctx;
    return inst ? !inst->running : 1;
}

static void publish_video_frame(struct av_player_instance * inst, const uint8_t * src, size_t size)
{
    pthread_mutex_lock(&inst->lock);
    int back = inst->frame_front ? 0 : 1;
    if(inst->frame_buf[back] && src && size <= inst->frame_size) {
        if(src != inst->frame_buf[back]) {
            memcpy(inst->frame_buf[back], src, size);
        }
        inst->frame_ready = back;
    }
    pthread_mutex_unlock(&inst->lock);
}

static void * av_decode_thread(void * arg)
{
    struct av_player_instance * p = (struct av_player_instance *)arg;
    p->log = NULL;

    // TEST: return immediately - disable for now
    //p->started = 1;
    //p->running = 1;
    //return NULL;

    // Full FFmpeg decode code starts here
    AVFormatContext * fmt = NULL;
    AVCodecContext * audio_dec = NULL;
    AVCodecContext * video_dec = NULL;
    SwrContext * swr = NULL;
    struct SwsContext * sws = NULL;
    snd_pcm_t * pcm = NULL;
    int audio_index = -1;
    int video_index = -1;

    fprintf(stderr, "[AV_PLAYER] Opening: %s\n", p->path);
    if(avformat_open_input(&fmt, p->path, NULL, NULL) < 0 || !fmt) {
        fprintf(stderr, "[AV_PLAYER] Failed to open: %s\n", p->path);
        goto done;
    }
    fprintf(stderr, "[AV_PLAYER] Opened successfully\n");
    fmt->interrupt_callback.callback = av_interrupt_cb;
    fmt->interrupt_callback.opaque = p;

    if(avformat_find_stream_info(fmt, NULL) < 0) {
        goto done;
    }

    for(unsigned i = 0; i < fmt->nb_streams; ++i) {
        if(fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < 0) {
            audio_index = (int)i;
        } else if(fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_index < 0) {
            video_index = (int)i;
        }
    }

    fprintf(stderr, "[AV_PLAYER] audio_index=%d video_index=%d\n", audio_index, video_index);
    if(audio_index < 0 || video_index < 0) {
        fprintf(stderr, "[AV_PLAYER] No audio or video stream\n");
        goto done;
    }

    {
        AVCodecParameters * par = fmt->streams[audio_index]->codecpar;
        const AVCodec * codec = avcodec_find_decoder(par->codec_id);
        if(!codec) goto done;
        audio_dec = avcodec_alloc_context3(codec);
        if(!audio_dec) goto done;
        if(avcodec_parameters_to_context(audio_dec, par) < 0) goto done;
        if(avcodec_open2(audio_dec, codec, NULL) < 0) goto done;
    }

    {
        AVCodecParameters * par = fmt->streams[video_index]->codecpar;
        const AVCodec * codec = avcodec_find_decoder(par->codec_id);
        if(!codec) goto done;
        video_dec = avcodec_alloc_context3(codec);
        if(!video_dec) goto done;
        if(avcodec_parameters_to_context(video_dec, par) < 0) goto done;
        if(avcodec_open2(video_dec, codec, NULL) < 0) goto done;
    }

    if(setup_pcm(&pcm, AUDIO_SAMPLE_RATE, AUDIO_CHANNELS) < 0) {
        goto done;
    }

    {
        int64_t src_layout = audio_dec->channel_layout;
        if(src_layout == 0) {
            src_layout = av_get_default_channel_layout(audio_dec->channels);
        }
        swr = swr_alloc_set_opts(NULL,
                                 AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, AUDIO_SAMPLE_RATE,
                                 src_layout, audio_dec->sample_fmt, audio_dec->sample_rate,
                                 0, NULL);
        if(!swr || swr_init(swr) < 0) {
            goto done;
        }
    }

    sws = sws_getContext(video_dec->width, video_dec->height, video_dec->pix_fmt,
                         video_dec->width, video_dec->height, AV_PIX_FMT_BGRA,
                         SWS_BILINEAR, NULL, NULL, NULL);
    if(!sws) {
        goto done;
    }

    p->frame_size = (size_t)video_dec->width * (size_t)video_dec->height * 4;
    p->frame_buf[0] = malloc(p->frame_size);
    p->frame_buf[1] = malloc(p->frame_size);
    if(!p->frame_buf[0] || !p->frame_buf[1]) {
        goto done;
    }
    memset(p->frame_buf[0], 0, p->frame_size);
    memset(p->frame_buf[1], 0, p->frame_size);
    p->frame_front = 0;
    p->frame_ready = -1;

    p->imgdsc.header.w = video_dec->width;
    p->imgdsc.header.h = video_dec->height;
    p->imgdsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    p->imgdsc.header.stride = video_dec->width * 4;
    p->imgdsc.data_size = (uint32_t)p->frame_size;
    p->imgdsc.data = p->frame_buf[p->frame_front];

    AVPacket * pkt = av_packet_alloc();
    AVFrame * frame = av_frame_alloc();
    AVFrame * a_frame = av_frame_alloc();
    uint8_t * audio_buf = NULL;
    int audio_buf_linesize = 0;
    int audio_buf_samples = 2048;
    if(av_samples_alloc(&audio_buf, &audio_buf_linesize, AUDIO_CHANNELS, audio_buf_samples, AV_SAMPLE_FMT_S16, 0) < 0) {
        goto done;
    }

    p->samples_written = 0;
    p->audio_clock = 0.0;
    p->audio_clock_valid = 0;

    p->running = 1;
    p->started = 1;

    while(p->running) {
        if(av_read_frame(fmt, pkt) < 0) {
            if(p->loop) {
                av_seek_frame(fmt, -1, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(audio_dec);
                avcodec_flush_buffers(video_dec);
                p->samples_written = 0;
                p->audio_clock_valid = 0;
                continue;
            }
            break;
        }

        if(pkt->stream_index == audio_index) {
            if(avcodec_send_packet(audio_dec, pkt) == 0) {
                while(avcodec_receive_frame(audio_dec, a_frame) == 0 && p->running) {
                    int64_t pts = av_frame_get_best_effort_timestamp(a_frame);
                    if(!p->audio_clock_valid && pts != AV_NOPTS_VALUE) {
                        double pts_sec = pts * av_q2d(fmt->streams[audio_index]->time_base);
                        p->audio_clock = pts_sec;
                        p->audio_clock_valid = 1;
                        p->samples_written = 0;
                    }

                    int out_nb = swr_convert(swr, &audio_buf, audio_buf_samples,
                                             (const uint8_t **)a_frame->extended_data, a_frame->nb_samples);
                    if(out_nb > 0) {
                        int frames_left = out_nb;
                        int16_t * samples = (int16_t *)audio_buf;
                        while(frames_left > 0 && p->running) {
                            int rc = (int)snd_pcm_writei(pcm, samples, frames_left);
                            if(rc == -EPIPE) {
                                snd_pcm_recover(pcm, rc, 1);
                                continue;
                            } else if(rc < 0) {
                                break;
                            }
                            frames_left -= rc;
                            samples += rc * AUDIO_CHANNELS;
                            p->samples_written += rc;
                        }
                    }
                    av_frame_unref(a_frame);
                }
            }
        } else if(pkt->stream_index == video_index) {
            if(avcodec_send_packet(video_dec, pkt) == 0) {
                while(avcodec_receive_frame(video_dec, frame) == 0 && p->running) {
                    int64_t pts = av_frame_get_best_effort_timestamp(frame);
                    double video_pts = 0.0;
                    if(pts != AV_NOPTS_VALUE) {
                        video_pts = pts * av_q2d(fmt->streams[video_index]->time_base);
                    }

                    if(p->audio_clock_valid) {
                        while(p->running) {
                            double audio_clock = get_audio_clock(p, pcm);
                            double diff = video_pts - audio_clock;
                            if(diff <= VIDEO_SYNC_EARLY_SEC) {
                                if(diff < -VIDEO_DROP_LATE_SEC) {
                                    break;
                                }
                                break;
                            }
                            usleep(5000);
                        }
                    }

                    if(p->running && p->frame_buf[0] && p->frame_buf[1]) {
                        uint8_t * dst_data[4] = { 0 };
                        int dst_linesize[4] = { 0 };
                        dst_data[0] = p->frame_buf[p->frame_front ? 0 : 1];
                        dst_linesize[0] = video_dec->width * 4;
                        if(sws_scale(sws, (const uint8_t * const *)frame->data, frame->linesize, 0,
                                     video_dec->height, dst_data, dst_linesize) > 0) {
                            fprintf(stderr, "[AV_PLAYER] Publishing frame\n");
                            publish_video_frame(p, dst_data[0], p->frame_size);
                        }
                    }

                    av_frame_unref(frame);
                }
            }
        }

        av_packet_unref(pkt);
    }

    if(audio_buf) av_freep(&audio_buf);
    if(pkt) av_packet_free(&pkt);
    if(frame) av_frame_free(&frame);
    if(a_frame) av_frame_free(&a_frame);

done:
    if(pcm) snd_pcm_close(pcm);
    if(swr) swr_free(&swr);
    if(sws) sws_freeContext(sws);
    if(audio_dec) avcodec_free_context(&audio_dec);
    if(video_dec) avcodec_free_context(&video_dec);
    if(fmt) avformat_close_input(&fmt);

    p->started = 0;
    return NULL;
}

static void av_player_timer_cb(lv_timer_t * timer)
{
    struct av_player_instance * p = (struct av_player_instance *)lv_timer_get_user_data(timer);
    if(!p) return;

    pthread_mutex_lock(&p->lock);
    if(p->frame_ready >= 0 && p->imgdsc.data && p->imgdsc.header.w > 0 && p->imgdsc.header.h > 0) {
        p->frame_front = p->frame_ready;
        p->frame_ready = -1;
        p->imgdsc.data = p->frame_buf[p->frame_front];
        lv_image_set_src(p->img, &p->imgdsc);
        lv_obj_invalidate(p->img);
    }
    pthread_mutex_unlock(&p->lock);
}

av_player_handle av_player_create(struct lv_obj_t * parent, const char * path, bool loop)
{
    if(!parent || !path) {
        return NULL;
    }

    struct av_player_instance * p = calloc(1, sizeof(struct av_player_instance));
    if(!p) {
        return NULL;
    }

    p->path = strdup(path);
    if(!p->path) {
        free(p);
        return NULL;
    }

    pthread_mutex_init(&p->lock, NULL);
    p->loop = loop;
    
    p->img = lv_image_create(parent);
    lv_image_set_src(p->img, &p->imgdsc);
    p->imgdsc.data = NULL;
    p->frame_ready = -1;
    
    // Create timer to update LVGL image from decoded frames (~60fps)
    p->timer = lv_timer_create(av_player_timer_cb, 16, p);
    if(!p->timer) {
        lv_obj_del(p->img);
        pthread_mutex_destroy(&p->lock);
        free(p->path);
        free(p);
        return NULL;
    }
    
    if(pthread_create(&p->thread, NULL, av_decode_thread, p) != 0) {
        lv_obj_del(p->img);
        pthread_mutex_destroy(&p->lock);
        free(p->path);
        free(p);
        return NULL;
    }
    
    return p;
}

struct lv_obj_t * av_player_get_obj(av_player_handle handle)
{
    struct av_player_instance * p = (struct av_player_instance *)handle;
    if(!p) return NULL;
    return p->img;
}

int av_player_start(av_player_handle handle)
{
    struct av_player_instance * p = (struct av_player_instance *)handle;
    if(!p) return -1;
    return 0;
}

int av_player_stop(av_player_handle handle)
{
    struct av_player_instance * p = (struct av_player_instance *)handle;
    if(!p) return -1;
    p->running = 0;
    if(p->started) {
        pthread_join(p->thread, NULL);
    }
    return 0;
}

void av_player_destroy(av_player_handle handle)
{
    struct av_player_instance * p = (struct av_player_instance *)handle;
    if(!p) return;

    av_player_stop(handle);

    if(p->timer) {
        lv_timer_delete(p->timer);
        p->timer = NULL;
    }

    if(p->img) {
        lv_obj_del(p->img);
        p->img = NULL;
    }

    if(p->frame_buf[0]) free(p->frame_buf[0]);
    if(p->frame_buf[1]) free(p->frame_buf[1]);
    p->frame_buf[0] = NULL;
    p->frame_buf[1] = NULL;

    pthread_mutex_destroy(&p->lock);
    free(p->path);
    free(p);
}

#endif
