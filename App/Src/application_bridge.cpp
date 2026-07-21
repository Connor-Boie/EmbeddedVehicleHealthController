#include "application_bridge.h"

#include "Application.hpp"

namespace
{
Application application;
}

extern "C" void application_init(void)
{
    application.initialize();
}

extern "C" void application_run(void)
{
    application.run();
}
