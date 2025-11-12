#include "app_2048.h"
#include "ui.h"
#include "ui_fonts.h"
#include "esp_log.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* TAG = "APP_2048";

#define GRID_SIZE 4
#define TILE_SIZE 90
#define TILE_GAP 10
#define GRID_PADDING 20

typedef struct {
    lv_obj_t* container;
    lv_obj_t* grid_container;
    lv_obj_t* tiles[GRID_SIZE][GRID_SIZE];
    lv_obj_t* score_label;
    int board[GRID_SIZE][GRID_SIZE];
    int score;
    bool game_over;
} game_2048_t;

static game_2048_t game = {0};

static lv_color_t tile_colors[12];

static void init_tile_colors(void) {
    tile_colors[0] = lv_color_hex(0xcdc1b4);   // 0 (empty)
    tile_colors[1] = lv_color_hex(0xeee4da);   // 2
    tile_colors[2] = lv_color_hex(0xede0c8);   // 4
    tile_colors[3] = lv_color_hex(0xf2b179);   // 8
    tile_colors[4] = lv_color_hex(0xf59563);   // 16
    tile_colors[5] = lv_color_hex(0xf67c5f);   // 32
    tile_colors[6] = lv_color_hex(0xf65e3b);   // 64
    tile_colors[7] = lv_color_hex(0xedcf72);   // 128
    tile_colors[8] = lv_color_hex(0xedcc61);   // 256
    tile_colors[9] = lv_color_hex(0xedc850);   // 512
    tile_colors[10] = lv_color_hex(0xedc53f);  // 1024
    tile_colors[11] = lv_color_hex(0xedc22e);  // 2048
}

static void add_random_tile(void);
static void update_ui(void);
static bool move_tiles(int dx, int dy);
static void gesture_event_cb(lv_event_t* e);
static void reset_game(void);

static int get_color_index(int value) {
    if (value == 0) return 0;
    int index = 0;
    int temp = value;
    while (temp > 1) {
        temp /= 2;
        index++;
    }
    return index > 11 ? 11 : index;
}

static void add_random_tile(void) {
    int empty_tiles[GRID_SIZE * GRID_SIZE][2];
    int count = 0;
    
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.board[i][j] == 0) {
                empty_tiles[count][0] = i;
                empty_tiles[count][1] = j;
                count++;
            }
        }
    }
    
    if (count > 0) {
        int idx = rand() % count;
        int value = (rand() % 10 < 9) ? 2 : 4;
        game.board[empty_tiles[idx][0]][empty_tiles[idx][1]] = value;
    }
}

static void update_ui(void) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int value = game.board[i][j];
            lv_obj_t* tile = game.tiles[i][j];
            
            if (tile) {
                int color_idx = get_color_index(value);
                lv_obj_set_style_bg_color(tile, tile_colors[color_idx], 0);
                
                lv_obj_t* label = lv_obj_get_child(tile, 0);
                if (label) {
                    if (value > 0) {
                        lv_label_set_text_fmt(label, "%d", value);
                        lv_obj_set_style_text_color(label, 
                            value <= 4 ? lv_color_hex(0x776e65) : lv_color_white(), 0);
                    } else {
                        lv_label_set_text(label, "");
                    }
                }
            }
        }
    }
    
    if (game.score_label) {
        lv_label_set_text_fmt(game.score_label, "Score: %d", game.score);
    }
}

static bool can_move(void) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.board[i][j] == 0) return true;
            if (i < GRID_SIZE - 1 && game.board[i][j] == game.board[i + 1][j]) return true;
            if (j < GRID_SIZE - 1 && game.board[i][j] == game.board[i][j + 1]) return true;
        }
    }
    return false;
}

static bool move_tiles(int dx, int dy) {
    bool moved = false;
    int startX = (dx == 1) ? GRID_SIZE - 2 : 0;
    int startY = (dy == 1) ? GRID_SIZE - 2 : 0;
    int endX = (dx == 1) ? -1 : GRID_SIZE;
    int endY = (dy == 1) ? -1 : GRID_SIZE;
    int stepX = (dx == 1) ? -1 : 1;
    int stepY = (dy == 1) ? -1 : 1;
    
    for (int i = startX; i != endX; i += stepX) {
        for (int j = startY; j != endY; j += stepY) {
            if (game.board[i][j] == 0) continue;
            
            int newI = i, newJ = j;
            
            while (true) {
                int nextI = newI + dx;
                int nextJ = newJ + dy;
                
                if (nextI < 0 || nextI >= GRID_SIZE || nextJ < 0 || nextJ >= GRID_SIZE) break;
                if (game.board[nextI][nextJ] != 0 && game.board[nextI][nextJ] != game.board[i][j]) break;
                
                newI = nextI;
                newJ = nextJ;
                
                if (game.board[newI][newJ] != 0) break;
            }
            
            if (newI != i || newJ != j) {
                moved = true;
                if (game.board[newI][newJ] == game.board[i][j]) {
                    game.board[newI][newJ] *= 2;
                    game.score += game.board[newI][newJ];
                } else {
                    game.board[newI][newJ] = game.board[i][j];
                }
                game.board[i][j] = 0;
            }
        }
    }
    
    return moved;
}

