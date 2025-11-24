#include "application.h"

int main()
{
    ApplicationConfig config;
    Application app(config);
    return app.Run();
}
