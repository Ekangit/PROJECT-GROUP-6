
#pragma once
#include <lvgl.h>



// Ritar en väderikon i en ny container med size x size under parent.
// Returnerar container-objektet så att du kan positionera/styla i efterhand.
lv_obj_t* weather_icon_create(lv_obj_t* parent, int wsymb2, int size);

