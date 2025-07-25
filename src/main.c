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
    STATE_GAME_RESULT,
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

#define LEFT_GOAL_X 16
#define LEFT_GOAL_Y 273
#define LEFT_GOAL_W 68
#define LEFT_GOAL_H 175

#define RIGHT_GOAL_X 1188
#define RIGHT_GOAL_Y 273
#define RIGHT_GOAL_W 69
#define RIGHT_GOAL_H 175

#define SPOT_RADIUS 32

/* six wall rectangles */
// SDL_Rect walls[6] = {
//     {102,   0,  30, 408},   /* left-upper  vertical   */
//     {102, 662,  30, 410},   /* left-lower  vertical   */
//     {132,   0, 1640, 50},   /* top        horizontal */
//     {132,1022, 1640, 50},   /* bottom     horizontal */
//     {1772,  0,  30, 408},   /* right-upper vertical   */
//     {1772,662,  30, 410}    /* right-lower vertical   */
// };






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

/* remembers where the disk was before the current drag starts */
static int prev_left_cell  = -1;   /* –1 → came from setup bar */
static int prev_right_cell = -1;   /* –1 → came from setup bar */

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


int left_score  = 0;   /* Red  */
int right_score = 0;   /* Blue */

Button rematch_btn     = { { (1210-260)/2, 326, 320, 70 }, "Rematch",     0 };  /* 470-144 */
Button selmode_btn     = { { (1210-260)/2, 416, 320, 70 }, "Select Mode", 0 };  /* 560-144 */
Button quit_btn_result = { { (1210-260)/2, 506, 320, 70 }, "Quit",        0 };  /* 650-144 */

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

void render_main_menu(SDL_Renderer *ren, TTF_Font *font, Button *play_btn, Button *quit_btn_main) {
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
    /* --- draw Play & Quit buttons — dynamically centred --- */
    Button tmp;

    /* Play */
    tmp = *play_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);

    /* Quit */
    tmp = *quit_btn_main;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);
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
    /* --- draw Single-player, Multi-player, Back buttons – centred every frame --- */
    Button tmp;

    /* Single Player */
    tmp = *single_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);

    /* Multiplayer */
    tmp = *multi_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);

    /* Back */
    tmp = *back_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);
}
void render_pause_menu(SDL_Renderer *ren, TTF_Font *font, Button *resume_btn, Button *restart_btn, Button *quitmatch_btn) {
    // Dim the background
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    SDL_Rect dim = { 0, 0, 1280, 720 };
    SDL_RenderFillRect(ren, &dim);

    // Heading
    SDL_Color white = {255,255,255,255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, "Match Paused", white);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);

    /* X-centre stays (1280-tw)/2; new Y-pos = midway between scoreboard and buttons */
    const int scoreboard_y   = 6;    /* where render_scoreboard() puts it   */
    const int scoreboard_h   = 72;   /* height you set in render_scoreboard */
    const int scoreboard_bot = scoreboard_y + scoreboard_h;
    const int first_buttonY  = 250;  /* y-pos of Resume button              */

    int tw = surf->w, th = surf->h;
    SDL_Rect dst = { (1280-tw)/2, scoreboard_bot + (first_buttonY - scoreboard_bot - th) / 2, tw, th };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);

 /* ---- draw the three buttons, x-centred on the fly ---- */
    Button tmp;

    tmp = *resume_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);

    tmp = *restart_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);

    tmp = *quitmatch_btn;
    tmp.rect.x = (1280 - tmp.rect.w) / 2;
    render_button(ren, font, &tmp);
}

static void render_scoreboard(SDL_Renderer *ren, TTF_Font *font,
                              int red_score, int blue_score);

