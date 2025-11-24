// Ponto de entrada da aplicação
// Cria e executa a aplicação principal com configuração padrão

#include "application.h"

int main()
{
    ApplicationConfig config;
    Application app(config);
    return app.Run();
}
