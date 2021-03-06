/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "system.h"
#include "timer.h"
#include "graphics.h"
#include "vec_math.h"
#include "scene.h"
#include "ui.h"
#include "assert.h"

/* Defines
 */
#define NUM_LIGHTS 15

/* Types
 */
struct Game
{
    int width;
    int height;
    /* Engine objects */
    Timer*      timer;
    Graphics*   graphics;
    UI*         ui;

    /* Game objects */
    Transform   camera;
    Scene*      scene;
    Light       sun_light;
    Light       lights[NUM_LIGHTS];
    float       light_transform;
    int         dynamic_lights;

    /* Input */
    TouchPoint  points[16];
    int         num_points;
    Vec2        prev_single;
    Vec2        prev_double;
    float       tap_timer;

    /* FPS Counting */
    float       fps_time;
    int         fps_count;
    float       fps;
};

/* Constants
 */

/* Variables
 */

/* Internal functions
 */
static float _rand_float()
{
    return rand()/(float)RAND_MAX;
}
static void _control_camera(Game* G, float delta_time)
{
    if(G->num_points == 1) {
        Vec2 curr = G->points[0].pos;
        Vec2 delta = vec2_sub(curr, G->prev_single);

        /* L-R rotation */
        Quaternion q = quat_from_axis_anglef(0, 1, 0, delta_time*delta.x*0.2f);
        G->camera.orientation = quat_multiply(G->camera.orientation, q);

        /* U-D rotation */
        q = quat_from_axis_anglef(1, 0, 0, delta_time*delta.y*0.2f);
        G->camera.orientation = quat_multiply(q, G->camera.orientation);

        G->prev_single = curr;
    } else if(G->num_points == 2) {
        float camera_speed = 0.1f;
        Vec3 look = quat_get_z_axis(G->camera.orientation);
        Vec3 right = quat_get_x_axis(G->camera.orientation);
        Vec2 avg = vec2_add(G->points[0].pos, G->points[1].pos);
        Vec2 delta;

        avg = vec2_mul_scalar(avg, 0.5f);
        delta = vec2_sub(avg, G->prev_double);

        look = vec3_mul_scalar(look, -delta.y*camera_speed);
        right = vec3_mul_scalar(right, delta.x*camera_speed);


        G->camera.position = vec3_add(G->camera.position, look);
        G->camera.position = vec3_add(G->camera.position, right);

        G->prev_double = avg;
    }
}

/* External functions
 */
