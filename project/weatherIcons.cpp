
// weather_icons.cpp
#include <lvgl.h>
#include "weatherIcons.hpp"

// Liten hjälpare: slå av alla streck/effekter som temat kan lägga på objekt
static inline void no_stroke(lv_obj_t* o) {
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_outline_width(o, 0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    // Ingen gradient som kan ge “band”
    #ifdef LV_GRAD_DIR_NONE
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_NONE, 0);
    #endif
}

// ---------------------------------------------------------------
// Hjälpfunktioner (behålls som static i .cpp)
// Små justeringar i storlekar/positioner för att undvika overflow
// ---------------------------------------------------------------

// Sun Icon Helper
static void draw_sun(lv_obj_t *parent, int size, int color_hex) {
    // Dra ner solens diameter en aning för säker marginal
    int d = (int)(size * 0.95);
    lv_obj_t *sun = lv_obj_create(parent);
    no_stroke(sun);
    lv_obj_set_size(sun, (lv_coord_t)d, (lv_coord_t)d);
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sun, lv_color_hex(color_hex), 0);
    lv_obj_set_style_bg_opa(sun, LV_OPA_COVER, 0);
    lv_obj_align(sun, LV_ALIGN_CENTER, 0, 0);
}

// Cloud Icon Helper
static void draw_cloud(lv_obj_t *parent, int size, int color_hex) {
    // Lite mer överlapp samt något mindre diameter för att undvika små glipor
    lv_obj_t *c1 = lv_obj_create(parent);
    no_stroke(c1);
    lv_obj_set_size(c1, (lv_coord_t)(size * 0.58), (lv_coord_t)(size * 0.58)); // var 0.6
    lv_obj_set_style_radius(c1, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c1, lv_color_hex(color_hex), 0);
    lv_obj_set_style_bg_opa(c1, LV_OPA_COVER, 0);
    lv_obj_align(c1, LV_ALIGN_CENTER, 0, 1);

    lv_obj_t *c2 = lv_obj_create(parent);
    no_stroke(c2);
    lv_obj_set_size(c2, (lv_coord_t)(size * 0.42), (lv_coord_t)(size * 0.42)); // var 0.4
    lv_obj_set_style_radius(c2, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c2, lv_color_hex(color_hex), 0);
    lv_obj_set_style_bg_opa(c2, LV_OPA_COVER, 0);
    lv_obj_align(c2, LV_ALIGN_CENTER, (lv_coord_t)(-size * 0.30), 4);

    lv_obj_t *c3 = lv_obj_create(parent);
    no_stroke(c3);
    lv_obj_set_size(c3, (lv_coord_t)(size * 0.47), (lv_coord_t)(size * 0.47)); // var 0.45
    lv_obj_set_style_radius(c3, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c3, lv_color_hex(color_hex), 0);
    lv_obj_set_style_bg_opa(c3, LV_OPA_COVER, 0);
    lv_obj_align(c3, LV_ALIGN_CENTER, (lv_coord_t)(size * 0.28), 3);

    lv_obj_t *c4 = lv_obj_create(parent);
    no_stroke(c4);
    lv_obj_set_size(c4, (lv_coord_t)(size * 0.72), (lv_coord_t)(size * 0.32));  // var 0.7/0.3
    lv_obj_set_style_radius(c4, 10, 0);
    lv_obj_set_style_bg_color(c4, lv_color_hex(color_hex), 0);
    lv_obj_set_style_bg_opa(c4, LV_OPA_COVER, 0);
    lv_obj_align(c4, LV_ALIGN_CENTER, 0, 7);  // var 8
}

