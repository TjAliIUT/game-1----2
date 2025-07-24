#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "background.h"
#include "player.h"
#include <math.h>

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

#define FIELD_W 1280
#define FIELD_H 720
#define HOLD_ZONE_W 100   // (change if your yellow area is a different width)

#define LEFT_GOAL_X 18
#define LEFT_GOAL_Y 273
#define LEFT_GOAL_W 68
#define LEFT_GOAL_H 175

#define RIGHT_GOAL_X 1186
#define RIGHT_GOAL_Y 273
#define RIGHT_GOAL_W 69
#define RIGHT_GOAL_H 175

#define SPOT_RADIUS 32

// const SDL_Color BROWN = {90, 50, 20, 255};
// const SDL_Color RED   = {220, 30, 30, 255};
// const SDL_Color BLUE  = {30, 80, 220, 255};





//dynamic parts
#define NUM_ROWS 5
#define NUM_COLS 3
#define NUM_DISKS 5
#define NUM_CELLS (NUM_ROWS * NUM_COLS)

// Left grid rectangle (adjusted for proper positioning)  
#define LEFT_GRID_LEFT   235  
#define LEFT_GRID_TOP    135  
#define LEFT_GRID_RIGHT  535   // Reduced width  
#define LEFT_GRID_BOTTOM 585   // Reduced height  
  
// Right grid rectangle (adjusted for proper positioning)  
#define RIGHT_GRID_LEFT   745   // Changed from 1155  
#define RIGHT_GRID_TOP    135  
#define RIGHT_GRID_RIGHT  1045  // Changed from 1655  
#define RIGHT_GRID_BOTTOM 585   // Changed from 935  

typedef struct {
    int x, y;
    int occupied_by;
} GridCell;


/* index of the currently selected disk on each side (–1 = no selection) */
int selected_left_disk  = -1;
int selected_right_disk = -1;

typedef struct {
    int start_x, start_y;   /* initial setup position */
    int x, y;               /* live position while dragging */
    int is_dragged;         /* 1 = under the mouse */
    int placed_cell;        /* –1 = not in grid */
    int selected;           /* 1 = this disk glows */
    SDL_Color color;
} PlayerDisk;


// Colors
const SDL_Color brown = {90, 50, 20, 255};
const SDL_Color red   = {220, 30, 30, 255};
const SDL_Color blue  = {30, 80, 220, 255};

GridCell left_grid[NUM_CELLS];
GridCell right_grid[NUM_CELLS];
PlayerDisk left_disks[NUM_DISKS];
PlayerDisk right_disks[NUM_DISKS];




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

//solid color helper function
void draw_filled_circle(SDL_Renderer *ren, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w*w + h*h <= radius*radius)
                SDL_RenderDrawPoint(ren, cx + w, cy + h);
        }
    }
}


