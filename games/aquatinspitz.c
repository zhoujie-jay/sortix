/*
 * Copyright (c) 2014, 2015, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * aquatinspitz.c
 * Aqua tin spitz!
 */

#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include <dispd.h>

// Utility global variables every game will need.
static bool game_running = true;
static size_t game_width = 1280;
static size_t game_height = 720;
#define MAX_KEY_NUMBER 512
static bool keys_down[MAX_KEY_NUMBER];
static bool keys_pending[MAX_KEY_NUMBER];
static struct timespec key_handled_last[MAX_KEY_NUMBER];

// Utility functions every game will need.
bool pop_is_key_just_down(int abskbkey);
static inline uint32_t make_color(uint8_t r, uint8_t g, uint8_t b);

// Your game is customized from here ...

struct player
{
	float x;
	float y;
	int size;
};

struct player player;

struct enemy
{
	float x;
	float y;
	float vx;
	float vy;
	int size;
	int shift;
};

#define NUM_ENEMIES 256
static struct enemy enemies[NUM_ENEMIES];

// Prepare the game state for the first round.
void init(void)
{
	player.x = game_width / 2;
	player.y = game_height / 2;
	player.size = 24.0;

	for ( size_t i = 0; i < NUM_ENEMIES; i++ )
	{
		enemies[i].x = (float) arc4random_uniform(game_width);
		enemies[i].y = (float) arc4random_uniform(game_height);
		enemies[i].vx = (float) ((int) arc4random_uniform(96) - 48);
		enemies[i].vy = (float) ((int) arc4random_uniform(96) - 48);
		enemies[i].size = arc4random_uniform(8) + 8;
		enemies[i].shift = (int) arc4random_uniform(6) - 3;
		if ( enemies[i].shift <= 0 )
			enemies[i].shift -= 1;
	}
}

// Calculate the game state of the next round.
void update(float deltatime)
{
	float player_speed = 64.0f;
	float player_velocity_x = 0.0f;
	float player_velocity_y = 0.0f;
	if ( keys_down[KBKEY_UP] )
		player_velocity_y -= player_speed;
	if ( keys_down[KBKEY_DOWN] )
		player_velocity_y += player_speed;
	if ( keys_down[KBKEY_LEFT] )
		player_velocity_x -= player_speed;
	if ( keys_down[KBKEY_RIGHT] )
		player_velocity_x += player_speed;
	player.x += deltatime * player_velocity_x;
	player.y += deltatime * player_velocity_y;

	if ( pop_is_key_just_down(KBKEY_SPACE) )
		player.size = 192 - player.size;

	float total_speed = 0.0;
	for ( size_t i = 0; i < NUM_ENEMIES; i++ )
	{
		struct enemy* enemy = &enemies[i];
		float g = 10000.0;
		float dist_sq = (player.x - enemy->x) * (player.x - enemy->x) +
		                (player.y - enemy->y) * (player.y - enemy->y);
		if ( dist_sq < 0.1 )
			dist_sq = 0.1;
		float dist = sqrtf(dist_sq);
		float f = g * enemy->size * player.size / dist_sq;
		float f_x = (player.x - enemy->x) / dist * f;
		float f_y = (player.y - enemy->y) / dist * f;
		float a_x = f_x / enemy->size;
		float a_y = f_y / enemy->size;
		enemy->vx += deltatime * a_x;
		enemy->vy += deltatime * a_y;
		float speed = sqrtf(enemy->vx * enemy->vx + enemy->vy * enemy->vy);
		total_speed += speed;
	}

	float average_speed = total_speed / NUM_ENEMIES;
	float mid_game = game_width / 2.0;

	for ( size_t i = 0; i < NUM_ENEMIES; i++ )
	{
		struct enemy* enemy = &enemies[i];
		float speed = sqrtf(enemy->vx * enemy->vx + enemy->vy * enemy->vy);
		float ox = enemy->x;
		float oy = enemy->y;
		float nx = ox + deltatime * enemy->vx;
		float ny = oy + deltatime * enemy->vy;
		if ( mid_game + enemy->size / 2 < ox &&
		     nx <= mid_game + enemy->size / 2 )
		{
			if ( speed < average_speed )
			{
				if ( enemy->vx < 0.0 )
					enemy->vx = -enemy->vx;
				continue;
			}
		}
		else if ( ox <= mid_game - enemy->size / 2 &&
		          mid_game - enemy->size / 2 < nx )
		{
			if ( speed >= average_speed )
			{
				if ( enemy->vx > 0.0 )
					enemy->vx = -enemy->vx;
				continue;
			}
		}
		enemy->x = nx;
		enemy->y = ny;
	}

	for ( size_t i = 0; i < NUM_ENEMIES; i++ )
	{
		struct enemy* enemy = &enemies[i];
		if ( enemy->x - enemy->size / 2 < 0 )
		{
			enemy->x = 0.0f + enemy->size / 2;
			if ( enemy->vx < 0.0 )
				enemy->vx = -0.9 * enemy->vx;
		}
		else if ( game_width < (size_t) (enemy->x + enemy->size / 2) )
		{
			enemy->x = (float) game_width - enemy->size / 2;
			if ( 0.0 < enemy->vx )
				enemy->vx = -0.9 * enemy->vx;
		}
		if ( enemy->y - enemy->size / 2 < 0 )
		{
			enemy->y = 0.0f + enemy->size / 2;
			if ( enemy->vy < 0.0 )
				enemy->vy = -0.9 * enemy->vy;
		}
		else if ( game_height < (size_t) (enemy->y + enemy->size / 2) )
		{
			enemy->y = (float) game_height - enemy->size / 2;
			if ( 0.0 < enemy->vy )
				enemy->vy = -0.9 * enemy->vy;
		}
	}
}