Game* create_game(void)
{
    int ii;
    Game* G = (Game*)calloc(1, sizeof(Game));
    G->timer = create_timer();
    G->graphics = create_graphics();
    G->ui = create_ui(G->graphics);

    /* Set up camera */
    G->camera = transform_zero;
    G->camera.orientation = quat_from_euler(0, -0.75f * kPi, 0);
    G->camera.position.x = 4.0f;
    G->camera.position.y = 2;
    G->camera.position.z = 7.5f;

    /* Load scene */
    reset_timer(G->timer);
    G->scene = create_scene("lightHouse.obj");
    G->sun_light.position = vec3_create(-4.0f, 5.0f, 2.0f);
    G->sun_light.color = vec3_create(1, 1, 1);
    G->sun_light.size = 35.0f;

    G->lights[0].color = vec3_create(1, 0, 0);
    G->lights[1].color = vec3_create(1, 1, 0);
    G->lights[2].color = vec3_create(0, 1, 0);
    G->lights[3].color = vec3_create(1, 0, 1);
    G->lights[4].color = vec3_create(0, 0, 1);
    G->lights[5].color = vec3_create(0, 1, 1);

    for(ii=0;ii<NUM_LIGHTS;++ii) {
        float x = (20.0f/NUM_LIGHTS) * ii - 8.0f;
        G->lights[ii].color = vec3_create(_rand_float(), _rand_float(), _rand_float());
        G->lights[ii].color = vec3_normalize(G->lights[ii].color);
        if(ii % 2)
            G->lights[ii].position = vec3_create(x, _rand_float()*3 + 2.0f, 0.0f);
        else
            G->lights[ii].position = vec3_create(0.0f, _rand_float()*3 + 2.0f, x);
        G->lights[ii].size = 5;
    }

    get_model(G->scene, 3)->material->specular_color = vec3_create(0.5f, 0.5f, 0.5f);
    get_model(G->scene, 3)->material->specular_coefficient = 1.0f;

    G->dynamic_lights = 1;

    reset_timer(G->timer);
    return G;
}
void destroy_game(Game* G)
{
    destroy_timer(G->timer);
    destroy_graphics(G->graphics);
    free(G);
}
void resize_game(Game* G, int width, int height)
{
    G->width = width;
    G->height = height;
    resize_graphics(G->graphics, width, height);
    resize_ui(G->ui, width, height);
}
void update_game(Game* G)
{
    float delta_time = (float)get_delta_time(G->timer);
    int ii;

    _control_camera(G, delta_time);
    set_view_matrix(G->graphics, mat4_inverse(transform_get_matrix(G->camera)));

    /* Dynamic Lights */
    if(G->dynamic_lights) {
        G->sun_light.position = mat3_mul_vector(vec3_create(5,5,0), mat3_rotation_y((float)get_running_time(G->timer)*0.5f));
        G->light_transform += delta_time;
        for(ii=0;ii<NUM_LIGHTS;++ii) {
            if(ii % 2)
                G->lights[ii].position.z = sinf((G->light_transform + ii * 1.0f)/2.0f) * 10.0f;
            else
                G->lights[ii].position.x = sinf((G->light_transform + ii * 1.0f)/2.0f) * 10.0f;
        }
    }

    add_light(G->graphics, G->sun_light);
    for(ii=0;ii<NUM_LIGHTS;++ii) {
        add_light(G->graphics, G->lights[ii]);
    }
    render_scene(G->scene, G->graphics);

    G->tap_timer += delta_time;

    /* Calculate FPS */
    G->fps_time += delta_time;
    G->fps_count++;

    if(G->fps_time >= 1.0f) {
        G->fps = G->fps_count/G->fps_time;
        system_log("FPS: %f\n", G->fps);
        G->fps_time -= 1.0f;
        G->fps_count = 0;
    }
    {
        int width, height;
        float scale = 50.0f;
        float x = -G->width/2.0f;
        float y = G->height/2.0f-scale;
        char buffer[256] = {0};
        // FPS
        sprintf(buffer, "FPS: %.2f", G->fps);
        add_string(G->ui, x, y, scale, buffer);
        y -= scale;
        // Renderer
        switch(renderer_type(G->graphics)) {
        case kForward: add_string(G->ui, x, y, scale, "Forward renderer"); break;
        case kLightPrePass: add_string(G->ui, x, y, scale, "Deferred Lighting"); break;
        case kDeferred: add_string(G->ui, x, y, scale, "Deferred Shading"); break;
        default:
                system_log("Invalid renderer");
                assert(0);
                break;
        }
        y -= scale;
        // Resolution
        graphics_size(G->graphics, &width, &height);
        sprintf(buffer, "%dx%d", width, height);
        add_string(G->ui, x, y, scale, buffer);
    }
}
void render_game(Game* G)
{
    render_graphics(G->graphics);
    draw_ui(G->ui);
}
void add_touch_points(Game* G, int num_touch_points, TouchPoint* points)
{
    int ii;
    for(ii=0;ii<num_touch_points;++ii) {
        G->points[G->num_points++] = points[ii];
    }

    if(G->num_points == 1) {
        G->prev_single = G->points[0].pos;
        G->tap_timer = 0.0f;
    } else if(G->num_points == 2) {
        Vec2 avg = vec2_add(G->points[0].pos, G->points[1].pos);
        avg = vec2_mul_scalar(avg, 0.5f);
        G->prev_double = avg;
    }
}
void update_touch_points(Game* G, int num_touch_points, TouchPoint* points)
{
    int ii, jj;
    for(ii=0;ii<G->num_points;++ii) {
        for(jj=0;jj<num_touch_points;++jj) {
            if(G->points[ii].index == points[jj].index) {
                G->points[ii] = points[jj];
                break;
            }
        }
    }
}
void remove_touch_points(Game* G, int num_touch_points, TouchPoint* points)
{
    int orig_num_points = G->num_points;
    int ii, jj;
    for(ii=0;ii<orig_num_points;++ii) {
        for(jj=0;jj<num_touch_points;++jj) {
            if(G->points[ii].index == points[jj].index) {
                /* This is the removed touch, swap it with the end of the list */
                G->points[ii] = G->points[--G->num_points];
                break;
            }
        }
    }

    if(G->num_points == 1) {
        G->prev_single = G->points[0].pos;
    } else if(G->num_points == 2) {
        Vec2 avg = vec2_add(G->points[0].pos, G->points[1].pos);
        avg = vec2_mul_scalar(avg, 0.5f);
        G->prev_double = avg;
    } else {
        if(G->tap_timer < 0.5f) {
            if(G->prev_single.x < G->width/2) {
                if(G->prev_single.y < G->height/2) { // Top Left
                    cycle_renderers(G->graphics);
                } else { // bottom left
                    G->dynamic_lights = !G->dynamic_lights;
                }

            } else {
                if(G->prev_single.y < G->height/2) { // Top right
                } else { // bottom right
                }
            }
        }
    }
}
