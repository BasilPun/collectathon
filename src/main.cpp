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
#include <bn_backdrop.h>
#include <bn_color.h>
#include <cmath>

#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_square.h"
#include "common_fixed_8x16_font.h"
#include "bn_music_items.h"


// Pixels / Frame player moves at
// bn::fixed SPEED = 1.5;

// spawn locations for sprite & treasure
static constexpr int PLAYER_STARTING_X = 0;
static constexpr int PLAYER_STARTING_Y = 0;
static constexpr int TREASURE_STARTING_X = -40;
static constexpr int TREASURE_STARTING_Y = -40;

// Width and height of the the player and treasure bounding boxes
static constexpr bn::size PLAYER_SIZE = {8, 8};
static constexpr bn::size TREASURE_SIZE = {8, 8};

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

    int score = 0;

    //Setting a 60s game timer
    int time_left_seconds = 60; //starts at 60s
    int frame_counter = 0;  //Tracks 60 fps
    bool game_over = false; //stops the game when time runs out

    bn::vector<bn::sprite_ptr,MAX_SCORE_CHARS> timer_sprites ={};
    // setting backdrop
    bn::backdrop::set_color(bn::color(0, 6, 20)); // Color Blue

    bn::sprite_ptr player = bn::sprite_items::square.create_sprite(-50, 50);
    bn::sprite_ptr treasure = bn::sprite_items::dot.create_sprite(0, 0);

    // spawn location, we will be spawning our char in the middle
    // of the screen & spawning starting trasure at top left of char
    player.set_position(PLAYER_STARTING_X, PLAYER_STARTING_Y);
    treasure.set_position(TREASURE_STARTING_X, TREASURE_STARTING_Y);

    //background music 
    bn::music_items::somewher.play();

    while (true)
    {
        if(game_over)
    {
        if(bn::keypad::start_pressed()){
            score=0;
            SPEED =1;
            time_left_seconds = 60;
            frame_counter =0;
            game_over = false;
            player.set_position(PLAYER_STARTING_X,PLAYER_STARTING_Y);
            treasure.set_position(TREASURE_STARTING_X,TREASURE_STARTING_Y);
        }
        //Display message showing their final score
        timer_sprites.clear();
        text_generator.generate(0,0,"TIME'S UP!!", timer_sprites);
        rng.update();
        bn::core::update();
        continue;// Skips the rest of the file so they cant score
    }
        // Move player with d-pad
        if (bn::keypad::left_held())
        {
            player.set_x(player.x() - SPEED);
        }
        if (bn::keypad::right_held())
        {
            player.set_x(player.x() + SPEED);
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
            // Reset positions of player and treasure to starting defaults
            player.set_position(PLAYER_STARTING_X, PLAYER_STARTING_Y);
            treasure.set_position(TREASURE_STARTING_X, TREASURE_STARTING_Y);

            // Reset score, speed, and boost mechanics
            score = 0;
            SPEED = 1;

            // boosts = 0;
            // boostMode = false;
            // counter = 0;
        }

        if (bn::keypad::a_held() && !no_stamina)
        {
            SPEED = 2.5;
            stamina = stamina - 1;
            boostMode = true;
        }
        if (bn::keypad::a_released() || stamina <= 0)
        {
            // revert speed back
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
        // if (boosts != 0 && boostMode == false)
        // {
        //     if (bn::keypad::a_pressed())
        //     {
        //         // if A pressed, increase speed & change bg color
        //         SPEED = 2.5;
        //         bn::backdrop::set_color(bn::color(15, 0, 0));//Color Red
        //         boosts--;
        //         boostMode = true;
        //     }
        // }

        // // so that counter doesn't constantly count and waste compute
        // if (boostMode)
        // {
        //     counter++;
        //     // since 60fps, 180 frames = 3 second boost
        //     if (counter >= 180)
        //     {

        //         bn::backdrop::set_color(bn::color(0, 6, 20));//Comes back to blue after boost
        //         SPEED = 1.5;
        //         counter = 0;
        //         boostMode = false;
        //     }
        // }

        // The bounding boxes of the player and treasure, snapped to integer pixels
        bn::rect player_rect = bn::rect(player.x().round_integer(),
                                        player.y().round_integer(),
                                        PLAYER_SIZE.width(),
                                        PLAYER_SIZE.height());
        bn::rect treasure_rect = bn::rect(treasure.x().round_integer(),
                                          treasure.y().round_integer(),
                                          TREASURE_SIZE.width(),
                                          TREASURE_SIZE.height());

        // If the bounding boxes overlap, set the treasure to a new location an increase score
        if (player_rect.intersects(treasure_rect))
        {
            // Jump to any random point in the screen
            int new_x = rng.get_int(MIN_X, MAX_X);
            int new_y = rng.get_int(MIN_Y, MAX_Y);
            treasure.set_position(new_x, new_y);

            score++;
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

        // Update score display
        bn::string<MAX_SCORE_CHARS> score_string = bn::to_string<MAX_SCORE_CHARS>(score);
        score_sprites.clear();
        text_generator.generate(SCORE_X, SCORE_Y,
                                score_string,
                                score_sprites);

        // update stamina display

        int int_stamina_percentage = stamina / MAX_STAMINA * 100;
        bn::string<MAX_SCORE_CHARS> stamina_string = bn::to_string<MAX_SCORE_CHARS>(int_stamina_percentage);
        stamina_sprites.clear();

        // call stamina boost cause stamina has too many chars
        bn::string stamina_text = "BOOST:" + stamina_string + "%";

        text_generator.generate(STAMINA_X, STAMINA_Y,
                                stamina_text,
                                stamina_sprites);
        // Clock ticking down
        frame_counter++;
        if(frame_counter>=60)
        {
            time_left_seconds--;
            frame_counter =0;
            if(time_left_seconds<=0)
            {
                time_left_seconds =0;
                game_over = true; // This will trigger a freez block
            }
        }
        bn::string<MAX_SCORE_CHARS>timer_string="TIME:"+bn::to_string<MAX_SCORE_CHARS>(time_left_seconds);
        timer_sprites.clear();
        text_generator.generate(0,-70, timer_string, timer_sprites);
        // Update RNG seed every frame so we don't get the same sequence of positions every time
        rng.update();

        bn::core::update();
    }
}