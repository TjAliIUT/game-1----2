#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "background.h"
#include "player.h"

typedef enum {
    STATE_STARTUP_ANIMATION,
    STATE_MAIN_MENU,
    STATE_MODE_SELECT,
    STATE_GAME_RUNNING,
    STATE_PAUSE_MENU,
    STATE_EXIT
} GameState;

typedef struct {
    SDL_Rect rect;
    const char *label;
    int hovered;
} Button;

void render_button(SDL_Renderer *ren, TTF_Font *font, Button *btn) {
    SDL_Color color = btn->hovered ? (SDL_Color){200,200,255,255} : (SDL_Color){255,255,255,255};
    SDL_SetRenderDrawColor(ren, btn->hovered ? 70 : 50, 50, 150, 255);
    SDL_RenderFillRect(ren, &btn->rect);
    // Draw border
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderDrawRect(ren, &btn->rect);

    // Render label
    SDL_Surface *surf = TTF_RenderText_Blended(font, btn->label, color);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw = surf->w, th = surf->h;
    SDL_Rect dst = { btn->rect.x + (btn->rect.w-tw)/2, btn->rect.y + (btn->rect.h-th)/2, tw, th };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void render_main_menu(SDL_Renderer *ren, TTF_Font *font, Button *play_btn, Button *quit_btn) {
    // Draw heading (centered)
    SDL_Color white = {255,255,255,255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Main Menu", white);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw = surf->w, th = surf->h;
    SDL_Rect dst = { (1280-tw)/2, 120, tw, th };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);

    // Draw buttons
    render_button(ren, font, play_btn);
    render_button(ren, font, quit_btn);
}
void render_mode_menu(SDL_Renderer *ren, TTF_Font *font, Button *single_btn, Button *multi_btn, Button *back_btn) {
    // Draw heading
    SDL_Color white = {255,255,255,255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Select Mode", white);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw = surf->w, th = surf->h;
    SDL_Rect dst = { (1280-tw)/2, 120, tw, th };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);

    // Draw buttons
    render_button(ren, font, single_btn);
    render_button(ren, font, multi_btn);
    render_button(ren, font, back_btn);
}
void render_pause_menu(SDL_Renderer *ren, TTF_Font *font, Button *resume_btn, Button *restart_btn, Button *quitgame_btn) {
    // Dim the background
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    SDL_Rect dim = { 0, 0, 1280, 720 };
    SDL_RenderFillRect(ren, &dim);

    // Heading
    SDL_Color white = {255,255,255,255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Paused", white);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw = surf->w, th = surf->h;
    SDL_Rect dst = { (1280-tw)/2, 120, tw, th };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);

    render_button(ren, font, resume_btn);
    render_button(ren, font, restart_btn);
    render_button(ren, font, quitgame_btn);
}

void render_fade(SDL_Renderer *ren, int alpha) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, alpha);
    SDL_Rect screen = {0, 0, 1280, 720};
    SDL_RenderFillRect(ren, &screen);
}

int point_in_rect(int x, int y, SDL_Rect *rect) {
    return (x >= rect->x && x <= rect->x+rect->w &&
            y >= rect->y && y <= rect->y+rect->h);
}


