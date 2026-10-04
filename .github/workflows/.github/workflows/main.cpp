
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    u32 __nx_applet_type = AppletType_None;
    size_t __nx_heap_size = 0x400000;
}

#define SWITCH_CHANNELS 2
#define AUDIO_BUFFER_SIZE 0x4000

static volatile bool g_isRunning = true;
static volatile u64 g_currentTitleId = 0;
static u64 g_lastTitleId = UINT64_MAX;

static Mutex g_titleMutex;
static char g_currentMusicPath[256];

static u64 getActiveTitleId()
{
    u64 titleId = 0;

    PdmAppletQuery query;

    memset(&query, 0, sizeof(query));

    if (R_SUCCEEDED(pdmCmdGetForegroundApplet(&query)))
    {
        titleId = query.title_id;
    }

    return titleId;
}

static bool openMusicFile(drmp3* mp3, u64 titleId)
{
    char path[256];

    /*
     * HOME / sin aplicación en primer plano
     */
    if (titleId == 0 ||
        titleId == 0x0100000000001000ULL)
    {
        snprintf(
            path,
            sizeof(path),
            "sdmc:/music/menu.mp3"
        );
    }
    else
    {
        /*
         * Ejemplo:
         *
         * Title ID:
         * 0100XXXXXXXXXXXX
         *
         * Archivo:
         * sdmc:/music/0100XXXXXXXXXXXX.mp3
         */
        snprintf(
            path,
            sizeof(path),
            "sdmc:/music/%016llX.mp3",
            (unsigned long long)titleId
        );
    }

    printf("Intentando abrir: %s\n", path);

    /*
     * Intentamos primero la música específica.
     */
    if (drmp3_init_file(mp3, path, NULL))
    {
        strncpy(
            g_currentMusicPath,
            path,
            sizeof(g_currentMusicPath) - 1
        );

        g_currentMusicPath[
            sizeof(g_currentMusicPath) - 1
        ] = '\0';

        printf("Reproduciendo: %s\n", g_currentMusicPath);

        return true;
    }

    /*
     * Si no existe, usamos default.mp3.
     */
    printf("No existe. Usando default.mp3\n");

    if (drmp3_init_file(
            mp3,
            "sdmc:/music/default.mp3",
            NULL))
    {
        strncpy(
            g_currentMusicPath,
            "sdmc:/music/default.mp3",
            sizeof(g_currentMusicPath) - 1
        );

        g_currentMusicPath[
            sizeof(g_currentMusicPath) - 1
        ] = '\0';

        return true;
    }

    printf("ERROR: no se pudo abrir ningun MP3\n");

    g_currentMusicPath[0] = '\0';

    return false;
}

