#include <bn_core.h>
#include <bn_display.h>
#include <bn_log.h>
#include <bn_keypad.h>
#include <bn_random.h>
#include <bn_rect.h>
#include <bn_sprite_ptr.h>
#include <bn_sprite_text_generator.h>
#include <bn_size.h>
#include <bn_string.h>
#include <bn_vector.h>
#include <bn_backdrop.h>
#include <bn_color.h>
#include <cmath>

#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_square.h"
#include "common_fixed_8x16_font.h"
#include "bn_music_items.h"

// graphics for bird
#include "bn_sprite_items_boostingbird.h"
#include "bn_sprite_items_closewingbird.h"
#include "bn_sprite_items_openwingbird.h"

// graphics for pill
#include "bn_sprite_items_pill.h"
// For Enemy Sprite
#include "bn_sprite_items_enemy.h"
// bg sprite
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_cave.h"

// Pixels / Frame player moves at
// bn::fixed SPEED = 1.5;

// spawn locations for sprite & treasure
static constexpr int PLAYER_STARTING_X = 0;
static constexpr int PLAYER_STARTING_Y = 0;
static constexpr int TREASURE_STARTING_X = -40;
static constexpr int TREASURE_STARTING_Y = -40;
static constexpr int POWERUP_STARTING_X = 40;
static constexpr int POWERUP_STARTING_Y = -40;
static constexpr int ENEMY_STARTING_X = 60;
static constexpr int ENEMY_STARTING_Y = 40;

// Width and height of the the player and treasure bounding boxes
static constexpr bn::size PLAYER_SIZE = {8, 8};
static constexpr bn::size TREASURE_SIZE = {8, 8};
static constexpr bn::size POWERUP_SIZE = {8, 8};
static constexpr bn::size ENEMY_SIZE = {16,16}; 

// Full bounds of the screen
static constexpr int MIN_Y = -bn::display::height() / 2;
static constexpr int MAX_Y = bn::display::height() / 2;
static constexpr int MIN_X = -bn::display::width() / 2;
static constexpr int MAX_X = bn::display::width() / 2;

// Number of characters required to show the longest numer possible in an int (-2147483647)
static constexpr int MAX_SCORE_CHARS = 11;

// Score location
static constexpr int SCORE_X = 70;
static constexpr int SCORE_Y = -70;

// stamina location, simillar location to score
static constexpr int STAMINA_X = -100;
static constexpr int STAMINA_Y = -70;

//HUD ONE ROW ACROSS THE TOP each Starting at its X(text is left aligned)
static constexpr int HUD_Y = -70;
static constexpr int HUD_LIVES_X = -116;
static constexpr int HUD_TIME_X = -48;
static constexpr int HUD_BOOST_X= 20;
static constexpr int NUM_TREASURES = 3;
static constexpr int TREASURE_LIFETIME = 180; // 3 seconds

static constexpr int HIT_COOLDOWN = 60;