// Rain Icon Helper
static void draw_rain(lv_obj_t *parent, int size) {
    for (int i = 0; i < 3; i++) {
        lv_obj_t *drop = lv_obj_create(parent);
        no_stroke(drop);
        lv_obj_set_size(drop, 4, 10);
        lv_obj_set_style_bg_color(drop, lv_color_hex(0x209CEE), 0);
        lv_obj_set_style_bg_opa(drop, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(drop, 2, 0);
        lv_obj_align(drop, LV_ALIGN_CENTER, (i - 1) * 8, size / 2);
    }
}

// Lightning Icon Helper
static void draw_lightning(lv_obj_t *parent, int /*size*/) {
    lv_obj_t *l1 = lv_obj_create(parent);
    no_stroke(l1);
    lv_obj_set_size(l1, 5, 18);
    lv_obj_set_style_bg_color(l1, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_bg_opa(l1, LV_OPA_COVER, 0);

    // Rotation kan öka bounding box — vår ikoncontainer har marginal,
    // men vill du testa utan, sätt 0.
    // lv_obj_set_style_transform_angle(l1, 0, 0);
    lv_obj_set_style_transform_angle(l1, 300, 0); // 30.0 deg (v8); ok att lämna även i v7—har bara ingen effekt

    lv_obj_align(l1, LV_ALIGN_CENTER, 0, 5);
}

// Snow Icon Helper
static void draw_snow(lv_obj_t *parent, int size) {
    for (int i = 0; i < 3; i++) {
        lv_obj_t *flake = lv_obj_create(parent);
        no_stroke(flake);
        lv_obj_set_size(flake, 6, 6);
        lv_obj_set_style_bg_color(flake, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(flake, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(flake, 3, 0);
        lv_obj_align(flake, LV_ALIGN_CENTER, (i - 1) * 10, size / 2);
    }
}

// Sleet Icon Helper
static void draw_sleet(lv_obj_t *parent, int size) {
    for (int i = 0; i < 2; i++) {
        lv_obj_t *drop = lv_obj_create(parent);
        no_stroke(drop);
        lv_obj_set_size(drop, 4, 10);
        lv_obj_set_style_bg_color(drop, lv_color_hex(0x209CEE), 0);
        lv_obj_set_style_bg_opa(drop, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(drop, 2, 0);
        lv_obj_align(drop, LV_ALIGN_CENTER, (i - 1) * 12 - 4, size / 2);
    }
    for (int i = 0; i < 2; i++) {
        lv_obj_t *flake = lv_obj_create(parent);
        no_stroke(flake);
        lv_obj_set_size(flake, 6, 6);
        lv_obj_set_style_bg_color(flake, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(flake, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(flake, 3, 0);
        lv_obj_align(flake, LV_ALIGN_CENTER, (i - 1) * 12 + 4, (size / 2) + 2);
    }
}

// ---------------------------------------------------------------
// Intern huvudrenderare
// ---------------------------------------------------------------
static void draw_weather_icon(lv_obj_t *parent, int s, int size) {
    if (s == 1) {
        draw_sun(parent, (int)(size * 0.75), 0xFFD700);

    } else if (s >= 2 && s <= 4) {
        draw_sun(parent, (int)(size * 0.72), 0xFFD700);

        lv_obj_t *c = lv_obj_create(parent);
        no_stroke(c);
        lv_obj_set_size(c, (lv_coord_t)size, (lv_coord_t)size);
        lv_obj_set_style_bg_opa(c, LV_OPA_0, 0);
        lv_obj_align(c, LV_ALIGN_CENTER, 8, 8);
        draw_cloud(c, size, 0xFFFFFF);

    } else if (s >= 5 && s <= 7) {
        draw_cloud(parent, size, 0xBBBBBB);

    } else if ((s >= 8 && s <= 10) || (s >= 18 && s <= 20)) {
        draw_cloud(parent, size, 0x888888);
        draw_rain(parent, size);

    } else if (s == 11 || s == 21) {
        draw_cloud(parent, size, 0x555555);
        draw_rain(parent, size);
        draw_lightning(parent, size);

    } else if ((s >= 12 && s <= 14) || (s >= 22 && s <= 24)) {
        draw_cloud(parent, size, 0x888888);
        draw_sleet(parent, size);

    } else if ((s >= 15 && s <= 17) || (s >= 25 && s <= 27)) {
        draw_cloud(parent, size, 0xBBBBBB);
        draw_snow(parent, size);

    } else {
        lv_obj_t *l = lv_label_create(parent);
        no_stroke(l);
        lv_label_set_text(l, "?");
        lv_obj_center(l);
    }
}

// ---------------------------------------------------------------
// Publikt API – skapa ikoncontainer med liten marginal
// ---------------------------------------------------------------
lv_obj_t* weather_icon_create(lv_obj_t *parent, int wsymb2, int size) {
    // Lägg 2–4 px marginal runt ikonen för att säkert undvika overflow/scroll
    const int margin = 4;
    lv_obj_t *cont = lv_obj_create(parent);

    // Sätt neutral stil: ingen bakgrund, inga linjer, ingen skugga
    no_stroke(cont);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);

    // Sätt storlek med marginal – ikonen ritas innanför
    lv_obj_set_size(cont, (lv_coord_t)(size + margin), (lv_coord_t)(size + margin));
    lv_obj_center(cont);

    // (v8) försök stänga av scroll-flaggor om de finns; oskadligt om de inte finns
    #if LVGL_VERSION_MAJOR >= 8
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    #endif

    // Rita själva ikonen
    draw_weather_icon(cont, wsymb2, size);

    return cont;
}

// ---------------------------------------------------------------
// Valfri uppdateringsfunktion – rita om i befintlig container
// ---------------------------------------------------------------
void weather_icon_update(lv_obj_t* container, int wsymb2, int size) {
    if (!container) return;
    // Rensa alla barn
    while (lv_obj_get_child_cnt(container) > 0) {
        lv_obj_del(lv_obj_get_child(container, 0));
    }
    // Sätt storlek med samma marginal som create()
    const int margin = 4;
    lv_obj_set_size(container, (lv_coord_t)(size + margin), (lv_coord_t)(size + margin));
    no_stroke(container);
    lv_obj_set_style_bg_opa(container, LV_OPA_0, 0);

    #if LVGL_VERSION_MAJOR >= 8
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    #endif

    draw_weather_icon(container, wsymb2, size);
}