static void gesture_event_cb(lv_event_t* e) {
    if (game.game_over) return;
    
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    bool moved = false;
    
    switch (dir) {
        case LV_DIR_LEFT:
            moved = move_tiles(0, -1);
            break;
        case LV_DIR_RIGHT:
            moved = move_tiles(0, 1);
            break;
        case LV_DIR_TOP:
            moved = move_tiles(-1, 0);
            break;
        case LV_DIR_BOTTOM:
            moved = move_tiles(1, 0);
            break;
        default:
            return;
    }
    
    if (moved) {
        add_random_tile();
        update_ui();
        
        if (!can_move()) {
            game.game_over = true;
            ESP_LOGI(TAG, "Game Over! Score: %d", game.score);
            
            lv_obj_t* game_over_label = lv_label_create(game.container);
            lv_label_set_text(game_over_label, "Game Over!");
            lv_obj_set_style_text_color(game_over_label, lv_color_hex(0xff0000), 0);
            lv_obj_set_style_text_font(game_over_label, &font_bold_32, 0);
            lv_obj_align(game_over_label, LV_ALIGN_CENTER, 0, 0);
        }
    }
}

static void reset_game(void) {
    memset(game.board, 0, sizeof(game.board));
    game.score = 0;
    game.game_over = false;
    
    srand(time(NULL));
    add_random_tile();
    add_random_tile();
    update_ui();
}

void app_2048_create(lv_obj_t* parent) {
    if (!parent) {
        ESP_LOGE(TAG, "Parent object is NULL");
        return;
    }

    ESP_LOGI(TAG, "Creating 2048 game");

    memset(&game, 0, sizeof(game));
    init_tile_colors();

    game.container = lv_obj_create(parent);
    lv_obj_set_size(game.container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(game.container, lv_color_hex(0xfaf8ef), 0);
    lv_obj_set_style_bg_opa(game.container, LV_OPA_100, 0);
    lv_obj_set_style_border_width(game.container, 0, 0);
    lv_obj_set_style_pad_all(game.container, GRID_PADDING, 0);
    lv_obj_center(game.container);
    lv_obj_add_event_cb(game.container, gesture_event_cb, LV_EVENT_GESTURE, NULL);

    // Score label
    game.score_label = lv_label_create(game.container);
    lv_label_set_text(game.score_label, "Score: 0");
    lv_obj_set_style_text_color(game.score_label, lv_color_hex(0x776e65), 0);
    lv_obj_set_style_text_font(game.score_label, &font_bold_26, 0);
    lv_obj_align(game.score_label, LV_ALIGN_TOP_MID, 0, 10);

    // Grid container
    game.grid_container = lv_obj_create(game.container);
    lv_obj_set_size(game.grid_container, 
        GRID_SIZE * TILE_SIZE + (GRID_SIZE - 1) * TILE_GAP,
        GRID_SIZE * TILE_SIZE + (GRID_SIZE - 1) * TILE_GAP);
    lv_obj_set_style_bg_color(game.grid_container, lv_color_hex(0xbbada0), 0);
    lv_obj_set_style_bg_opa(game.grid_container, LV_OPA_100, 0);
    lv_obj_set_style_border_width(game.grid_container, 0, 0);
    lv_obj_set_style_pad_all(game.grid_container, TILE_GAP, 0);
    lv_obj_set_style_radius(game.grid_container, 8, 0);
    lv_obj_center(game.grid_container);
    lv_obj_clear_flag(game.grid_container, LV_OBJ_FLAG_SCROLLABLE);

    // Create tiles
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            lv_obj_t* tile = lv_obj_create(game.grid_container);
            lv_obj_set_size(tile, TILE_SIZE, TILE_SIZE);
            lv_obj_set_pos(tile, 
                j * (TILE_SIZE + TILE_GAP),
                i * (TILE_SIZE + TILE_GAP));
            lv_obj_set_style_radius(tile, 6, 0);
            lv_obj_set_style_border_width(tile, 0, 0);
            lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t* label = lv_label_create(tile);
            lv_obj_set_style_text_font(label, &font_bold_32, 0);
            lv_obj_center(label);

            game.tiles[i][j] = tile;
        }
    }

    reset_game();
    ESP_LOGI(TAG, "2048 game created successfully");
}