static void render_game_result(SDL_Renderer *ren, TTF_Font *font,
                               int l_score, int r_score)
{
    render_scoreboard(ren, font, l_score, r_score);

    /* --- top 60 % area: winner / loser text + scoreboard --- */
    const SDL_Color black = {0, 0, 0, 255};
    char title[32];

    if (l_score > r_score)       strcpy(title, "Red Wins!");
    else if (r_score > l_score)  strcpy(title, "Blue Wins!");
    else                         strcpy(title, "Match Drawn");

    SDL_Surface *surf = TTF_RenderText_Blended(font, title, black);
    SDL_Texture *tex  = SDL_CreateTextureFromSurface(ren, surf);

    const int scoreboard_y   = 6;    /* y where render_scoreboard() draws */
    const int scoreboard_h   = 72;   /* its height                       */
    const int scoreboard_bot = scoreboard_y + scoreboard_h;
    const int first_buttonY  = rematch_btn.rect.y;  /* y of the Rematch button          */

    int tw = surf->w, th = surf->h;
    SDL_Rect dst = {
        (1280 - tw) / 2,
        scoreboard_bot + (first_buttonY - scoreboard_bot - th) / 2 ,
        tw, th
    };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf); 
    SDL_DestroyTexture(tex);

    render_button(ren, font, &rematch_btn);
    render_button(ren, font, &selmode_btn);
    render_button(ren, font, &quit_btn_result);
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

static inline void render_walls(SDL_Renderer *ren)
{
    // SDL_SetRenderDrawColor(ren, 184, 134, 11, 255);   /* dark yellow */
    // for (int i = 0; i < 6; ++i) SDL_RenderFillRect(ren, &walls[i]);
}

/* draw goals with 20-px deep-tone border + lighter fill */
/* draw goals – open on one vertical side */
/* draw goals with mixed-tone 10-px borders */
static void render_goals(SDL_Renderer *ren)
{
    const int inset = 10;                       /* border thickness */

    /* colour presets */
    const SDL_Color deepRed    = {200, 0,   0,   255};
    const SDL_Color shallowRed = {255, 70,  70,  255};
    const SDL_Color deepBlue   = {0,   0,   200, 255};
    const SDL_Color shallowBlue= {70,  70,  255, 255};

    /* ---------- LEFT (RED) GOAL ---------- */
    SDL_Rect L_outer = {LEFT_GOAL_X, LEFT_GOAL_Y, LEFT_GOAL_W, LEFT_GOAL_H};
    SDL_Rect L_inner = {LEFT_GOAL_X + inset, LEFT_GOAL_Y + inset,
                        LEFT_GOAL_W - 2*inset, LEFT_GOAL_H - 2*inset};

    /* shallow red fill */
    SDL_SetRenderDrawColor(ren, shallowRed.r, shallowRed.g, shallowRed.b, 255);
    SDL_RenderFillRect(ren, &L_inner);

    /* deep red top border */
    SDL_SetRenderDrawColor(ren, deepRed.r, deepRed.g, deepRed.b, 255);
    SDL_Rect L_top = {L_outer.x, L_outer.y, L_outer.w, inset};
    SDL_RenderFillRect(ren, &L_top);

    /* deep red bottom border */
    SDL_Rect L_bot = {L_outer.x, L_outer.y + L_outer.h - inset, L_outer.w, inset};
    SDL_RenderFillRect(ren, &L_bot);

    /* deep red left border */
    SDL_Rect L_left = {L_outer.x, L_outer.y, inset, L_outer.h};
    SDL_RenderFillRect(ren, &L_left);

    /* shallow red right border */
    SDL_SetRenderDrawColor(ren, shallowRed.r, shallowRed.g, shallowRed.b, 255);
    SDL_Rect L_right = {L_outer.x + L_outer.w - inset, L_outer.y, inset, L_outer.h};
    SDL_RenderFillRect(ren, &L_right);

    /* ---------- RIGHT (BLUE) GOAL ---------- */
    SDL_Rect R_outer = {RIGHT_GOAL_X, RIGHT_GOAL_Y, RIGHT_GOAL_W, RIGHT_GOAL_H};
    SDL_Rect R_inner = {RIGHT_GOAL_X + inset, RIGHT_GOAL_Y + inset,
                        RIGHT_GOAL_W - 2*inset, RIGHT_GOAL_H - 2*inset};

    /* shallow blue fill */
    SDL_SetRenderDrawColor(ren, shallowBlue.r, shallowBlue.g, shallowBlue.b, 255);
    SDL_RenderFillRect(ren, &R_inner);

    /* deep blue top border */
    SDL_SetRenderDrawColor(ren, deepBlue.r, deepBlue.g, deepBlue.b, 255);
    SDL_Rect R_top = {R_outer.x, R_outer.y, R_outer.w, inset};
    SDL_RenderFillRect(ren, &R_top);

    /* deep blue bottom border */
    SDL_Rect R_bot = {R_outer.x, R_outer.y + R_outer.h - inset, R_outer.w, inset};
    SDL_RenderFillRect(ren, &R_bot);

    /* shallow blue left border */
    SDL_SetRenderDrawColor(ren, shallowBlue.r, shallowBlue.g, shallowBlue.b, 255);
    SDL_Rect R_left = {R_outer.x, R_outer.y, inset, R_outer.h};
    SDL_RenderFillRect(ren, &R_left);

    /* deep blue right border */
    SDL_SetRenderDrawColor(ren, deepBlue.r, deepBlue.g, deepBlue.b, 255);
    SDL_Rect R_right = {R_outer.x + R_outer.w - inset, R_outer.y, inset, R_outer.h};
    SDL_RenderFillRect(ren, &R_right);
}





