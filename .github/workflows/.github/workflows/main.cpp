#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" {
    u32 __nx_applet_type = AppletType_None; 
    size_t __nx_heap_size = 0x400000; 
}

#define SWITCH_SAMPLERATE 48000
#define SWITCH_CHANNELS 2
#define BUFFER_SIZE 0x4000

bool g_isRunning = true;
bool g_isPlaying = false;
u64 g_currentTitleId = 0;       
u64 g_lastTitleId = 0;          
char g_currentMusicPath[256];   

u64 getActiveTitleId() {
    u64 titleId = 0;
    PdmAppletQuery query;
    if (R_SUCCEEDED(pdmCmdGetForegroundApplet(&query))) {
        titleId = query.title_id;
    }
    return titleId;
}

void audioThreadFunc(void* arg) {
    drmp3 mp3;
    bool mp3Initialized = false;

    AudioOutSource source;
    if (R_FAILED(audoutInitialize(&source))) return;
    audoutStartAudioOut();

    s16* pcmBuffer = (s16*)malloc(BUFFER_SIZE);
    AudioOutBuffer audioBuffer;

    strcpy(g_currentMusicPath, "sdmc:/music/menu.mp3");

    while (g_isRunning) {
        if (g_currentTitleId != g_lastTitleId) {
            g_lastTitleId = g_currentTitleId;
            
            if (mp3Initialized) {
                drmp3_uninit(&mp3);
                mp3Initialized = false;
            }

            if (g_currentTitleId == 0 || g_currentTitleId == 0x0100000000001000) {
                snprintf(g_currentMusicPath, sizeof(g_currentMusicPath), "sdmc:/music/menu.mp3");
            } else {
                snprintf(g_currentMusicPath, sizeof(g_currentMusicPath), "sdmc:/music/%016lx.mp3", g_currentTitleId);
            }

            if (!drmp3_init_file(&mp3, g_currentMusicPath, NULL)) {
                drmp3_init_file(&mp3, "sdmc:/music/default.mp3", NULL);
            }
            mp3Initialized = true;
            g_isPlaying = true;
        }

        if (g_isPlaying && mp3Initialized) {
            drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16(&mp3, BUFFER_SIZE / (SWITCH_CHANNELS * sizeof(s16)), pcmBuffer);
            
            if (framesRead == 0) {
                drmp3_seek_to_start_of_stream(&mp3); 
                continue;
            }

            size_t bytesToWrite = framesRead * SWITCH_CHANNELS * sizeof(s16);

            audioBuffer.next = NULL;
            audioBuffer.buffer = (u8*)pcmBuffer;
            audioBuffer.buffer_size = bytesToWrite;
            audioBuffer.data_size = bytesToWrite;
            audioBuffer.data_offset = 0;

            AudioOutBuffer* releasedBuffer;
            u32 releasedCount = 0;
            
            audoutAppendAudioOutBuffer(&audioBuffer);
            audoutWaitPlayFinish(&releasedBuffer, &releasedCount, 1000000000ULL);
        } else {
            svcSleepThread(100000000ULL);
        }
    }

    if (mp3Initialized) drmp3_uninit(&mp3);
    free(pcmBuffer);
    audoutExit();
}

int main(int argc, char **argv) {
    smInitialize();
    fsInitialize();
    pdmInitialize(); 
    
    Thread audioThread;
    threadCreate(&audioThread, audioThreadFunc, NULL, NULL, 0x10000, 0x2C, -2);
    threadStart(&audioThread);

    while (g_isRunning) {
        g_currentTitleId = getActiveTitleId();
        svcSleepThread(1000000000ULL); 
    }

    threadWaitForExit(&audioThread);
    threadClose(&audioThread);

    pdmExit();
    fsExit();
    smExit();
    return 0;
}