// initial draw and placement
void render_player_zones_and_spots(SDL_Renderer *ren) {
    //printf("SPOT_RADIUS = %d\n", SPOT_RADIUS);
    const int spot_radius = 40;
    const float extra_gap_factor = 0.2f; // 20% of radius
    const int n_top = 2, n_bot = 3;

    // Colors
    SDL_Color brown = {90, 50, 20, 255};
    SDL_Color red   = {220, 30, 30, 255};
    SDL_Color blue  = {30, 80, 220, 255};

    // Draw goal rectangles (filled black)
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_Rect left_goal  = {LEFT_GOAL_X, LEFT_GOAL_Y, LEFT_GOAL_W, LEFT_GOAL_H};
    SDL_Rect right_goal = {RIGHT_GOAL_X, RIGHT_GOAL_Y, RIGHT_GOAL_W, RIGHT_GOAL_H};
    SDL_RenderFillRect(ren, &left_goal);
    SDL_RenderFillRect(ren, &right_goal);

    // --- Spot X positions (center of yellow area at each side) ---
    int left_spot_x  = LEFT_GOAL_X + LEFT_GOAL_W / 2 - (int)(0.080f * HOLD_ZONE_W);   // 10% more left
    int right_spot_x = RIGHT_GOAL_X + RIGHT_GOAL_W / 2 + (int)(0.080f * HOLD_ZONE_W); // 10% more right


    // --- Top 2 spots ---
    int top_start = spot_radius;
    int top_end = LEFT_GOAL_Y - spot_radius;
    int top_space = top_end - top_start;
    int left_spot_y[5], right_spot_y[5];

    for (int i = 0; i < n_top; ++i) {
        left_spot_y[i]  = top_start + top_space * (2 * i + 1) / (2 * n_top);
        right_spot_y[i] = left_spot_y[i];
    }

    // --- Bottom 3 spots with extra gap ---
    float gap = 2 * spot_radius * (1 + extra_gap_factor);
    int bot_area_start = LEFT_GOAL_Y + LEFT_GOAL_H + spot_radius;
    int bot_area_end = FIELD_H - spot_radius;
    float bot_area_space = bot_area_end - bot_area_start;
    float base_y = bot_area_start + (bot_area_space - (gap * (n_bot-1))) / 2.0f;

    for (int i = 0; i < n_bot; ++i) {
        left_spot_y[n_top + i]  = (int)(base_y + i * gap);
        right_spot_y[n_top + i] = left_spot_y[n_top + i];
    }

    // --- Draw the brown spots and colored circles ---
    for (int i = 0; i < 5; ++i) {
        // Draw brown spot (background)
        draw_filled_circle(ren, left_spot_x,  left_spot_y[i], SPOT_RADIUS, brown);
        // Draw colored circle (same radius, on top)
        draw_filled_circle(ren, left_spot_x,  left_spot_y[i], SPOT_RADIUS, red);

        draw_filled_circle(ren, right_spot_x, right_spot_y[i], SPOT_RADIUS, brown);
        draw_filled_circle(ren, right_spot_x, right_spot_y[i], SPOT_RADIUS, blue);
    }

}



// ---------- 1. fix grid rendering – top-left corner ----------
void render_left_grid_and_disks(SDL_Renderer *ren)
{
    int cell_w = (LEFT_GRID_RIGHT - LEFT_GRID_LEFT) / NUM_COLS;
    int cell_h = (LEFT_GRID_BOTTOM - LEFT_GRID_TOP) / NUM_ROWS;

    /* draw the grid */
    for (int i = 0; i < NUM_CELLS; ++i) {
        SDL_Rect cell = { left_grid[i].x, left_grid[i].y, cell_w, cell_h };
        SDL_SetRenderDrawColor(ren, 180, 180, 180, 90);
        SDL_RenderDrawRect(ren, &cell);
    }

    /* draw the disks */
    for (int i = 0; i < NUM_DISKS; ++i) {
        int cx, cy;

        if (left_disks[i].is_dragged) {
            cx = left_disks[i].x;
            cy = left_disks[i].y;
        } else if (left_disks[i].placed_cell != -1) {
            cx = left_grid[left_disks[i].placed_cell].x + cell_w / 2;
            cy = left_grid[left_disks[i].placed_cell].y + cell_h / 2;
        } else {
            cx = left_disks[i].start_x;
            cy = left_disks[i].start_y;
        }

        /* glow */
        if (left_disks[i].selected) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_Color glow = {255, 255, 0, 150};                /* soft yellow */
            draw_filled_circle(ren, cx, cy, SPOT_RADIUS + 6, glow);
        }

        /* disk body */
        draw_filled_circle(ren, cx, cy, SPOT_RADIUS, left_disks[i].color);
    }
}




void render_right_grid_and_disks(SDL_Renderer *ren)
{
    int cell_w = (RIGHT_GRID_RIGHT - RIGHT_GRID_LEFT) / NUM_COLS;
    int cell_h = (RIGHT_GRID_BOTTOM - RIGHT_GRID_TOP) / NUM_ROWS;

    for (int i = 0; i < NUM_CELLS; ++i) {
        SDL_Rect cell = { right_grid[i].x, right_grid[i].y, cell_w, cell_h };
        SDL_SetRenderDrawColor(ren, 180, 180, 180, 90);
        SDL_RenderDrawRect(ren, &cell);
    }

    for (int i = 0; i < NUM_DISKS; ++i) {
        int cx, cy;

        if (right_disks[i].is_dragged) {
            cx = right_disks[i].x;
            cy = right_disks[i].y;
        } else if (right_disks[i].placed_cell != -1) {
            cx = right_grid[right_disks[i].placed_cell].x + cell_w / 2;
            cy = right_grid[right_disks[i].placed_cell].y + cell_h / 2;
        } else {
            cx = right_disks[i].start_x;
            cy = right_disks[i].start_y;
        }

        if (right_disks[i].selected) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_Color glow = {255, 255, 0, 150};
            draw_filled_circle(ren, cx, cy, SPOT_RADIUS + 6, glow);
        }

        draw_filled_circle(ren, cx, cy, SPOT_RADIUS, right_disks[i].color);
    }
}



