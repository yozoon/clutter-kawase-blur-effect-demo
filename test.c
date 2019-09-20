#include <clutter/clutter.h>
#include <stdlib.h>
#include <cogl/cogl.h>

int main(int argc, char *argv[]) {
        clutter_init(&argc, &argv);

        ClutterColor stage_color = { 0, 0, 0, 255 };

        ClutterActor *stage = clutter_stage_get_default();
        clutter_actor_set_size(stage, 512, 512);
        clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);
        clutter_actor_show(stage);

        CoglContext *ctx = clutter_backend_get_cogl_context (clutter_get_default_backend ());
        CoglPipeline *pipeline = cogl_pipeline_new(ctx);

        clutter_main();

        return EXIT_SUCCESS;
}