// Render the game into the framebuffer.
void render(struct dispd_window* window)
{
	struct dispd_framebuffer* window_fb = dispd_begin_render(window);
	if ( !window_fb )
	{
		error(0, 0, "unable to begin rendering dispd window");
		game_running = false;
		return;
	}

	uint32_t* fb = (uint32_t*) dispd_get_framebuffer_data(window_fb);
	size_t xres = dispd_get_framebuffer_width(window_fb);
	size_t yres = dispd_get_framebuffer_height(window_fb);
	size_t pitch = dispd_get_framebuffer_pitch(window_fb) / sizeof(uint32_t);

	// Render a colorful background.
	for ( size_t y = 0; y < yres; y++ )
	{
		for ( size_t x = 0; x < xres; x++ )
		{
			uint32_t color = make_color(x * y, y ? x / y : 255, x ^ y);
			fb[y * pitch + x] = color;
		}
	}

	// Render the player.
	for ( int t = -player.size / 2; t < player.size / 2; t++ )
	{
		if ( player.y + t < 0 )
			continue;
		size_t y = (size_t) (player.y + t);
		if ( yres <= y )
			continue;
		for ( int l = -player.size / 2; l < player.size / 2; l++ )
		{
			if ( player.x + l < 0 )
				continue;
			size_t x = (size_t) (player.x + l);
			if ( xres <= x )
				continue;
			uint32_t background = fb[y * pitch + x];
			uint32_t color = ~background;
			fb[y * pitch + x] = color;
		}
	}

	// Render the enemies.
	for ( size_t i = 0; i < NUM_ENEMIES; i++ )
	{
		struct enemy* enemy = &enemies[i];
		for ( int t = -enemy->size / 2; t < enemy->size / 2; t++ )
		{
			if ( enemy->y + t < 0 )
				continue;
			size_t y = (size_t) (enemy->y + t);
			if ( yres <= y )
				continue;
			for ( int l = -enemy->size / 2; l < enemy->size / 2; l++ )
			{
				if ( enemy->x + l < 0 )
					continue;
				size_t x = (size_t) (enemy->x + l);
				if ( xres <= x )
					continue;
				uint32_t background = fb[y * pitch + x];
				uint32_t color = enemy->shift < 0 ? background >> -enemy->shift
				                                  : background <<  enemy->shift;
				color = ~color;
				fb[y * pitch + x] = color;
			}
		}
	}

	dispd_finish_render(window_fb);
}

// ... to here. No need to edit stuff below.

// Create a color from rgb values.
static inline uint32_t make_color(uint8_t r, uint8_t g, uint8_t b)
{
	return b << 0UL | g << 8UL | r << 16UL;
}