int main(int argc, char *argv[])
{
    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        fprintf(stderr, "Mix_OpenAudio Error: %s\n", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() < 0)
    {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create window
    SDL_Window *win = SDL_CreateWindow(
        "SMACK!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN);
    if (!win)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren)
    {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create background
    Background *bg = create_background(ren, "assets/textures/22.png");
    if (!bg)
    {
        //destroy_background(bg);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Background *game_bg =  create_background(ren, "assets/textures/game-bg.png");
    if (!game_bg) {
        destroy_background(bg);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // --- [Load Font] ---
    TTF_Font *font = TTF_OpenFont("assets/fonts/OpenSans-Bold.ttf", 48); // Use a path to your .ttf font!
    if (!font) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        // Cleanup here if needed
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // --- [Button Declarations] ---

    // main menu buttons
    Button play_btn = { { (1280-220)/2, 280, 220, 70 }, "Play", 0 };
    Button quit_btn = { { (1280-220)/2, 380, 220, 70 }, "Quit", 0 };

    // Mode menu buttons (BIGGER & CENTERED)
    Button single_btn = { { (1280-350)/2, 240, 350, 80 }, "Single Player", 0 };
    Button multi_btn  = { { (1280-350)/2, 350, 350, 80 }, "Multiplayer", 0 };
    Button back_btn   = { { (1280-180)/2, 470, 180, 55 }, "Back", 0 };

    // pause menu buttons
    Button resume_btn = { { (1280-220)/2, 250, 220, 70 }, "Resume", 0 };
    Button restart_btn = { { (1280-220)/2, 350, 220, 70 }, "Restart", 0 };
    Button quitgame_btn = { { (1280-220)/2, 450, 220, 70 }, "Quit", 0 };

    // --- [Fade Speed & Next State] ---
    float fade_speed = 300.0f; // You can adjust for faster/slower fade
    GameState next_state = STATE_MAIN_MENU; // initial dummy value


    // For frame timing
    Uint32 last_time = SDL_GetTicks();
    SDL_Event e;
    int running = 1;

        // --- [Game State Vars: before main loop] ---
    GameState state = STATE_STARTUP_ANIMATION;
    int fade_alpha = 255;         // for fade effects
    int fading_in = 1, fading_out = 0;
    Uint32 state_timer = 0;       // timer for animation state


    // Main loop
    while (running)
    {
        Uint32 current_time = SDL_GetTicks();
        Uint32 delta_time = current_time - last_time;
        last_time = current_time;

        // Handle events
        while (SDL_PollEvent(&e))
        {   
            // If user clicks red cross (window close)
            if (e.type == SDL_QUIT)
            {
                running = 0;
            }

            //use of esc button to go back or quit for convinience
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE && !fading_in && !fading_out) 
            {
                if (state == STATE_MAIN_MENU) {
                    fading_out = 1;
                    next_state = STATE_EXIT;
                } 
                else if (state == STATE_MODE_SELECT) {
                    fading_out = 1;
                    next_state = STATE_MAIN_MENU;
                }
                else if (state == STATE_GAME_RUNNING) {
                    state = STATE_PAUSE_MENU;
                    // Reset fade flags so pause menu is instantly visible and responsive
                    fade_alpha = 0;
                    fading_in = 0;
                    fading_out = 0;
                }
                else if (state == STATE_PAUSE_MENU) {
                    state = STATE_GAME_RUNNING;
                    fade_alpha = 0;
                    fading_in = 0;
                    fading_out = 0;
                }
            }



            //Checks if the left mouse button was pressed.
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) 
            {
                //retrieves the mouse x and y coordinates (mx, my) where the click happened.
                int mx = e.button.x, my = e.button.y;

                //Checks if you are on the main menu and not currently fading in or out.
                if (state == STATE_MAIN_MENU && !fading_in && !fading_out) 
                {   
                    //If Play button clicked
                    if (point_in_rect(mx, my, &play_btn.rect)) {
                        fading_out = 1;  // Start fade out
                        next_state = STATE_MODE_SELECT;
                    }
                    //If quit button clicked 
                    else if (point_in_rect(mx, my, &quit_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_EXIT;
                    }
                }

                //Checks if you are on the mode select menu (choose single/multi) and not in a transition.
                else if (state == STATE_MODE_SELECT && !fading_in && !fading_out) 
                {   
                    //if Single or Multiplayer button clicked
                    if (point_in_rect(mx, my, &single_btn.rect) ||
                        point_in_rect(mx, my, &multi_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_GAME_RUNNING;
                    }
                    //if back button clicked
                    else if (point_in_rect(mx, my, &back_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_MAIN_MENU;
                    }
                }
                else if (state == STATE_PAUSE_MENU && !fading_in && !fading_out) 
                {
                    if (point_in_rect(mx, my, &resume_btn.rect)) {
                        state = STATE_GAME_RUNNING;
                    }
                    else if (point_in_rect(mx, my, &restart_btn.rect)) {
                        // TODO: Add game state reset logic here
                        state = STATE_GAME_RUNNING;
                    }
                    else if (point_in_rect(mx, my, &quitgame_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_MAIN_MENU;
                    }
                }

            }

        }

        // Render
        SDL_RenderClear(ren);
                                                                                        // render_background(ren, bg);
                                                                                        
        switch (state) {
            case STATE_STARTUP_ANIMATION:
                // --- [Draw Startup Animation: logo/text fade in/out] ---
                // Example: Draw a logo/text in the center
                {
                    SDL_Color white = {255,255,255,255};
                    SDL_Surface *surf = TTF_RenderText_Blended(font, "NO BALL FOOTBALL!", white);
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
                    int tw = surf->w, th = surf->h;
                    SDL_Rect dst = { (1280-tw)/2, (720-th)/2, tw, th };
                    SDL_RenderCopy(ren, tex, NULL, &dst);
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                }

                // Fade in, then after delay, fade out
                if (fading_in) {
                    fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha <= 0) {
                        fade_alpha = 0;
                        fading_in = 0;
                        state_timer = SDL_GetTicks(); // Start delay
                    }
                }
                else if (!fading_out && SDL_GetTicks() - state_timer > 1000) { // 1 sec delay
                    fading_out = 1;
                }
                if (fading_out) {
                    fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha >= 255) {
                        fade_alpha = 255;
                        fading_out = 0;
                        fading_in = 1;
                        state = STATE_MAIN_MENU;
                        fade_alpha = 255;
                    }
                }
                render_fade(ren, fade_alpha);
                break;

            case STATE_MAIN_MENU:
                // --- [Draw Background] ---
                render_background(ren, bg);

                // --- [Draw Main Menu Heading] ---
                SDL_Color white = {255,255,255,255};
                SDL_Surface *surf = TTF_RenderText_Blended(font, "Main Menu", white);
                SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
                int tw = surf->w, th = surf->h;
                SDL_Rect dst = { (1280-tw)/2, 120, tw, th };
                SDL_RenderCopy(ren, tex, NULL, &dst);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);

                // --- [Draw Play & Quit Buttons] ---
                render_button(ren, font, &play_btn);
                render_button(ren, font, &quit_btn);

                // --- [Fade Logic] ---
                if (fading_in) {
                    fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha <= 0) { fade_alpha = 0; fading_in = 0; }
                }
                if (fading_out) 
                {
                    fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha >= 255) {
                        fade_alpha = 255;
                        fading_out = 0;

                        // Only fade in for menus, not for game running
                        if (next_state == STATE_GAME_RUNNING) {
                            fading_in = 0;
                        } else {
                            fading_in = 1;
                            fade_alpha = 255;
                        }
                        state = next_state;
                    }
                }
                render_fade(ren, fade_alpha);
                break;

            case STATE_MODE_SELECT:
                // --- [Draw Background] ---
                render_background(ren, bg);

                // --- [Draw Mode Menu Heading] ---
                {
                    SDL_Color white = {255,255,255,255};
                    SDL_Surface *surf = TTF_RenderText_Blended(font, "Select Mode", white);
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
                    int tw = surf->w, th = surf->h;
                    SDL_Rect dst = { (1280-tw)/2, 120, tw, th };
                    SDL_RenderCopy(ren, tex, NULL, &dst);
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                }

                // --- [Draw Single, Multi, Back Buttons] ---
                render_button(ren, font, &single_btn);
                render_button(ren, font, &multi_btn);
                render_button(ren, font, &back_btn);

                // --- [Fade Logic] ---
                if (fading_in) {
                    fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha <= 0) { fade_alpha = 0; fading_in = 0; }
                }
                if (fading_out) {
                    fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha >= 255) {
                        fade_alpha = 255;
                        fading_out = 0;

                        // Only fade in for menus, not for game running
                        if (next_state == STATE_GAME_RUNNING) {
                            fading_in = 0;
                        } else {
                            fading_in = 1;
                            fade_alpha = 255;
                        }
                        state = next_state;
                    }
                }
                render_fade(ren, fade_alpha);
                break;

            case STATE_GAME_RUNNING:
                render_background(ren, game_bg);
                // --- [Start Your Game] ---
                // Here, remove all menu/UI and run your game logic and rendering.
                // Example: render_background(ren, bg); render_player(ren, ...); etc.
                // (No fade here unless you want a transition out.)
                break;
            case STATE_PAUSE_MENU:
                render_background(ren, game_bg);
                render_pause_menu(ren, font, &resume_btn, &restart_btn, &quitgame_btn);

                // Fade logic for quitting out of pause menu
                if (fading_out) {
                    fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                    if (fade_alpha >= 255) {
                        fade_alpha = 255;
                        fading_out = 0;
                        // Fade in only if returning to a menu, not if going back to game
                        if (next_state == STATE_MAIN_MENU) {
                            fading_in = 1;
                            fade_alpha = 255;
                        } else {
                            fading_in = 0;
                        }
                        state = next_state;
                    }
                }
                render_fade(ren, fade_alpha);
                break;
            case STATE_EXIT:
                running = 0; // End the main loop
                break;
        }

        //render_player(ren, &player);
        SDL_RenderPresent(ren);
    }

    // Cleanup
    destroy_background(bg);
    destroy_background(game_bg);
    //destroy_player(&player);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}