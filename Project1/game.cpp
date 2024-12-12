#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

// Game-related variables
float player_1_p, player_1_dp, player_2_p, player_2_dp; // Positions and velocities of players
float arena_half_size_x = 85, arena_half_size_y = 45;   // Arena dimensions
float player_1_half_size_x = 2.5, player_1_half_size_y = 12; // Player 1 paddle dimensions
float player_2_half_size_x = 2.5, player_2_half_size_y = 12; // Player 2 paddle dimensions
float ball_p_x, ball_p_y, ball_dp_x = 130, ball_dp_y, ball_half_size = 1; // Ball properties
int player_1_score, player_2_score; // Scores for both players

u32 background_colors[] = {
    0x1F1F1F, // Dark Grey
    0x001F3F, // Navy Blue
    0x0A0A0A, // Almost Black
    0x112233, // Dark Slate
    0x2E3440, // Nordic Grey
    0x282828, // Graphite Grey
    0x3B4252, // Soft Black
    0x4C566A, // Stormy Grey
    0x5E81AC, // Sky Blue
    0x242424  // Charcoal Black
};

u32 paddle_border_colors[] = {
    0xFFFFFF, // White
    0xFF4136, // Bright Red
    0x00FF00, // Green
    0x88C0D0, // Light Cyan
    0xBF616A, // Muted Red
    0xA3BE8C, // Forest Green
    0xD08770, // Burnt Orange
    0xEBCB8B, // Golden Yellow
    0x81A1C1, // Pastel Blue
    0xECEFF4  // Frosty White
};
int i;
u32 color1 = background_colors[0];
u32 color2 = paddle_border_colors[0];


// Function to simulate player movement with acceleration and boundary collision handling
internal void
simulate_player(float* p, float* dp, float ddp, float dt, float half_size_y) {
    // Apply acceleration and drag
    ddp -= *dp * 10.f;

    // Update position and velocity using kinematic equations
    *p = *p + *dp * dt + ddp * dt * dt * .5f;
    *dp = *dp + ddp * dt;

    // Check for boundary collisions and apply bounce
    if (*p + half_size_y > arena_half_size_y) {
        *p = arena_half_size_y - half_size_y;
        *dp *= -2;
    }
    else if (*p - half_size_y < -arena_half_size_y) {
        *p = -arena_half_size_y + half_size_y;
        *dp *= -2;
    }
}

// Function to check Axis-Aligned Bounding Box (AABB) collision detection
internal bool
aabb_vs_aabb(float p1x, float p1y, float hs1x, float hs1y,
    float p2x, float p2y, float hs2x, float hs2y) {
    return (p1x + hs1x > p2x - hs2x &&
        p1x - hs1x < p2x + hs2x &&
        p1y + hs1y > p2y - hs2y &&
        p1y - hs1y < p2y + hs2y);
}

// Enumeration for game modes
enum Gamemode {
    GM_MENU,
    GM_GAMEPLAY,
};

Gamemode current_gamemode; // Current game mode (Menu or Gameplay)
int hot_button; // Tracks the selected menu button
bool enemy_is_ai; // Whether the opponent is AI-controlled

