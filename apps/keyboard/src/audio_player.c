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

#include "audio_player.h"

static void set_volume(void)
{
    system("amixer -c 0 cset name='Master Playback Volume' 90% 2>&1");
}

struct audio_player_instance {
    pthread_t thr;
    volatile int running;
    volatile int started;
    char *path;
    int one_shot;
    volatile int64_t samples_played;
};

static int setup_pcm(snd_pcm_t **ppcm, unsigned rate, unsigned channels)
{
    int rc;
    snd_pcm_t *pcm = NULL;
    const char *dev = getenv("PGS_SND_CARD");
    if (!dev) dev = "default"; 

    if ((rc = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
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
        snd_pcm_close(pcm);
        return rc;
    }

    if ((rc = snd_pcm_prepare(pcm)) < 0) {
        snd_pcm_close(pcm);
        return rc;
    }

    *ppcm = pcm;
    return 0;
}

static void *player_thread(void *arg)
{
    struct audio_player_instance *inst = (struct audio_player_instance *)arg;
    const char *arg_str = inst->path;
    snd_pcm_t *pcm = NULL;
    int rc;

    unsigned dst_rate = 48000;
    unsigned dst_channels = 2;

    rc = setup_pcm(&pcm, dst_rate, dst_channels);
    if (rc < 0) {
        set_volume();
        inst->started = 0;
        return NULL;
    }

    // Set volume once at startup
    static int volume_set = 0;
    if (!volume_set) {
        set_volume();
        volume_set = 1;
    }

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

    inst->running = 1;
    inst->started = 1;
    inst->samples_played = 0;

    int loop = 1; // 默认循环播放
    while (inst->running && loop) {
        if (list_count > 0) {
            for (int idx = 0; inst->running && idx < list_count; ++idx) {
                const char *path = list_items[idx];

                fmt = NULL; dec = NULL; swr = NULL; stream_index = -1;
                if (avformat_open_input(&fmt, path, NULL, NULL) < 0 || !fmt) {
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

                    while (inst->running) {
                        if (av_read_frame(fmt, pkt) < 0) break; // track结束
                        if (pkt->stream_index != stream_index) { av_packet_unref(pkt); continue; }
                        if (avcodec_send_packet(dec, pkt) == 0) {
                            while (avcodec_receive_frame(dec, frame) == 0) {
                                int out_nb = swr_convert(swr, &out_buf, out_nb_samples_alloc,
                                                         (const uint8_t **)frame->extended_data, frame->nb_samples);
                                if (out_nb > 0) {
                                    int frames = out_nb; int16_t *samples = (int16_t *)out_buf;
                                    while (frames > 0 && inst->running) {
                                        rc = (int)snd_pcm_writei(pcm, samples, frames);
                                        if (rc == -EPIPE) { snd_pcm_recover(pcm, rc, 1); continue; }
                                        else if (rc < 0) { break; }
                                        inst->samples_played += rc;
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

            while (inst->running) {
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
                            while (frames > 0 && inst->running) {
                                rc = (int)snd_pcm_writei(pcm, samples, frames);
                                if (rc == -EPIPE) { snd_pcm_recover(pcm, rc, 1); continue; }
                                else if (rc < 0) { break; }
                                inst->samples_played += rc;
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
    inst->started = 0;
    return NULL;
}

audio_player_handle audio_player_create(void)
{
    struct audio_player_instance *inst = calloc(1, sizeof(struct audio_player_instance));
    return inst;
}

static int audio_player_start_internal(audio_player_handle handle, const char *path)
{
    if (!handle) return -1;
    struct audio_player_instance *inst = (struct audio_player_instance *)handle;
    
    if (inst->started) return 0;
    
    char *p = strdup(path ? path : "");
    if (!p) return -1;
    
    inst->path = p;
    inst->running = 0;
    inst->started = 0;
    
    if (pthread_create(&inst->thr, NULL, player_thread, inst) != 0) {
        free(p);
        inst->path = NULL;
        return -1;
    }
    pthread_detach(inst->thr);
    return 0;
}

int audio_player_start_ex(audio_player_handle handle, const char *path)
{
    return audio_player_start_internal(handle, path);
}

int audio_player_stop(audio_player_handle handle)
{
    if (!handle) return -1;
    struct audio_player_instance *inst = (struct audio_player_instance *)handle;
    
    if (!inst->started) return 0;
    
    inst->running = 0;
    for (int i = 0; i < 50 && inst->started; ++i) usleep(20000);
    
    if (inst->path) {
        free(inst->path);
        inst->path = NULL;
    }
    
    return 0;
}

int audio_player_is_running(audio_player_handle handle)
{
    if (!handle) return 0;
    struct audio_player_instance *inst = (struct audio_player_instance *)handle;
    return inst->started;
}

void audio_player_destroy(audio_player_handle handle)
{
    if (!handle) return;
    struct audio_player_instance *inst = (struct audio_player_instance *)handle;
    
    audio_player_stop(handle);
    
    if (inst->path) {
        free(inst->path);
        inst->path = NULL;
    }
    
    free(inst);
}

int audio_player_start_list_ex(audio_player_handle handle, const char *list_paths)
{
    return audio_player_start_internal(handle, list_paths);
}

static audio_player_handle g_global_player = NULL;

int audio_player_start(const char *path)
{
    if (!g_global_player) {
        g_global_player = audio_player_create();
    }
    if (!g_global_player) return -1;
    return audio_player_start_ex(g_global_player, path);
}

int audio_player_start_list(const char *list_paths)
{
    if (!g_global_player) {
        g_global_player = audio_player_create();
    }
    if (!g_global_player) return -1;
    return audio_player_start_list_ex(g_global_player, list_paths);
}

int64_t audio_player_get_clock(audio_player_handle handle)
{
    if (!handle) return -1;
    struct audio_player_instance *inst = (struct audio_player_instance *)handle;
    if (!inst->started) return -1;
    return (inst->samples_played * 1000) / 48000;
}

int64_t audio_player_get_global_clock(void)
{
    if (!g_global_player) return -1;
    return audio_player_get_clock(g_global_player);
}

int audio_player_restart(audio_player_handle handle)
{
    if (!handle) return -1;
    struct audio_player_instance *inst = (struct audio_player_instance *)handle;
    if (!inst->path) return -1;
    char *path = strdup(inst->path);
    if (!path) return -1;
    audio_player_stop(handle);
    int rc = audio_player_start_ex(handle, path);
    free(path);
    return rc;
}

static audio_player_handle g_beep_player = NULL;
static snd_pcm_t *g_beep_pcm = NULL;
static int16_t *g_beep_buffer = NULL;
static int g_beep_samples = 0;

__attribute__((constructor))
static void beep_init(void)
{
    set_volume();
    
    const char *dev = getenv("PGS_SND_CARD");
    if (!dev) dev = "default";

    int rc = snd_pcm_open(&g_beep_pcm, dev, SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        g_beep_pcm = NULL;
        return;
    }

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(g_beep_pcm, params);
    snd_pcm_hw_params_set_access(g_beep_pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(g_beep_pcm, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(g_beep_pcm, params, 1);
    snd_pcm_hw_params_set_rate(g_beep_pcm, params, 48000, 0);
    snd_pcm_hw_params(g_beep_pcm, params);
    snd_pcm_prepare(g_beep_pcm);

    const int freq = 400;
    const int dur = 40;
    const int sr = 48000;
    g_beep_samples = (sr * dur) / 1000;
    g_beep_buffer = malloc(g_beep_samples * sizeof(int16_t));
    for (int i = 0; i < g_beep_samples; i++) {
        double t = (double)i / sr;
        double env = 1.0;
        if (i < g_beep_samples / 10) env = (double)i / (g_beep_samples / 10);
        else if (i > g_beep_samples * 9 / 10) env = (double)(g_beep_samples - i) / (g_beep_samples / 10);
        g_beep_buffer[i] = (int16_t)(20000 * env * sin(2 * 3.14159 * freq * t));
    }
}

int audio_player_keybeep(uint8_t mode, const char *path)
{
    // Ensure volume is set before playing
    static int volume_checked = 0;
    if (!volume_checked) {
        set_volume();
        volume_checked = 1;
    }
    
    if (mode == 1 && path) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "aplay -q %s &", path);
        system(cmd);
    } else if (g_beep_pcm && g_beep_buffer) {
        snd_pcm_prepare(g_beep_pcm);
        snd_pcm_writei(g_beep_pcm, g_beep_buffer, g_beep_samples);
    }
    return 0;
}
