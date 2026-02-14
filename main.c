#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <math.h>

/* FFmpeg */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

/* =======================
   Button callbacks
   ======================= */
void newGame() {
    printf(">> NEW GAME STARTED\n");
}

void openSystem() {
    printf(">> SYSTEM MENU OPENED\n");
}

/* =======================
   Helper
   ======================= */
int mouseInside(SDL_Rect r, int mx, int my) {
    return (mx >= r.x && mx <= r.x + r.w &&
            my >= r.y && my <= r.y + r.h);
}

int main(int argc, char *argv[]) {

    /* =======================
       SDL INIT
       ======================= */
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow(
        "TUNESCA",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 800,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    /* =======================
       VIDEO INIT (FFmpeg)
       ======================= */
    AVFormatContext *fmt = NULL;
    avformat_open_input(&fmt, "back.mp4", NULL, NULL);
    avformat_find_stream_info(fmt, NULL);

    int videoStream = -1;
    for (int i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    AVCodec *codec =
        avcodec_find_decoder(fmt->streams[videoStream]->codecpar->codec_id);

    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(
        codecCtx, fmt->streams[videoStream]->codecpar);
    avcodec_open2(codecCtx, codec, NULL);

    AVFrame *frame = av_frame_alloc();
    AVFrame *frameRGB = av_frame_alloc();
    AVPacket packet;

    struct SwsContext *sws = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL
    );

    int bufSize = av_image_get_buffer_size(
        AV_PIX_FMT_RGB24,
        codecCtx->width, codecCtx->height, 1);

    uint8_t *buffer = (uint8_t*)av_malloc(bufSize);

    av_image_fill_arrays(
        frameRGB->data, frameRGB->linesize,
        buffer, AV_PIX_FMT_RGB24,
        codecCtx->width, codecCtx->height, 1
    );

    SDL_Texture *videoTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        codecCtx->width,
        codecCtx->height
    );

    /* =======================
       AUDIO
       ======================= */
    Mix_Music *music = Mix_LoadMUS("music.mp3");
    if (music) Mix_PlayMusic(music, -1);

    /* =======================
       FONT
       ======================= */
    TTF_Font *font = TTF_OpenFont("arial.ttf", 34);

    SDL_Color gold = {212, 175, 55, 255};
    SDL_Color gray = {170, 170, 170, 255};

    /* =======================
       BUTTONS
       ======================= */
    SDL_Rect btnNewGame = {490, 440, 300, 70};
    SDL_Rect btnSystem  = {490, 530, 300, 70};

    SDL_Texture *txtNewGame = NULL;
    SDL_Texture *txtSystem  = NULL;
    SDL_Rect txtNewGameRect, txtSystemRect;

    SDL_Cursor *hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    SDL_Cursor *arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

    int running = 1;
    SDL_Event e;

    /* =======================
       MAIN LOOP
       ======================= */
    while (running) {

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        int hoverNew = mouseInside(btnNewGame, mx, my);
        int hoverSys = mouseInside(btnSystem, mx, my);

        SDL_SetCursor((hoverNew || hoverSys) ? hand : arrow);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = 0;

            if (e.type == SDL_MOUSEBUTTONDOWN &&
                e.button.button == SDL_BUTTON_LEFT) {
                if (hoverNew) newGame();
                if (hoverSys) openSystem();
            }
        }

        /* =======================
           VIDEO FRAME
           ======================= */
        if (av_read_frame(fmt, &packet) >= 0) {

            if (packet.stream_index == videoStream) {

                avcodec_send_packet(codecCtx, &packet);

                if (avcodec_receive_frame(codecCtx, frame) == 0) {

                    sws_scale(
                        sws,
                        (const uint8_t * const*)frame->data,
                        frame->linesize,
                        0,
                        codecCtx->height,
                        frameRGB->data,
                        frameRGB->linesize
                    );

                    SDL_UpdateTexture(
                        videoTexture,
                        NULL,
                        frameRGB->data[0],
                        frameRGB->linesize[0]
                    );
                }
            }

            av_packet_unref(&packet);
        } else {
            av_seek_frame(fmt, videoStream, 0, AVSEEK_FLAG_BACKWARD);
        }

        /* =======================
           RENDER
           ======================= */
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, videoTexture, NULL, NULL);

        Uint32 t = SDL_GetTicks();
        int glow = 150 + (int)((sin(t * 0.004f) + 1) * 52);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 15, 15, 15, 190);
        SDL_RenderFillRect(renderer, &btnNewGame);
        SDL_RenderFillRect(renderer, &btnSystem);

        if (hoverNew) {
            SDL_SetRenderDrawColor(renderer, glow, glow-40, 0, 255);
            SDL_RenderDrawRect(renderer, &btnNewGame);
        }
        if (hoverSys) {
            SDL_SetRenderDrawColor(renderer, glow, glow-40, 0, 255);
            SDL_RenderDrawRect(renderer, &btnSystem);
        }

        SDL_Color cNew = hoverNew ? gold : gray;
        SDL_Color cSys = hoverSys ? gold : gray;

        SDL_Surface *s1 = TTF_RenderText_Blended(font, "NEW GAME", cNew);
        txtNewGame = SDL_CreateTextureFromSurface(renderer, s1);
        txtNewGameRect = (SDL_Rect){
            btnNewGame.x + (btnNewGame.w - s1->w)/2,
            btnNewGame.y + (btnNewGame.h - s1->h)/2,
            s1->w, s1->h
        };
        SDL_FreeSurface(s1);

        SDL_Surface *s2 = TTF_RenderText_Blended(font, "SYSTEM", cSys);
        txtSystem = SDL_CreateTextureFromSurface(renderer, s2);
        txtSystemRect = (SDL_Rect){
            btnSystem.x + (btnSystem.w - s2->w)/2,
            btnSystem.y + (btnSystem.h - s2->h)/2,
            s2->w, s2->h
        };
        SDL_FreeSurface(s2);

        SDL_RenderCopy(renderer, txtNewGame, NULL, &txtNewGameRect);
        SDL_RenderCopy(renderer, txtSystem, NULL, &txtSystemRect);

        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(txtNewGame);
        SDL_DestroyTexture(txtSystem);
    }

    /* =======================
       CLEANUP
       ======================= */
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmt);
    sws_freeContext(sws);

    SDL_DestroyTexture(videoTexture);
    Mix_FreeMusic(music);

    SDL_FreeCursor(hand);
    SDL_FreeCursor(arrow);

    TTF_CloseFont(font);
    TTF_Quit();
    Mix_CloseAudio();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