static void audioThreadFunc(void* arg)
{
    (void)arg;

    drmp3 mp3;
    bool mp3Initialized = false;

    AudioOutSource source;

    /*
     * Inicializar salida de audio.
     */
    Result rc = audoutInitialize(&source);

    if (R_FAILED(rc))
    {
        printf(
            "ERROR: audoutInitialize: 0x%08X\n",
            rc
        );

        return;
    }

    audoutStartAudioOut();

    /*
     * Buffer PCM estéreo de 16 bits.
     */
    s16* pcmBuffer =
        (s16*)malloc(AUDIO_BUFFER_SIZE);

    if (!pcmBuffer)
    {
        printf("ERROR: no se pudo reservar pcmBuffer\n");

        audoutExit();

        return;
    }

    AudioOutBuffer audioBuffer;

    memset(
        &audioBuffer,
        0,
        sizeof(audioBuffer)
    );

    while (g_isRunning)
    {
        u64 titleId;

        mutexLock(&g_titleMutex);

        titleId = g_currentTitleId;

        mutexUnlock(&g_titleMutex);

        /*
         * Ha cambiado el juego / aplicación.
         */
        if (titleId != g_lastTitleId)
        {
            g_lastTitleId = titleId;

            printf(
                "Cambio de Title ID: %016llX\n",
                (unsigned long long)titleId
            );

            /*
             * Cerrar MP3 anterior.
             */
            if (mp3Initialized)
            {
                drmp3_uninit(&mp3);
                mp3Initialized = false;
            }

            /*
             * Abrir la nueva música.
             */
            mp3Initialized =
                openMusicFile(
                    &mp3,
                    titleId
                );
        }

        /*
         * No hay música disponible.
         */
        if (!mp3Initialized)
        {
            svcSleepThread(100000000ULL);
            continue;
        }

        /*
         * Número máximo de frames que caben
         * en nuestro buffer.
         *
         * 2 canales x 2 bytes = 4 bytes/frame.
         */
        const drmp3_uint64 framesToRead =
            AUDIO_BUFFER_SIZE /
            (SWITCH_CHANNELS * sizeof(s16));

        drmp3_uint64 framesRead =
            drmp3_read_pcm_frames_s16(
                &mp3,
                framesToRead,
                pcmBuffer
            );

        /*
         * Fin de la canción.
         *
         * Volvemos al principio.
         */
        if (framesRead == 0)
        {
            if (!drmp3_seek_to_pcm_frame(&mp3, 0))
            {
                printf(
                    "ERROR: no se pudo reiniciar el MP3\n"
                );

                drmp3_uninit(&mp3);
                mp3Initialized = false;
            }

            continue;
        }

        size_t bytesToWrite =
            (size_t)framesRead *
            SWITCH_CHANNELS *
            sizeof(s16);

        memset(
            &audioBuffer,
            0,
            sizeof(audioBuffer)
        );

        audioBuffer.next = NULL;
        audioBuffer.buffer = (u8*)pcmBuffer;
        audioBuffer.buffer_size = bytesToWrite;
        audioBuffer.data_size = bytesToWrite;
        audioBuffer.data_offset = 0;

        /*
         * Enviar PCM a la salida de audio.
         */
        Result appendRc =
            audoutAppendAudioOutBuffer(
                &audioBuffer
            );

        if (R_FAILED(appendRc))
        {
            printf(
                "ERROR audoutAppendAudioOutBuffer: 0x%08X\n",
                appendRc
            );

            svcSleepThread(10000000ULL);
            continue;
        }

        /*
         * Esperar a que termine el buffer.
         */
        AudioOutBuffer* releasedBuffer = NULL;
        u32 releasedCount = 0;

        Result waitRc =
            audoutWaitPlayFinish(
                &releasedBuffer,
                &releasedCount,
                1000000000ULL
            );

        if (R_FAILED(waitRc))
        {
            printf(
                "ERROR audoutWaitPlayFinish: 0x%08X\n",
                waitRc
            );
        }
    }

    /*
     * Limpieza.
     */
    if (mp3Initialized)
    {
        drmp3_uninit(&mp3);
    }

    free(pcmBuffer);

    audoutExit();
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    printf("=================================\n");
    printf("       MI SYS PLAY - START\n");
    printf("=================================\n");

    /*
     * Inicializar servicios.
     */
    Result rc;

    rc = smInitialize();

    if (R_FAILED(rc))
    {
        printf(
            "ERROR smInitialize: 0x%08X\n",
            rc
        );

        return 1;
    }

    rc = fsInitialize();

    if (R_FAILED(rc))
    {
        printf(
            "ERROR fsInitialize: 0x%08X\n",
            rc
        );

        smExit();

        return 1;
    }

    rc = pdmInitialize();

    if (R_FAILED(rc))
    {
        printf(
            "ERROR pdmInitialize: 0x%08X\n",
            rc
        );

        fsExit();
        smExit();

        return 1;
    }

    mutexInit(&g_titleMutex);

    /*
     * Thread de audio.
     */
    Thread audioThread;

    rc = threadCreate(
        &audioThread,
        audioThreadFunc,
        NULL,
        NULL,
        0x10000,
        0x2C,
        -2
    );

    if (R_FAILED(rc))
    {
        printf(
            "ERROR threadCreate: 0x%08X\n",
            rc
        );

        pdmExit();
        fsExit();
        smExit();

        return 1;
    }

    rc = threadStart(&audioThread);

    if (R_FAILED(rc))
    {
        printf(
            "ERROR threadStart: 0x%08X\n",
            rc
        );

        threadClose(&audioThread);

        pdmExit();
        fsExit();
        smExit();

        return 1;
    }

    /*
     * Hilo principal.
     *
     * Comprobamos el Title ID cada 250 ms.
     */
    while (g_isRunning)
    {
        u64 titleId =
            getActiveTitleId();

        mutexLock(&g_titleMutex);

        g_currentTitleId = titleId;

        mutexUnlock(&g_titleMutex);

        svcSleepThread(250000000ULL);
    }

    /*
     * Esperar al hilo de audio.
     */
    threadWaitForExit(&audioThread);
    threadClose(&audioThread);

    mutexClose(&g_titleMutex);

    pdmExit();
    fsExit();
    smExit();

    return 0;
}

Y una cosa importante

He cambiado la ruta de los juegos a:

sdmc:/music/010000XXXXXXXXXX.mp3

Es decir, 16 dígitos hexadecimales en mayúsculas.

Tu SD debería quedar así:

sdmc:/
└── music/
    ├── menu.mp3
    ├── default.mp3
    ├── 0100000000010000.mp3
    ├── 01006BB00C6F0000.mp3
    └── 0100XXXXXXXXXXXX.mp3

Pero antes de seguir con el empaquetado NSP, hay una cuestión que quiero evitarte: la API de "audout" y algunas APIs PDM pueden variar según la versión de libnx que esté usando el contenedor de GitHub. Por eso, si este "main.cpp" te da un error de compilación, no cambies cosas al azar: pásame el error y ajustamos exactamente la API que está rechazando tu versión de devkitPro.
