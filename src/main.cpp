#include "app/ApplicationController.h"

static ApplicationController app;

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