// Main game simulation function
internal void
simulate_game(Input* input, float dt) {
    // Draw arena and borders
    draw_rect(0, 0, arena_half_size_x, arena_half_size_y, color1);
    draw_arena_borders(arena_half_size_x, arena_half_size_y, color2);

    if (current_gamemode == GM_GAMEPLAY) {
        // Simulate player movement
        float player_1_ddp = 0.f;
        if (!enemy_is_ai) { // Player-controlled
            if (is_down(BUTTON_UP)) player_1_ddp += 2000;
            if (is_down(BUTTON_DOWN)) player_1_ddp -= 2000;
        }
        else { // AI-controlled
            player_1_ddp = (ball_p_y - player_1_p) * 100;
            if (player_1_ddp > 1300) player_1_ddp = 1300;
            if (player_1_ddp < -1300) player_1_ddp = -1300;
        }

        // Player 2 input
        float player_2_ddp = 0.f;
        if (is_down(BUTTON_W)) player_2_ddp += 2000;
        if (is_down(BUTTON_S)) player_2_ddp -= 2000;

        // Update both players
        simulate_player(&player_1_p, &player_1_dp, player_1_ddp, dt, player_1_half_size_y);
        simulate_player(&player_2_p, &player_2_dp, player_2_ddp, dt, player_2_half_size_y);

        // Simulate ball movement and collision
        ball_p_x += ball_dp_x * dt;
        ball_p_y += ball_dp_y * dt;

        // Ball collision with players
        if (aabb_vs_aabb(ball_p_x, ball_p_y, ball_half_size, ball_half_size, 80, player_1_p, player_1_half_size_x, player_1_half_size_y)) {
            ball_p_x = 80 - player_1_half_size_x - ball_half_size;
            ball_dp_x *= -1;
            ball_dp_y = (ball_p_y - player_1_p) * 2 + player_1_dp * .75f;
        }
        else if (aabb_vs_aabb(ball_p_x, ball_p_y, ball_half_size, ball_half_size, -80, player_2_p, player_2_half_size_x, player_2_half_size_y)) {
            ball_p_x = -80 + player_2_half_size_x + ball_half_size;
            ball_dp_x *= -1;
            ball_dp_y = (ball_p_y - player_2_p) * 2 + player_2_dp * .75f;
        }

        // Ball collision with arena boundaries
        if (ball_p_y + ball_half_size > arena_half_size_y) {
            ball_p_y = arena_half_size_y - ball_half_size;
            ball_dp_y *= -1;
        }
        else if (ball_p_y - ball_half_size < -arena_half_size_y) {
            ball_p_y = -arena_half_size_y + ball_half_size;
            ball_dp_y *= -1;
        }

        // Score handling
        if (ball_p_x + ball_half_size > arena_half_size_x) {
            ball_dp_x *= -1;
            ball_dp_y = 0;
            ball_p_x = ball_p_y = 0;
            player_1_score++;
            if (player_1_score > 5 && !enemy_is_ai) {
                player_2_half_size_y = (player_2_half_size_y <= 4) ? 3 : player_2_half_size_y - 2;
            }
            if (i == 9) i = 0;
            else i++;
            color1 = background_colors[i];
            color2 = paddle_border_colors[i];
        }
        else if (ball_p_x - ball_half_size < -arena_half_size_x) {
            ball_dp_x *= -1;
            ball_dp_y = 0;
            ball_p_x = ball_p_y = 0;
            player_2_score++;
            if (player_2_score > 5 && !enemy_is_ai) {
                player_1_half_size_y = (player_1_half_size_y <= 4) ? 3 : player_1_half_size_y - 2;
            }
            if (i == 9) i = 0;
            else i++;
            color1 = background_colors[i];
            color2 = paddle_border_colors[i];
        }

        // Draw scores, ball, and paddles
        draw_number(player_1_score, -10, 40, 1.f, color2);
        draw_number(player_2_score, 10, 40, 1.f, color2);
        draw_rect(ball_p_x, ball_p_y, ball_half_size, ball_half_size, color2);
        draw_rect(80, player_1_p, player_1_half_size_x, player_1_half_size_y, color2);
        draw_rect(-80, player_2_p, player_2_half_size_x, player_2_half_size_y, color2);

    }
    else { // Menu mode
        // Menu navigation
        if (pressed(BUTTON_LEFT) || pressed(BUTTON_RIGHT)) hot_button = !hot_button;
        if (pressed(BUTTON_ENTER)) {
            current_gamemode = GM_GAMEPLAY;
            enemy_is_ai = hot_button ? 0 : 1;
        }

        // Render menu
        if (hot_button == 0) {
            draw_text("SINGLE", -60, -10, 1, 0xaaaaaa);
            draw_text("AI", 40, -10, 1, 0xff0000);
        }
        else {
            draw_text("SINGLE", -60, -10, 1, 0xff0000);
            draw_text("AI", 40, -10, 1, 0xaaaaaa);
        }
        draw_text("PONG", -20, 40, 2, 0xffffff);
    }
};
