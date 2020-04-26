#include "SocketMulticast.h"
#include "SocketDatagrama.h"
#include <ctime>

int main(int argc, char const *argv[])
{
    
    if (argc != 5)
    {
        printf("Uso: ./%s [DIRECCIÓN MULTICAST] [PUERTO PARA ENVIAR] [DEPÓSITOS] [MIEMBROS]\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    // Se planta semilla para números aleatorios.
    //srand(getpid());


    // Extracción de parámetros.
    char direccionMulticast[16];
    sprintf(direccionMulticast, "%s", argv[1]);
    int puertoTransmision = atoi(argv[2]);
    unsigned char ttl = (unsigned char)1;
    int depositos = atoi(argv[3]);
    int miembros = atoi(argv[4]);
    srand(time(NULL));
    // Se abre socket .
    SocketMulticast socketMulticast(puertoTransmision);
    for (int i = 0; i < depositos; i++)
    {
        // Se prepara depósito.
        int deposito = rand() % 9 + 1;
        printf("Se depositarán $ %d\n", deposito);

        // Se prepara mensaje que llevará el depósito.
        mensaje m = {TRANSMIT, i};
        memcpy(m.args, (void *)&deposito, sizeof(deposito));
        PaqueteDatagrama pd((char *)&m, sizeof(mensaje), direccionMulticast, puertoTransmision);

        // Se envía el mensaje con depósito.
        if (socketMulticast.enviaConfiable(pd, ttl, miembros) < 0)
        {
            printf("Error en depósito %d de $%d.\n", i, deposito);
            exit(0);
        }
    }
    return 0;
}