// initial draw and placement
void render_player_zones_and_spots(SDL_Renderer *ren) {
    render_goals(ren); 

    //printf("SPOT_RADIUS = %d\n", SPOT_RADIUS);
    const int spot_radius = 40;
    const float extra_gap_factor = 0.2f; // 20% of radius
    const int n_top = 2, n_bot = 3;

    // Colors
    SDL_Color brown = {90, 50, 20, 255};
    SDL_Color red   = {220, 30, 30, 255};
    SDL_Color blue  = {30, 80, 220, 255};

    // Draw goal rectangles (filled black)
    // SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    // SDL_Rect left_goal  = {LEFT_GOAL_X, LEFT_GOAL_Y, LEFT_GOAL_W, LEFT_GOAL_H};
    // SDL_Rect right_goal = {RIGHT_GOAL_X, RIGHT_GOAL_Y, RIGHT_GOAL_W, RIGHT_GOAL_H};
    // SDL_RenderFillRect(ren, &left_goal);
    // SDL_RenderFillRect(ren, &right_goal);

    // --- Spot X positions (center of yellow area at each side) ---
    int left_spot_x  = LEFT_GOAL_X + LEFT_GOAL_W / 2 - (int)(0.080f * HOLD_ZONE_W) -5;   // 10% more left + 5 more left
    int right_spot_x = RIGHT_GOAL_X + RIGHT_GOAL_W / 2 + (int)(0.080f * HOLD_ZONE_W) +5; // 10% more right +5 more right 


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
/* ──────────────────────────────────────────────────────────────
   Drag, drop, *and* click-to-select with snap-back behaviour
   ────────────────────────────────────────────────────────────── */
void handle_disk_drag_events(SDL_Event *e, int show_default_setup)
{
    /* index of the disk currently being dragged (-1 → none) */
    static int drag_left_idx  = -1;
    static int drag_right_idx = -1;
    static int mouse_off_x = 0, mouse_off_y = 0;

    int cwL = (LEFT_GRID_RIGHT  - LEFT_GRID_LEFT)  / NUM_COLS;
    int chL = (LEFT_GRID_BOTTOM - LEFT_GRID_TOP ) / NUM_ROWS;
    int cwR = (RIGHT_GRID_RIGHT - RIGHT_GRID_LEFT) / NUM_COLS;
    int chR = (RIGHT_GRID_BOTTOM- RIGHT_GRID_TOP ) / NUM_ROWS;

    /* ─────────────────────────────
       1)  MOUSE BUTTON DOWN
       ───────────────────────────── */
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
    {
        int mx = e->button.x, my = e->button.y;
        int hitL = -1, hitR = -1;          /* which disk (if any) was clicked */

        /* scan LEFT disks first */
        for (int i = 0; i < NUM_DISKS; ++i) {
            int cx = (left_disks[i].placed_cell != -1)
                       ? left_grid[left_disks[i].placed_cell].x + cwL/2
                       : left_disks[i].start_x;
            int cy = (left_disks[i].placed_cell != -1)
                       ? left_grid[left_disks[i].placed_cell].y + chL/2
                       : left_disks[i].start_y;
            int dx = mx - cx, dy = my - cy;
            if (dx*dx + dy*dy <= SPOT_RADIUS*SPOT_RADIUS) { hitL = i; break; }
        }

        /* if nothing hit on left, scan RIGHT disks */
        if (hitL == -1) {
            for (int i = 0; i < NUM_DISKS; ++i) {
                int cx = (right_disks[i].placed_cell != -1)
                           ? right_grid[right_disks[i].placed_cell].x + cwR/2
                           : right_disks[i].start_x;
                int cy = (right_disks[i].placed_cell != -1)
                           ? right_grid[right_disks[i].placed_cell].y + chR/2
                           : right_disks[i].start_y;
                int dx = mx - cx, dy = my - cy;
                if (dx*dx + dy*dy <= SPOT_RADIUS*SPOT_RADIUS) { hitR = i; break; }
            }
        }

        /* ---------- update selection state ---------- */
        if (hitL != -1) {
            selected_left_disk  = hitL;
            selected_right_disk = -1;
        } else if (hitR != -1) {
            selected_right_disk = hitR;
            selected_left_disk  = -1;
        } else {                       /* clicked empty space */
            selected_left_disk  = -1;
            selected_right_disk = -1;
        }
        for (int i = 0; i < NUM_DISKS; ++i) {
            left_disks[i].selected  = (i == selected_left_disk);
            right_disks[i].selected = (i == selected_right_disk);
        }

        /* ---------- optional drag start ---------- */
        if (show_default_setup == 0 && hitL != -1) {
            drag_left_idx  = hitL;
            mouse_off_x    = mx - left_disks[hitL].x;
            mouse_off_y    = my - left_disks[hitL].y;

            prev_left_cell = left_disks[hitL].placed_cell;      /* remember origin */
            if (prev_left_cell != -1)
                left_grid[prev_left_cell].occupied_by = -1;     /* free old cell */
            left_disks[hitL].placed_cell = -1;
            left_disks[hitL].is_dragged  = 1;
        }
        else if (show_default_setup == 0 && hitR != -1) {
            drag_right_idx = hitR;
            mouse_off_x    = mx - right_disks[hitR].x;
            mouse_off_y    = my - right_disks[hitR].y;

            prev_right_cell = right_disks[hitR].placed_cell;
            if (prev_right_cell != -1)
                right_grid[prev_right_cell].occupied_by = -1;
            right_disks[hitR].placed_cell = -1;
            right_disks[hitR].is_dragged  = 1;
        }
    }

    /* ─────────────────────────────
       2)  MOUSE MOTION
       ───────────────────────────── */
    if (show_default_setup == 0 && e->type == SDL_MOUSEMOTION) {
        if (drag_left_idx != -1) {
            left_disks[drag_left_idx].x = e->motion.x - mouse_off_x;
            left_disks[drag_left_idx].y = e->motion.y - mouse_off_y;
        }
        if (drag_right_idx != -1) {
            right_disks[drag_right_idx].x = e->motion.x - mouse_off_x;
            right_disks[drag_right_idx].y = e->motion.y - mouse_off_y;
        }
    }

    /* ─────────────────────────────
       3)  MOUSE BUTTON UP
       ───────────────────────────── */
    if (show_default_setup == 0 &&
        e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT)
    {
        int mx = e->button.x, my = e->button.y;

        /* ----- LEFT drop ----- */
        if (drag_left_idx != -1) {
            int i = drag_left_idx; drag_left_idx = -1;
            left_disks[i].is_dragged = 0;

            int col = (mx - LEFT_GRID_LEFT)/cwL, row = (my - LEFT_GRID_TOP)/chL;
            int good_cell =
                col>=0 && col<NUM_COLS && row>=0 && row<NUM_ROWS &&
                mx>=LEFT_GRID_LEFT && mx<=LEFT_GRID_RIGHT &&
                my>=LEFT_GRID_TOP  && my<=LEFT_GRID_BOTTOM;

            if (good_cell) {
                int target = row*NUM_COLS + col;
                if (left_grid[target].occupied_by == -1) {
                    left_disks[i].placed_cell   = target;
                    left_grid[target].occupied_by = i;
                } else good_cell = 0;   /* occupied */
            }

            if (!good_cell) {          /* snap back */
                if (prev_left_cell != -1 &&
                    left_grid[prev_left_cell].occupied_by == -1) {
                    left_disks[i].placed_cell   = prev_left_cell;
                    left_grid[prev_left_cell].occupied_by = i;
                    left_disks[i].x = left_grid[prev_left_cell].x + cwL/2;
                    left_disks[i].y = left_grid[prev_left_cell].y + chL/2;
                } else {
                    left_disks[i].x = left_disks[i].start_x;
                    left_disks[i].y = left_disks[i].start_y;
                }
            }
            prev_left_cell = -1;
        }

        /* ----- RIGHT drop ----- */
        if (drag_right_idx != -1) {
            int i = drag_right_idx; drag_right_idx = -1;
            right_disks[i].is_dragged = 0;

            int col = (mx - RIGHT_GRID_LEFT)/cwR, row = (my - RIGHT_GRID_TOP)/chR;
            int good_cell =
                col>=0 && col<NUM_COLS && row>=0 && row<NUM_ROWS &&
                mx>=RIGHT_GRID_LEFT && mx<=RIGHT_GRID_RIGHT &&
                my>=RIGHT_GRID_TOP  && my<=RIGHT_GRID_BOTTOM;

            if (good_cell) {
                int target = row*NUM_COLS + col;
                if (right_grid[target].occupied_by == -1) {
                    right_disks[i].placed_cell  = target;
                    right_grid[target].occupied_by = i;
                } else good_cell = 0;
            }

            if (!good_cell) {
                if (prev_right_cell != -1 &&
                    right_grid[prev_right_cell].occupied_by == -1) {
                    right_disks[i].placed_cell   = prev_right_cell;
                    right_grid[prev_right_cell].occupied_by = i;
                    right_disks[i].x = right_grid[prev_right_cell].x + cwR/2;
                    right_disks[i].y = right_grid[prev_right_cell].y + chR/2;
                } else {
                    right_disks[i].x = right_disks[i].start_x;
                    right_disks[i].y = right_disks[i].start_y;
                }
            }
            prev_right_cell = -1;
        }
    }
}





/* ---------------------------------------------------------------------
   Render a centred “Red  |score| |score|  Blue” scoreboard bar.
   - height: 48 px
   - label blocks: 120 px wide
   - score blocks:  48 px wide (square)
   --------------------------------------------------------------------- */
/* ------------------------------------------------------------------
    Centred scoreboard:
   [  Red  ] [ 00 ] [ 00 ] [  Blue  ]
   - height: 72 px  (1.5× the old 48)
   - score squares: 72×72 → plenty of room for two digits
   - 2-px black border on every block
   ------------------------------------------------------------------ */
static void render_scoreboard(SDL_Renderer *ren, TTF_Font *font,
                              int red_score, int blue_score)
{
    /* layout constants */
    const int block_h   = 72;          /* common height  */
    const int label_w   = 120;         /* “Red” / “Blue” */
    const int square_w  = block_h;     /* score squares  */
    const int border_px = 2;           /* thicker border */
    const SDL_Color vio = {148, 0, 211, 255};          /* violet fill */
    const SDL_Color txt = {  0, 0,   0, 255};          /* black text  */

    /* total bar width and origin for horizontal centering */
    const int total_w = label_w + square_w + square_w + label_w;
    const SDL_Point origin = { (1280 - total_w) / 2, 6 };    /* top-centre */

    /* block rectangles */
    SDL_Rect blk[4] = {
        { origin.x,                       origin.y, label_w,  block_h },  /* Red label   */
        { origin.x + label_w,             origin.y, square_w, block_h },  /* Red score   */
        { origin.x + label_w + square_w,  origin.y, square_w, block_h },  /* Blue score  */
        { origin.x + label_w + 2*square_w,origin.y, label_w,  block_h }   /* Blue label  */
    };

    /* fill background */
    SDL_SetRenderDrawColor(ren, vio.r, vio.g, vio.b, vio.a);
    for (int i = 0; i < 4; ++i) SDL_RenderFillRect(ren, &blk[i]);

    /* 2-px border: draw two nested 1-px rects */
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    for (int i = 0; i < 4; ++i)
        for (int t = 0; t < border_px; ++t) {
            SDL_Rect r = { blk[i].x + t, blk[i].y + t,
                           blk[i].w - 2*t, blk[i].h - 2*t };
            SDL_RenderDrawRect(ren, &r);
        }

    /* ----- helper to centre any text surface in a rect ----- */
    #define BLIT_TEXT(text, target_rect)                        \
        do {                                                    \
            SDL_Surface *s = TTF_RenderText_Blended(font, text, txt); \
            SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);   \
            SDL_Rect dst = { (target_rect).x + ((target_rect).w - s->w)/2, \
                             (target_rect).y + ((target_rect).h - s->h)/2, \
                             s->w, s->h };                      \
            SDL_RenderCopy(ren, tx, NULL, &dst);                \
            SDL_FreeSurface(s); SDL_DestroyTexture(tx);         \
        } while (0)

    /* render labels and scores */
    BLIT_TEXT("Red",  blk[0]);

    char buf[4];
    snprintf(buf, sizeof(buf), "%d", red_score);   /* supports 0-99 */
    BLIT_TEXT(buf,  blk[1]);

    snprintf(buf, sizeof(buf), "%d", blue_score);
    BLIT_TEXT(buf,  blk[2]);

    BLIT_TEXT("Blue", blk[3]);

    #undef BLIT_TEXT
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
    Button play_btn      = { { (1280-220)/2, 280, 220, 70 }, "Play", 0 };
    Button quit_btn_main = { { (1280-220)/2, 380, 220, 70 }, "Quit", 0 };

    // Mode menu buttons (BIGGER & CENTERED)
    Button single_btn = { { (1280-350)/2, 240, 350, 80 }, "Single Player", 0 };
    Button multi_btn  = { { (1280-350)/2, 350, 350, 80 }, "Multiplayer", 0 };
    Button back_btn   = { { (1280-180)/2, 470, 180, 80 }, "Back", 0 };

    // pause menu buttons
    Button resume_btn    = { { (1280-220)/2, 250, 280, 70 }, "Resume", 0 };
    Button restart_btn   = { { (1280-220)/2, 350, 280, 70 }, "Restart", 0 };
    Button quitmatch_btn = { { (1280-220)/2, 450, 280, 70 }, "Quit Match", 0 };

    /* ---- game-result buttons ---- */ //---------->moved to global
    // Button rematch_btn     = { { (1280-260)/2, 470, 260, 70 }, "Rematch",    0 };
    // Button selmode_btn     = { { (1280-260)/2, 560, 260, 70 }, "Select Mode",0 };
    // Button quit_btn_result = { { (1280-260)/2, 650, 260, 70 }, "Quit",       0 };


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
    int left_spot_x  = LEFT_GOAL_X + LEFT_GOAL_W / 2 - (int)(0.080f * HOLD_ZONE_W) -5;  
    int right_spot_x = RIGHT_GOAL_X + RIGHT_GOAL_W / 2 + (int)(0.080f * HOLD_ZONE_W) +5;  
    
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
            /* ------------------------------------------------------------------
            Window close (red X)
            ------------------------------------------------------------------ */
            if (e.type == SDL_QUIT) {
                running = 0;
            }

            /* ------------------------------------------------------------------
            Drag-and-drop disks (only while playing)
            ------------------------------------------------------------------ */
            if (state == STATE_GAME_RUNNING)
                handle_disk_drag_events(&e, show_default_setup);

            /* ------------------------------------------------------------------
            ESC key: menu back / pause toggle
            ------------------------------------------------------------------ */
            if (e.type == SDL_KEYDOWN &&
                e.key.keysym.sym == SDLK_ESCAPE &&
                !fading_in && !fading_out)
            {
                switch (state) {
                case STATE_MAIN_MENU:
                    fading_out = 1;
                    next_state = STATE_EXIT;
                    break;

                case STATE_MODE_SELECT:
                    fading_out = 1;
                    next_state = STATE_MAIN_MENU;
                    break;

                case STATE_GAME_RUNNING:   /* pause the match */
                    state       = STATE_PAUSE_MENU;
                    fade_alpha  = 0;
                    fading_in   = 0;
                    fading_out  = 0;
                    break;

                case STATE_PAUSE_MENU:     /* resume play */
                    state       = STATE_GAME_RUNNING;
                    fade_alpha  = 0;
                    fading_in   = 0;
                    fading_out  = 0;
                    break;

                default:                   /* ESC does nothing in other states */
                    break;
                }
            }

            /* ------------------------------------------------------------------
            SPACE key: switch from default setup to interactive placement
            ------------------------------------------------------------------ */
            if (e.type == SDL_KEYDOWN &&
                e.key.keysym.sym == SDLK_SPACE &&
                state == STATE_GAME_RUNNING)
            {
                show_default_setup = 0;
            }

            /* ------------------------------------------------------------------
            Mouse clicks (left button)
            ------------------------------------------------------------------ */
            if (e.type == SDL_MOUSEBUTTONDOWN &&
                e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x;
                int my = e.button.y;

                /* ---- MAIN MENU ------------------------------------------------ */
                if (state == STATE_MAIN_MENU && !fading_in && !fading_out) {
                    if (point_in_rect(mx, my, &play_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_MODE_SELECT;
                    }
                    else if (point_in_rect(mx, my, &quit_btn_main.rect)) {
                        fading_out = 1;
                        next_state = STATE_EXIT;
                    }
                }

                /* ---- MODE-SELECT MENU ---------------------------------------- */
                else if (state == STATE_MODE_SELECT && !fading_in && !fading_out) {
                    if (point_in_rect(mx, my, &single_btn.rect) ||
                        point_in_rect(mx, my, &multi_btn.rect))
                    {
                        fading_out = 1;
                        next_state = STATE_GAME_RUNNING;
                    }
                    else if (point_in_rect(mx, my, &back_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_MAIN_MENU;
                    }
                }

                /* ---- PAUSE MENU ---------------------------------------------- */
                else if (state == STATE_PAUSE_MENU && !fading_in && !fading_out) {
                    if (point_in_rect(mx, my, &resume_btn.rect)) {
                        state = STATE_GAME_RUNNING;
                    }
                    else if (point_in_rect(mx, my, &restart_btn.rect)) {
                        /* TODO: reset game state here */
                        state = STATE_GAME_RUNNING;
                    }
                    else if (point_in_rect(mx, my, &quitmatch_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_GAME_RESULT;
                    }
                }

                /* ---- GAME-RESULT SCREEN -------------------------------------- */
                else if (state == STATE_GAME_RESULT && !fading_in && !fading_out) {
                    if (point_in_rect(mx, my, &rematch_btn.rect)) {
                        left_score  = 0;
                        right_score = 0;
                        /* TODO: reset board/disks here */
                        fading_out = 1;
                        next_state = STATE_GAME_RUNNING;
                    }
                    else if (point_in_rect(mx, my, &selmode_btn.rect)) {
                        fading_out = 1;
                        next_state = STATE_MODE_SELECT;
                    }
                    else if (point_in_rect(mx, my, &quit_btn_result.rect)) {
                        fading_out = 1;
                        next_state = STATE_EXIT;
                    }
                }
            }
        }


        // Render
        SDL_RenderClear(ren);
                                                                                        // render_background(ren, bg);
        switch (state) 
        {
            /* ──────────────────  STARTUP  ────────────────── */
            case STATE_STARTUP_ANIMATION: {
                    /* logo in the centre */
                    SDL_Color white = {255, 255, 255, 255};
                    SDL_Surface *surf = TTF_RenderText_Blended(font, "NO BALL FOOTBALL!", white);
                    SDL_Texture *tex  = SDL_CreateTextureFromSurface(ren, surf);
                    int tw = surf->w, th = surf->h;
                    SDL_Rect dst = { (1280 - tw) / 2, (720 - th) / 2, tw, th };
                    SDL_RenderCopy(ren, tex, NULL, &dst);
                    SDL_FreeSurface(surf); SDL_DestroyTexture(tex);

                    /* fade-in / hold / fade-out */
                    if (fading_in) {
                        fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha <= 0) { fade_alpha = 0; fading_in = 0; state_timer = SDL_GetTicks(); }
                    } else if (!fading_out && SDL_GetTicks() - state_timer > 1000) {
                        fading_out = 1;
                    }
                    if (fading_out) {
                        fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha >= 255) {
                            fade_alpha = 255;
                            fading_out = 0;
                            fading_in  = 1;
                            state      = STATE_MAIN_MENU;
                        }
                    }
                    render_fade(ren, fade_alpha);
                    break;
                }

            /* ──────────────────  MAIN MENU  ────────────────── */
            case STATE_MAIN_MENU: {
                    render_background(ren, bg);

                    SDL_Color white = {255, 255, 255, 255};
                    SDL_Surface *surf = TTF_RenderText_Blended(font, "Main Menu", white);
                    SDL_Texture *tex  = SDL_CreateTextureFromSurface(ren, surf);
                    int tw = surf->w, th = surf->h;
                    SDL_Rect dst = { (1280 - tw) / 2, 120, tw, th };
                    SDL_RenderCopy(ren, tex, NULL, &dst);
                    SDL_FreeSurface(surf); SDL_DestroyTexture(tex);

                    render_button(ren, font, &play_btn);
                    render_button(ren, font, &quit_btn_main);

                    if (fading_in) {
                        fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha <= 0) { fade_alpha = 0; fading_in = 0; }
                    }
                    if (fading_out) {
                        fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha >= 255) {
                            fade_alpha = 255; fading_out = 0;
                            if (next_state == STATE_GAME_RUNNING) { fading_in = 0; }
                            else { fading_in = 1; fade_alpha = 255; }
                            state = next_state;
                        }
                    }
                    render_fade(ren, fade_alpha);
                    break;
                }

            /* ──────────────────  MODE-SELECT MENU  ────────────────── */
            case STATE_MODE_SELECT: {
                    render_background(ren, bg);

                    SDL_Color white = {255, 255, 255, 255};
                    SDL_Surface *surf = TTF_RenderText_Blended(font, "Select Mode", white);
                    SDL_Texture *tex  = SDL_CreateTextureFromSurface(ren, surf);
                    int tw = surf->w, th = surf->h;
                    SDL_Rect dst = { (1280 - tw) / 2, 120, tw, th };
                    SDL_RenderCopy(ren, tex, NULL, &dst);
                    SDL_FreeSurface(surf); SDL_DestroyTexture(tex);

                    render_button(ren, font, &single_btn);
                    render_button(ren, font, &multi_btn);
                    render_button(ren, font, &back_btn);

                    if (fading_in) {
                        fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha <= 0) { fade_alpha = 0; fading_in = 0; }
                    }
                    if (fading_out) {
                        fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha >= 255) {
                            fade_alpha = 255; fading_out = 0;
                            if (next_state == STATE_GAME_RUNNING) { fading_in = 0; }
                            else { fading_in = 1; fade_alpha = 255; }
                            state = next_state;
                        }
                    }
                    render_fade(ren, fade_alpha);
                    break;
                }

            /* ──────────────────  GAME RUNNING  ────────────────── */
            case STATE_GAME_RUNNING: {
                    render_background(ren, game_bg);
                    render_goals(ren);
                    render_scoreboard(ren, font, left_score, right_score);

                    if (show_default_setup) {
                        render_player_zones_and_spots(ren);
                    } else {
                        render_left_grid_and_disks(ren);
                        render_right_grid_and_disks(ren);
                    }
                    break;
                }

            /* ──────────────────  PAUSE MENU  ────────────────── */
            case STATE_PAUSE_MENU: {
                    render_background(ren, game_bg);
                    render_goals(ren);

                    render_pause_menu(ren, font,
                                    &resume_btn, &restart_btn, &quitmatch_btn);

                    render_scoreboard(ren, font, left_score, right_score);

                    if (fading_out) {
                        fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha >= 255) {
                            fade_alpha = 255; fading_out = 0;

                            if (next_state == STATE_MAIN_MENU ||
                                next_state == STATE_MODE_SELECT ||
                                next_state == STATE_GAME_RESULT) {
                                fading_in  = 1;
                                fade_alpha = 255;
                            } else {
                                fading_in = 0;   /* returning to play */
                            }
                            state = next_state;
                        }
                    }
                    render_fade(ren, fade_alpha);
                    break;
                }

            /* ──────────────────  GAME RESULT  ────────────────── */
            case STATE_GAME_RESULT: {
                    render_background(ren, game_bg);
                    render_goals(ren);
                    render_game_result(ren, font, left_score, right_score);

                    if (fading_in) {
                        fade_alpha -= (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha <= 0) { fade_alpha = 0; fading_in = 0; }
                    }
                    if (fading_out) {
                        fade_alpha += (int)(fade_speed * (delta_time / 1000.0f));
                        if (fade_alpha >= 255) {
                            fade_alpha = 255; fading_out = 0;

                            if (next_state == STATE_MAIN_MENU ||
                                next_state == STATE_MODE_SELECT) {
                                fading_in  = 1; fade_alpha = 255;
                            } else {
                                fading_in = 0;
                            }
                            state = next_state;
                        }
                    }
                    render_fade(ren, fade_alpha);
                    break;
                }

            /* ──────────────────  EXIT  ────────────────── */
            case STATE_EXIT:
                    running = 0;
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