// Return if a keystroke is pending. For instance, if you press A on your
// keyboard and keep pressing it, a new A character will appear every time a
// small interval has passed, not just every time the code checks if A is down.
bool pop_is_key_just_down(int abskbkey)
{
	assert(0 <= abskbkey);
	if ( MAX_KEY_NUMBER <= (size_t) abskbkey )
		return false;
	if ( keys_pending[abskbkey] )
	{
		keys_pending[abskbkey] = false;
		clock_gettime(CLOCK_MONOTONIC, &key_handled_last[abskbkey]);
		return true;
	}
	if ( !keys_down[abskbkey] )
		return false;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	struct timespec elapsed = timespec_sub(now, key_handled_last[abskbkey]);
	struct timespec repress_delay = timespec_make(0, 100 * 1000 * 1000);
	if ( timespec_lt(elapsed, repress_delay) )
		return false;
	clock_gettime(CLOCK_MONOTONIC, &key_handled_last[abskbkey]);
	return true;
}

// Read input from the keyboard.
void input(void)
{
	// Read the keyboard input from the user.
	unsigned termmode = TERMMODE_KBKEY | TERMMODE_SIGNAL | TERMMODE_NONBLOCK;
	if ( settermmode(0, termmode) )
		error(1, errno, "settermmode");
	uint32_t codepoint;
	ssize_t numbytes;
	while ( 0 < (numbytes = read(0, &codepoint, sizeof(codepoint))) )
	{
		int kbkey = KBKEY_DECODE(codepoint);
		if( !kbkey )
			continue;
		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
		if ( MAX_KEY_NUMBER <= (size_t) abskbkey )
			continue;
		bool is_key_down_event = 0 < kbkey;
		if ( !keys_down[abskbkey] && is_key_down_event )
			keys_pending[abskbkey] = true;
		keys_down[abskbkey] = is_key_down_event;
	}
}

// Run the game until no longer needed.
void mainloop(struct dispd_window* window)
{
	struct dispd_framebuffer* window_fb = dispd_begin_render(window);
	if ( window_fb )
	{
		game_width = dispd_get_framebuffer_width(window_fb);
		game_height = dispd_get_framebuffer_height(window_fb);
		dispd_finish_render(window_fb);
	}

	init();

	struct timespec last_frame_time;
	clock_gettime(CLOCK_MONOTONIC, &last_frame_time);

	render(window);

	while ( game_running )
	{
		struct timespec current_frame_time;
		clock_gettime(CLOCK_MONOTONIC, &current_frame_time);

		struct timespec deltatime_ts =
			timespec_sub(current_frame_time, last_frame_time);
		float deltatime = deltatime_ts.tv_sec + deltatime_ts.tv_nsec / 1E9f;

		input();
		update(deltatime);
		render(window);

		last_frame_time = current_frame_time;
	}
}

// Reset the terminal state when the process terminates.
static struct termios saved_tio;

static void restore_terminal_on_exit(void)
{
	tcsetattr(0, TCSAFLUSH, &saved_tio);
}

static void restore_terminal_on_signal(int signum)
{
	if ( signum == SIGTSTP )
	{
		struct termios tio;
		tcgetattr(0, &tio);
		tcsetattr(0, TCSAFLUSH, &saved_tio);
		raise(SIGSTOP);
		tcgetattr(0, &saved_tio);
		tcsetattr(0, TCSAFLUSH, &tio);
		return;
	}
	tcsetattr(0, TCSAFLUSH, &saved_tio);
	raise(signum);
}

// Create a display context, run the game, and then cleanly exit.
int main(int argc, char* argv[])
{
	if ( !isatty(0) )
		error(1, errno, "standard input");
	if ( tcgetattr(0, &saved_tio) < 0 )
		error(1, errno, "tcsetattr: standard input");
	if ( atexit(restore_terminal_on_exit) != 0 )
		error(1, errno, "atexit");
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = restore_terminal_on_signal;
	sigaction(SIGTSTP, &sa, NULL);
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	if ( !dispd_initialize(&argc, &argv) )
		error(1, 0, "couldn't initialize dispd library");
	struct dispd_session* session = dispd_attach_default_session();
	if ( !session )
		error(1, 0, "couldn't attach to dispd default session");
	if ( !dispd_session_setup_game_rgba(session) )
		error(1, 0, "couldn't setup dispd rgba session");
	struct dispd_window* window = dispd_create_window_game_rgba(session);
	if ( !window )
		error(1, 0, "couldn't create dispd rgba window");

	mainloop(window);

	dispd_destroy_window(window);
	dispd_detach_session(session);

	return 0;
}
