#include <include/ecs_pong.h>

#define BALL_RADIUS (10)
#define PLAYER_HEIGHT (15)
#define PLAYER_WIDTH (100)
#define PLAYER_SPEED (6.0)
#define PADDLE_AIM_C (1.5)
#define BALL_SPEED (400.0)
#define BALL_SERVE_SPEED (BALL_SPEED / 3.0)
#define BALL_BOOST (0.5)
#define COURT_WIDTH (400)
#define COURT_HEIGHT (300)

typedef float Target;

void compute_bounce(
    EcsPosition2D *p_ball,
    EcsPosition2D *p_player,
    EcsVelocity2D *v_ball)
{
    double angle = PADDLE_AIM_C * (p_ball->x - p_player->x) / PLAYER_WIDTH;
    float abs_angle = fabs(angle);

    v_ball->x = sin(angle) * BALL_SPEED;
    v_ball->y = cos(angle) * BALL_SPEED;
    
    if (abs_angle > 0.6) {
        v_ball->x *= (1 + abs_angle * BALL_BOOST);
        v_ball->y *= (1 + abs_angle * BALL_BOOST);
    }

    if (p_ball->y < p_player->y) {
        v_ball->y *= -1;
    }
}

void PlayerInput(EcsRows *rows) {
    EcsInput *input = ecs_field(rows, EcsInput, 0, 1);
    Target *target = ecs_field(rows, Target, 0, 2);

    if (input->keys[ECS_KEY_A].state || input->keys[ECS_KEY_LEFT].state) {
        *target = -PLAYER_SPEED;
    } else if (input->keys[ECS_KEY_D].state || input->keys[ECS_KEY_RIGHT].state) {
        *target = PLAYER_SPEED;
    } else {
        *target = 0;
    }
}

void AiThink(EcsRows *rows) {
    EcsPosition2D *ball_pos = ecs_field(rows, EcsPosition2D, 0, 1);
    EcsPosition2D *player_pos = ecs_field(rows, EcsPosition2D, 0, 2);
    EcsPosition2D *ai_pos = ecs_field(rows, EcsPosition2D, 0, 3);
    float target_x = ball_pos->x + (player_pos->x > 0
        ? (float)PLAYER_WIDTH / 2.5 + BALL_RADIUS
        : -(float)PLAYER_WIDTH / 2.5 + BALL_RADIUS );

    *ecs_field(rows, Target, 0, 4) = target_x - ai_pos->x;
}

void MovePaddle(EcsRows *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    Target *target = ecs_column(rows, Target, 2);

    for (int i = rows->begin; i < rows->end; i ++) {
        float abs_target = fabs(target[i]);
        float dir = abs_target / target[i];
        p[i].x += (abs_target > PLAYER_SPEED) ? PLAYER_SPEED * dir : target[i];
        ecs_clamp(&p[i].x, -COURT_WIDTH, COURT_WIDTH);
    }
}

void Collision(EcsRows *rows) {
    EcsCollision2D *c = ecs_column(rows, EcsCollision2D, 1);
    EcsType TEcsPosition2D = ecs_column_type(rows, 2);
    EcsPosition2D *p_ball = ecs_shared(rows, EcsPosition2D, 2);
    EcsVelocity2D *v_ball = ecs_shared(rows, EcsVelocity2D, 3);

    for (int i = rows->begin; i < rows->end; i ++) {
        EcsPosition2D *p_player = ecs_get_ptr(rows->world, c[i].entity_2, EcsPosition2D);
        p_ball->y += c[i].normal.y * c[i].distance;
        compute_bounce(p_ball, p_player, v_ball);
    }
}

void BounceWalls(EcsRows *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);

    for (int i = rows->begin; i < rows->end; i ++) {
        if (ecs_clamp(&p[i].x, -COURT_WIDTH + BALL_RADIUS, COURT_WIDTH - BALL_RADIUS)) {
            v->x *= -1.0;
        }

        int side = ecs_clamp(&p[i].y, -COURT_HEIGHT, COURT_HEIGHT);
        if (side)  {
            p[i] = (EcsPosition2D){0, 0};
            v[i] = (EcsVelocity2D){0, BALL_SERVE_SPEED * -side};
        }
    }
}

int main(int argc, char *argv[]) {
    EcsWorld *world = ecs_init();

    ECS_IMPORT(world, EcsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, EcsComponentsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGeometry, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGraphics, ECS_2D);
    ECS_IMPORT(world, EcsComponentsInput, ECS_2D);
    ECS_IMPORT(world, EcsSystemsSdl2, ECS_2D);
    ECS_IMPORT(world, EcsSystemsTransform, ECS_2D);
    ECS_IMPORT(world, EcsSystemsPhysics, ECS_2D);

    /* Register target component and paddle prefab */
    ECS_COMPONENT(world, Target);
    ECS_PREFAB(world, PaddlePrefab, EcsRectangle, Target, EcsCollider);
    ecs_set(world, PaddlePrefab, EcsRectangle, {.width = PLAYER_WIDTH, .height = PLAYER_HEIGHT});
    ecs_set(world, PaddlePrefab, Target, {0});
    
    /* Create game entities, add them to canvas */
    ECS_ENTITY(world, Ball, EcsCollider);
    ECS_ENTITY(world, Player, PaddlePrefab, Target);
    ECS_ENTITY(world, AI, PaddlePrefab, Target);

    /* Define game systems */
    ECS_SYSTEM(world, PlayerInput, EcsOnFrame, EcsInput, Player.Target);
    ECS_SYSTEM(world, AiThink, EcsOnFrame, EcsPosition2D, Player.EcsPosition2D, AI.EcsPosition2D, AI.Target, !PaddlePrefab);
    ECS_SYSTEM(world, BounceWalls, EcsOnFrame, EcsPosition2D, EcsVelocity2D, !PaddlePrefab);
    ECS_SYSTEM(world, MovePaddle, EcsOnFrame, EcsPosition2D, Target, PaddlePrefab);
    ECS_SYSTEM(world, Collision, EcsOnSet, EcsCollision2D, Ball.EcsPosition2D, Ball.EcsVelocity2D);

    /* Initialize starting positions and ball velocity */
    ecs_set(world, Ball, EcsPosition2D, {0, 0});
    ecs_set(world, Ball, EcsVelocity2D, {0, BALL_SERVE_SPEED});
    ecs_set(world, Ball, EcsCircle, {.radius = BALL_RADIUS});
    ecs_set(world, Player, EcsPosition2D, {0, 250});
    ecs_set(world, AI, EcsPosition2D, {0, -250});

    /* Create drawing canvas by creating EcsCanvas2D component */
    ecs_set_singleton(world, EcsCanvas2D, {.window = {.width = 800, .height = 600}});

    /* Main loop */
    ecs_set_target_fps(world, 60);
    while (ecs_progress(world, 0)) { }

    return ecs_fini(world);
}