// Handles mouse drag-and-drop for disks (only when grid is active)
// ---------- 2. fix disk drop logic ----------
/* ---------------------------------------------------------------------------
   Handle dragging and dropping disks between grid cells (or back to setup)
   --------------------------------------------------------------------------- */
void handle_disk_drag_events(SDL_Event *e, int show_default_setup)
{
    static int drag_left_disk_idx  = -1;
    static int drag_right_disk_idx = -1;
    static int mouse_offset_x = 0, mouse_offset_y = 0;

    int cell_w_left  = (LEFT_GRID_RIGHT  - LEFT_GRID_LEFT)  / NUM_COLS;
    int cell_h_left  = (LEFT_GRID_BOTTOM - LEFT_GRID_TOP)   / NUM_ROWS;
    int cell_w_right = (RIGHT_GRID_RIGHT - RIGHT_GRID_LEFT) / NUM_COLS;
    int cell_h_right = (RIGHT_GRID_BOTTOM- RIGHT_GRID_TOP)  / NUM_ROWS;

    /* --------------------------------------------------------------------
       1) MOUSE DOWN  – handle selection, and (if allowed) start dragging
       -------------------------------------------------------------------- */
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int mx = e->button.x, my = e->button.y;
        int hit_left = -1, hit_right = -1;

        /* test all left disks */
        for (int i = 0; i < NUM_DISKS; ++i) {
            int cx, cy;
            if (left_disks[i].placed_cell != -1) {
                cx = left_grid[left_disks[i].placed_cell].x + cell_w_left / 2;
                cy = left_grid[left_disks[i].placed_cell].y + cell_h_left / 2;
            } else {
                cx = left_disks[i].start_x;
                cy = left_disks[i].start_y;
            }
            int dx = mx - cx, dy = my - cy;
            if (dx*dx + dy*dy <= SPOT_RADIUS*SPOT_RADIUS) { hit_left = i; break; }
        }

        /* test right disks only if nothing hit on left */
        if (hit_left == -1) {
            for (int i = 0; i < NUM_DISKS; ++i) {
                int cx, cy;
                if (right_disks[i].placed_cell != -1) {
                    cx = right_grid[right_disks[i].placed_cell].x + cell_w_right / 2;
                    cy = right_grid[right_disks[i].placed_cell].y + cell_h_right / 2;
                } else {
                    cx = right_disks[i].start_x;
                    cy = right_disks[i].start_y;
                }
                int dx = mx - cx, dy = my - cy;
                if (dx*dx + dy*dy <= SPOT_RADIUS*SPOT_RADIUS) { hit_right = i; break; }
            }
        }

        /* ---------- update selection ---------- */
        if (hit_left != -1) {
            selected_left_disk  = hit_left;
            selected_right_disk = -1;
        } else if (hit_right != -1) {
            selected_right_disk = hit_right;
            selected_left_disk  = -1;
        } else {
            /* clicked empty space → clear selection */
            selected_left_disk  = -1;
            selected_right_disk = -1;
        }
        /* flag array entries */
        for (int i = 0; i < NUM_DISKS; ++i) {
            left_disks[i].selected  = (i == selected_left_disk);
            right_disks[i].selected = (i == selected_right_disk);
        }

        /* ---------- optional drag start ---------- */
        int can_drag = (show_default_setup == 0);
        if (can_drag && hit_left != -1) {
            /* free old cell */
            if (left_disks[hit_left].placed_cell != -1) {
                left_grid[left_disks[hit_left].placed_cell].occupied_by = -1;
                left_disks[hit_left].placed_cell = -1;
            }
            drag_left_disk_idx         = hit_left;
            left_disks[hit_left].is_dragged = 1;
            mouse_offset_x = mx - left_disks[hit_left].x;
            mouse_offset_y = my - left_disks[hit_left].y;
        } else if (can_drag && hit_right != -1) {
            if (right_disks[hit_right].placed_cell != -1) {
                right_grid[right_disks[hit_right].placed_cell].occupied_by = -1;
                right_disks[hit_right].placed_cell = -1;
            }
            drag_right_disk_idx         = hit_right;
            right_disks[hit_right].is_dragged = 1;
            mouse_offset_x = mx - right_disks[hit_right].x;
            mouse_offset_y = my - right_disks[hit_right].y;
        }
    }

    /* --------------------------------------------------------------------
       2) MOUSE MOTION  – only while dragging is allowed
       -------------------------------------------------------------------- */
    if (show_default_setup == 0 && e->type == SDL_MOUSEMOTION) {
        if (drag_left_disk_idx != -1) {
            left_disks[drag_left_disk_idx].x = e->motion.x - mouse_offset_x;
            left_disks[drag_left_disk_idx].y = e->motion.y - mouse_offset_y;
        }
        if (drag_right_disk_idx != -1) {
            right_disks[drag_right_disk_idx].x = e->motion.x - mouse_offset_x;
            right_disks[drag_right_disk_idx].y = e->motion.y - mouse_offset_y;
        }
    }

    /* --------------------------------------------------------------------
       3) MOUSE UP  – drop (only matters if we were dragging)
       -------------------------------------------------------------------- */
    if (show_default_setup == 0 &&
        e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {

        int mx = e->button.x, my = e->button.y;

        /* drop left disk */
        if (drag_left_disk_idx != -1) {
            int idx = drag_left_disk_idx;  drag_left_disk_idx = -1;
            left_disks[idx].is_dragged = 0;

            int col = (mx - LEFT_GRID_LEFT) / cell_w_left;
            int row = (my - LEFT_GRID_TOP ) / cell_h_left;
            if ( col >= 0 && col < NUM_COLS &&
                 row >= 0 && row < NUM_ROWS &&
                 mx >= LEFT_GRID_LEFT && mx <= LEFT_GRID_RIGHT &&
                 my >= LEFT_GRID_TOP  && my <= LEFT_GRID_BOTTOM ) {

                int target = row * NUM_COLS + col;
                if (left_grid[target].occupied_by == -1) {
                    left_disks[idx].placed_cell   = target;
                    left_grid[target].occupied_by = idx;
                } else {
                    left_disks[idx].x = left_disks[idx].start_x;
                    left_disks[idx].y = left_disks[idx].start_y;
                }
            } else {
                left_disks[idx].x = left_disks[idx].start_x;
                left_disks[idx].y = left_disks[idx].start_y;
            }
        }

        /* drop right disk */
        if (drag_right_disk_idx != -1) {
            int idx = drag_right_disk_idx; drag_right_disk_idx = -1;
            right_disks[idx].is_dragged = 0;

            int col = (mx - RIGHT_GRID_LEFT) / cell_w_right;
            int row = (my - RIGHT_GRID_TOP ) / cell_h_right;
            if ( col >= 0 && col < NUM_COLS &&
                 row >= 0 && row < NUM_ROWS &&
                 mx >= RIGHT_GRID_LEFT && mx <= RIGHT_GRID_RIGHT &&
                 my >= RIGHT_GRID_TOP  && my <= RIGHT_GRID_BOTTOM ) {

                int target = row * NUM_COLS + col;
                if (right_grid[target].occupied_by == -1) {
                    right_disks[idx].placed_cell   = target;
                    right_grid[target].occupied_by = idx;
                } else {
                    right_disks[idx].x = right_disks[idx].start_x;
                    right_disks[idx].y = right_disks[idx].start_y;
                }
            } else {
                right_disks[idx].x = right_disks[idx].start_x;
                right_disks[idx].y = right_disks[idx].start_y;
            }
        }
    }
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

    Background *game_bg =  create_background(ren, "assets/textures/game-bg-u.png");
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







    // ---- NEW: ADD THIS FLAG HERE ----
    int show_default_setup = 1;   // 1 = show static setup, 0 = enable interactive placement

   // ---- Grid Setup ----
         /* ----------  ABSOLUTE-PIXEL GRID + DISK SETUP  ---------- */
    const int L_CELL_W = (LEFT_GRID_RIGHT  - LEFT_GRID_LEFT) / NUM_COLS;
    const int L_CELL_H = (LEFT_GRID_BOTTOM - LEFT_GRID_TOP)  / NUM_ROWS;
    const int R_CELL_W = (RIGHT_GRID_RIGHT  - RIGHT_GRID_LEFT) / NUM_COLS;
    const int R_CELL_H = (RIGHT_GRID_BOTTOM - RIGHT_GRID_TOP)  / NUM_ROWS;

    /* Grid cells */
    for (int row = 0; row < NUM_ROWS; ++row) {
        for (int col = 0; col < NUM_COLS; ++col) {
            int idx = row * NUM_COLS + col;
            left_grid[idx].x  = LEFT_GRID_LEFT + col * L_CELL_W;
            left_grid[idx].y  = LEFT_GRID_TOP  + row * L_CELL_H;
            left_grid[idx].occupied_by = -1;

            right_grid[idx].x = RIGHT_GRID_LEFT + col * R_CELL_W;
            right_grid[idx].y = RIGHT_GRID_TOP  + row * R_CELL_H;
            right_grid[idx].occupied_by = -1;
        }
    }

    // /* Disk start positions (setup bars) */
    /* Disk start positions (setup bars) - matching render_player_zones_and_spots */  
    const int spot_radius = 40;  
    const float extra_gap_factor = 0.2f;  
    const int n_top = 2, n_bot = 3;  
    
    // Calculate positions exactly as in render_player_zones_and_spots  
    int left_spot_x  = LEFT_GOAL_X + LEFT_GOAL_W / 2 - (int)(0.080f * HOLD_ZONE_W);  
    int right_spot_x = RIGHT_GOAL_X + RIGHT_GOAL_W / 2 + (int)(0.080f * HOLD_ZONE_W);  
    
    // Top 2 spots  
    int top_start = spot_radius;  
    int top_end = LEFT_GOAL_Y - spot_radius;  
    int top_space = top_end - top_start;  
    int setup_y[5];  
    
    for (int i = 0; i < n_top; ++i) {  
        setup_y[i] = top_start + top_space * (2 * i + 1) / (2 * n_top);  
    }  
    
    // Bottom 3 spots  
    float gap = 2 * spot_radius * (1 + extra_gap_factor);  
    int bot_area_start = LEFT_GOAL_Y + LEFT_GOAL_H + spot_radius;  
    int bot_area_end = FIELD_H - spot_radius;  
    float bot_area_space = bot_area_end - bot_area_start;  
    float base_y = bot_area_start + (bot_area_space - (gap * (n_bot-1))) / 2.0f;  
    
    for (int i = 0; i < n_bot; ++i) {  
        setup_y[n_top + i] = (int)(base_y + i * gap);  
    }  
    
    // Initialize disks with these positions  
    for (int i = 0; i < NUM_DISKS; ++i) {  
        left_disks[i].start_x = left_disks[i].x = left_spot_x;  
        left_disks[i].start_y = left_disks[i].y = setup_y[i];  
        left_disks[i].is_dragged = 0;  
        left_disks[i].placed_cell = -1;  
        left_disks[i].color = red;  
    
        right_disks[i].start_x = right_disks[i].x = right_spot_x;  
        right_disks[i].start_y = right_disks[i].y = setup_y[i];  
        right_disks[i].is_dragged = 0;  
        right_disks[i].placed_cell = -1;  
        right_disks[i].color = blue;  
    }  





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




            if (state == STATE_GAME_RUNNING) handle_disk_drag_events(&e, show_default_setup);




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






            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE && state == STATE_GAME_RUNNING) {
                show_default_setup = 0;
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
                if (show_default_setup) 
                {
                    render_player_zones_and_spots(ren);   // <--- Your default "static" positions
                }
                else
                {
                    render_left_grid_and_disks(ren);      // <--- Moveable disks in grid
                    render_right_grid_and_disks(ren);
                }
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