int main()
{
    bn::core::init();

    bn::random rng = bn::random();
    // int counter = 0;
    float SPEED = 1;
    // int boosts = 3;
    bool boostMode = false;

    // stamina bar feature
    // boost will last 240 frames (4 seconds) & take 8 seconds to recover
    const float MAX_STAMINA = 240;
    float stamina = MAX_STAMINA;
    float recovery_rate = 0.5;
    bool no_stamina = false;

    // Will hold the sprites for the score
    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> score_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> stamina_sprites = {};
    bn::sprite_text_generator text_generator(common::fixed_8x16_sprite_font);
    text_generator.set_left_alignment();//every text at the X we give it

    int lives = 3;
    bool roided = false;
    int hit_cooldown = 0; // frames left before enemy can hurt you again

    // Setting a 60s game timer
    int time_left_seconds = 60; // starts at 60s
    int frame_counter = 0;      // Tracks 60 fps
    bool game_over = false;     // stops the game when time runs out

    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> timer_sprites = {};
    // setting backdrop
    bn::regular_bg_ptr cave = bn::regular_bg_items::cave.create_bg(0, 0);

    //bird wings logic
    bn::sprite_ptr player = bn::sprite_items::openwingbird.create_sprite(PLAYER_STARTING_X, PLAYER_STARTING_Y);
    bn::vector<bn::sprite_ptr,NUM_TREASURES> treasures;
    bn::vector<int,NUM_TREASURES> treasure_timers;
    //spwan each treasure at a random spot with a full 3 sec timer
    for(int i=0; i < NUM_TREASURES; i++)
    {
        treasures.push_back(bn::sprite_items::dot.create_sprite(rng.get_int(MIN_X, MAX_X),
                                                                rng.get_int(MIN_Y, MAX_Y)));
        treasure_timers.push_back(TREASURE_LIFETIME);
    }
    bn::sprite_ptr powerup = bn::sprite_items::pill.create_sprite(POWERUP_STARTING_X, POWERUP_STARTING_Y);
    bn::sprite_ptr enemy = bn::sprite_items::enemy.create_sprite(ENEMY_STARTING_X, ENEMY_STARTING_Y);
    bn::string<16> costume = "open";

    // BACKGROUND MUSIC
    bn::music_items::somewher.play();

    while (true)
    {
        // Timer control if GAME OVER, FREEZE GAMEPLAY
        if (game_over)
        {
            // if start is pressed the timer variables reset to restart the game
            if (bn::keypad::start_pressed())
            {
                lives = 3;
                hit_cooldown = 0;
                SPEED = 1;
                time_left_seconds = 60;
                frame_counter = 0;
                game_over = false;
                enemy.set_position(ENEMY_STARTING_X,ENEMY_STARTING_Y);
                player.set_position(PLAYER_STARTING_X, PLAYER_STARTING_Y);
                for(int i= 0; i<NUM_TREASURES;i++)
                {
                    treasures[i].set_position(rng.get_int(MIN_X, MAX_X), rng.get_int(MIN_Y, MAX_Y));
                    treasure_timers[i]= TREASURE_LIFETIME;
                }
            }
            // Display message showing their final score
            timer_sprites.clear();
            text_generator.generate(-44, 0, "GAME OVER!!", timer_sprites);
            rng.update();
            bn::core::update();
            continue; // Skips the rest of the file so they cant score
        }
        // Move player with d-pad
        if (bn::keypad::left_held())
        {
            player.set_x(player.x() - SPEED);
            player.set_horizontal_flip(false);
        }
        if (bn::keypad::right_held())
        {
            player.set_x(player.x() + SPEED);
            player.set_horizontal_flip(true);
        }
        if (bn::keypad::up_held())
        {
            player.set_y(player.y() - SPEED);
        }
        if (bn::keypad::down_held())
        {
            player.set_y(player.y() + SPEED);
        }

        // Reset game state and player stats when the START button is pressed
        if (bn::keypad::start_pressed())
        {
            // Puts the player back at the start and respawn all treasures randomly
            player.set_position(PLAYER_STARTING_X, PLAYER_STARTING_Y);
            for (int i = 0; i < NUM_TREASURES; i++)
            {
                treasures[i].set_position(rng.get_int(MIN_X, MAX_X), rng.get_int(MIN_Y, MAX_Y));
                treasure_timers[i] = TREASURE_LIFETIME;
            }
            // Reset score, speed, and boost mechanics
            SPEED = 1;
            lives = 3;
            roided = false;
            powerup.set_visible(true);

            // Timer Reset
            time_left_seconds = 60;
            frame_counter = 0;
            game_over = false;

            // boosts = 0;
            // boostMode = false;
            // counter = 0;
        }

        if (bn::keypad::a_held() && !no_stamina)
        {
            SPEED = 2.5;
            stamina = stamina - 1;
            boostMode = true;
            player.set_item(bn::sprite_items::boostingbird);
            costume = "boosting";
        }
        if (bn::keypad::a_released() || stamina <= 0)
        {
            // revert speed back
            costume = "openwing";
            SPEED = 1;
            boostMode = false;
            if (stamina <= 0)
            {
                no_stamina = true;
                stamina = 0;
            }
        }
        if (!boostMode)
        {
            if (stamina < MAX_STAMINA)
            {
                stamina = stamina + recovery_rate;
            }
            else
            {
                stamina = MAX_STAMINA;
            }
            no_stamina = false;
        }

        // The bounding boxes of the player and powerup, snapped to integer pixel
        bn::rect player_rect = bn::rect(player.x().round_integer(),
                                        player.y().round_integer(),
                                        PLAYER_SIZE.width(),
                                        PLAYER_SIZE.height());

        bn::rect powerup_rect = bn::rect(powerup.x().round_integer(),
                                         powerup.y().round_integer(),
                                         POWERUP_SIZE.width(),
                                         POWERUP_SIZE.height());

        // Go through every treasure: count down its 3 second timer, check if the
        // player touched it, and move it to a new spot if it was touched or timed out
        for (int i = 0; i < NUM_TREASURES; i++)
        {
            treasure_timers[i]--; // treasure's own 3 second countdown
            //Build a box around this treasure so we can check if the player overlaps
            bn::rect treasure_rect = bn::rect(treasures[i].x().round_integer(),
                                      treasures[i].y().round_integer(),
                                      TREASURE_SIZE.width(),
                                      TREASURE_SIZE.height());
            bool touched = player_rect.intersects(treasure_rect);//player is on top of it
            bool expired = treasure_timers[i] <= 0; // 3 seconds ran out

            // move this treasure if the player hit it or its time ran out
            if (touched || expired)
            {
                treasures[i].set_position(rng.get_int(MIN_X, MAX_X), rng.get_int(MIN_Y, MAX_Y));
                treasure_timers[i] = TREASURE_LIFETIME;
            }

            //  Touching costs a life unless you have the powerup
            if (touched && !roided)
            {
                lives--;
            }
        }

        if (player_rect.intersects(powerup_rect))
        {
            powerup.set_visible(false);
            roided = true;
        }
        //Build a box around the enemy to check if player overlaps it
        bn::rect enemy_rect = bn::rect(enemy.x().round_integer(),
                                        enemy.y().round_integer(),
                                        ENEMY_SIZE.width(),
                                        ENEMY_SIZE.height());
        //count down the invincibility timer after a hit
        if(hit_cooldown > 0)
        {
            hit_cooldown--;
        }
        //Enemy touch costs a life, but only if we arent still invincible from recent hit
        if(player_rect.intersects(enemy_rect)&& hit_cooldown == 0)
        {
            lives--;
            hit_cooldown = HIT_COOLDOWN; // starts 1 sec invincibility
            if(lives <=0)
            {
                game_over = true;
            }
        }
        // looping char around screen boundaries
        if (player.x() > MAX_X)
        {
            player.set_position(MIN_X, player.y()); // loop from right edge to left edge
        }
        if (player.x() < MIN_X)
        {
            player.set_position(MAX_X, player.y()); // loop from left edge to right edge
        }
        if (player.y() > MAX_Y)
        {
            player.set_position(player.x(), MIN_Y); // loop from bottom edge to top edge
        }
        if (player.y() < MIN_Y)
        {
            player.set_position(player.x(), MAX_Y); // loop from top edge to bottom edge
        }

       // Update lives display
        bn::string<MAX_SCORE_CHARS> lives_string = "LIVES:" + bn::to_string<MAX_SCORE_CHARS>(lives);
        score_sprites.clear();
        text_generator.generate(HUD_LIVES_X, HUD_Y,
                                lives_string,
                                score_sprites);

        // update stamina display

        int int_stamina_percentage = stamina / MAX_STAMINA * 100;
        bn::string<MAX_SCORE_CHARS> stamina_string = bn::to_string<MAX_SCORE_CHARS>(int_stamina_percentage);
        stamina_sprites.clear();

        // call stamina boost cause stamina has too many chars
        bn::string stamina_text = "BOOST:" + stamina_string + "%";

        text_generator.generate(HUD_BOOST_X, HUD_Y,
                                stamina_text,
                                stamina_sprites);

        // Clock ticking down
        frame_counter++;
        if (frame_counter >= 60)
        {
            time_left_seconds--;
            frame_counter = 0;

            if (time_left_seconds <= 0 || lives <= 0)
            {
                time_left_seconds = 0;
                game_over = true; // This will trigger a freez block
            }
        }

        // updating graphics for player
        if (!boostMode && frame_counter >= 59)
        {
            if (costume == "openwing")
            {
                player.set_item(bn::sprite_items::closewingbird);
                costume = "closewing";
            }
            else
            {
                player.set_item(bn::sprite_items::openwingbird);
                costume = "openwing";
            }
        }

        bn::string<MAX_SCORE_CHARS> timer_string = "TIME:" + bn::to_string<MAX_SCORE_CHARS>(time_left_seconds);
        timer_sprites.clear();
        text_generator.generate(HUD_TIME_X,HUD_Y, timer_string, timer_sprites);
        // Update RNG seed every frame so we don't get the same sequence of positions every time
        rng.update();

        bn::core::update();
    }
}