# ecs_pong
This example aims to be an extremely compact implementation (150 lines!) of the pong game in the [flecs ECS framework](https://github.com/SanderMertens/flecs). The purpose is to demonstrate how flecs modules can be used to quickly create an actually working 2D game that handles user input, physics and rendering.

Note that flecs as well as the flecs modules are still under heavy development. If you are looking to use this for your own game, expect to find some bugs!

## Getting started
Flecs uses the bake build system (https://github.com/SanderMertens/bake). To install bake on Linux and MacOS, do:

```
git clone https://github.com/SanderMertens/bake
make -C bake/build-$(uname)
bake/bake setup
```

Before you can run the demo, make sure to have SDL2 installed.

On MacOS:

```
brew install sdl2
```

On Debian/Ubuntu:

```
sudo apt-get install sdl2
```

To install and run the demo, do:

```
bake clone SanderMertens/ecs_pong
bake run ecs_pong
```

## Implementation
This demo heavily leverages the flecs modules to keep the implementation compact. Everything related to rendering, collision detection, capturing user input, transformations and running systems is done by either flecs modules or the flecs framework. As a result, the game code itself only deals with the game logic, like moving the paddles and ball, running the (extremely simple) AI and handling user input.

The main routine of the demo declaratively sets up the scene with its components, systems and entities. A few things are noteworthy. To begin with, the paddles are defined using a prefab. The prefab serves three purposes. First of all, since both paddles share the same geometry, the demo uses a prefab to store this data once. This causes the `EcsRectangle` component to be shared between paddles.

Secondly, the prefab contains the `Target` component. This component captures the "goal" of where a player wants to move to. In case of a human player, the target is set whenever one of the movement keys is pressed. In case of the AI, the target is set to where it wants its paddle to be when it hits the ball. Obviously, this target is specific for each player, so why specify it on the prefab? The reason for this is that this is an easy method for initializing the component. Note that the `Player` and `AI` entities immediately override the `Target` component in their declaration. This will copy the initialized prefab value to the private components.

Thirdly, the prefab has the `EcsCollision` tag. This tag is used by the physics system to automatically add collider components, based on the geometry of the entity. For more complex shapes you may want to define your own colliders, but for rectangles and circles it is much more convenient to let the physics system do it for us. By specifying the `EcsCollision` tag on the prefab, we ensure that the collider components get added to both paddle entities.

Another thing you may notice in the code is the system component expressions. In flecs, these expressions are like a function argument list. They contain everything the system needs to know to be able to run. Some elements in these expressions are straightforward, like `EcsPosition2D`, which selects the `EcsPosition2D` component from entities. 

Other elements are more interesting, like `Player.Target`. This element will actually pass the `Target` component from the `Player` entity into the system as a _shared component_. This is a really convenient way of letting flecs resolve the components for you, so you don't have to do this in your system. Another thing you may notice is the `!PaddlePrefab` expression. This tells flecs to exclude any entity that is created with the `PaddlePrefab` prefab.

Finally, you'll notice the `ecs_set_singleton` function. This lets you set a singleton component, without having to create an entity. This serves the purpose of creating the drawing canvas well, since we will only ever create one.

### Collision detection
The most interesting part of the demo is its usage of the collision detection system. I already mentioned the `EcsCollider` component, which automatically adds the appropriate collider component to our entities based on its geometry. When an entity has a collider, it will automatically be evaluated by the physics system. One thing that is critical in particular in this demo is that collisions are handled within the same frame in that they are detected. To appreciate how this works, a better understanding of how systems are ordered is required.

In flecs, systems are assigned to a "stage". The most common stage out there is `EcsOnFrame`. This is the main stage where most of the game logic happens. The collision detection routines happen in an `EcsPostFrame` stage, which happens directly after `EcsOnFrame`. When the collision system detects a collision, it will create a new entity with an `EcsCollision` component. The `Collision` system in the pong application subscribes for this component. To ensure that the collision is properly handled within the same frame, the system uses the `EcsOnSet` mode. This will ensure that the system is executed as soon as an `EcsCollision` comopnent is assigned to an entity.

## AI
The artificial intelligence in the game is extremely simple, but surprisingly effective. The AI will look at the current location of the human player, and based on that aim for the opposite corner. It works well in most cases, though it is not smart enough to handle the bounding off the walls. Even with that limitation, the AI is very accurate at hitting the ball with the edge of the paddle when it is given the chance.

## System API
You may notice that systems use different functions (`ecs_column`, `ecs_field`) to access their arguments. The `ecs_column` function can be used when the application is sure that the component is "owned" by an entity. By owned, I mean that the component is added to the entity itself, and does not come from a prefab or equivalent. The `ecs_column` function essentially gives the system a raw array to iterate over, which is extremely fast.

The `ecs_field` function provides a convenient way to abstract away from the differences between shared components and owned components. When a system uses `ecs_field`, it can be written in a way that is agnostic to how the components are stored in memory, at the cost of some performance (though not a lot). Flecs lets you test whether a component is shared or not, and thus it is theoretically possible to support versions of a system optimized for each usecase. For this demo though, performance is more than sufficient.

## Optimizing the critical path
While the ability to quickly import modules and leverage existing functionality is great, you also potentially load some systems that the application may not require. For this demo for example, there are a whole lot of them, from rotation shapes (not used), to velocity based movements (entities don't have velocity) and things like custom camera's.

To prevent these unused systems from hogging CPU cycles, flecs removes them from the main loop completely. It does this by automatically enabling/disabling systems based on how many entities they match with. If a system matches with 0 entities, it will be disabled will not be evaluated in the main loop. This ensures that you only pay for what you need, without having to explicitly enable/disable systems in your application.

## Conclusion
Hopefully this demo gives you a good idea of what we are aiming to accomplish with flecs: build a fast, modular and easy to use ECS framework. Flecs makes it easy to only use the functionality that you need. You can simply omit the modules you need (like rendering) and implement your own framework. Our ideal scenario is that you use flecs modules for prototyping, and _if_ you hit a wall in either capabilities or performance, it is easy to swap one module out for a more customized implementation.

If you like what you see, consider giving [Flecs a star](https://github.com/SanderMertens/flecs), or better yet, help us build it by contributing code and filing issues!
