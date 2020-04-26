#include "SocketMulticast.h"

SocketMulticast::SocketMulticast(int puerto)
{
    ultimoId = -1;
    s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int reuse = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
    {
        printf("Error al llamar a la función setsockopt\n");
        exit(0);
    }

    sockaddr_in direccionLocal;
    int len = sizeof(direccionLocal);
    bzero(&direccionLocal, len);
    direccionLocal.sin_family = AF_INET;
    direccionLocal.sin_addr.s_addr = INADDR_ANY;
    direccionLocal.sin_port = htons(puerto);

    bind(s, (sockaddr *)&direccionLocal, len);
}

SocketMulticast::~SocketMulticast()
{
    close(s);
}

int SocketMulticast::recibe(PaqueteDatagrama &p)
{
    sockaddr_in direccionForanea;
    int clilen = sizeof(direccionForanea);
    int recibidos = recvfrom(s,p.obtieneDatos(),p.obtieneLongitud(),0,(struct sockaddr *)&direccionForanea,(socklen_t *)&clilen);
    p.inicializaIp(inet_ntoa(direccionForanea.sin_addr));
    p.inicializaPuerto(ntohs(direccionForanea.sin_port));
    return recibidos;
}

/**
 * Recibe un paquete de datagrama por multicast.
 * 
 * Retorna el número de bytes recibidos o -1 en caso de error.
 * 
 * El contenido del paquete debe ser una estructura tipo mensaje.
 * Si el mensaje recibido ya ha sido recibido, retorna -2.
 */
int SocketMulticast::recibeConfiable(PaqueteDatagrama &p)
{
    // Se abre socket unicast para enviar respuesta.
    SocketDatagrama socketUnicast(0);

    // Recibe el paquete.
    int recibidos = recibe(p);

    // Se extrae id.
    mensaje m = *(mensaje *)p.obtieneDatos();

    if (obtenerUltimoId() != m.id)
    {
        mensaje reply = {REPLY, m.id};
        PaqueteDatagrama pdUnicast((char *)&reply, sizeof(reply), p.obtieneDireccion(), UNICAST_PORT);
        // Se genera paquete y se envía.
        socketUnicast.envia(pdUnicast);

        // Cierra socket.
        socketUnicast.~SocketDatagrama();

        // Actualiza último id.
        fijarUltimoId(m.id);
    }
    else
    {
        return -2;
    }

    return recibidos;
}

int SocketMulticast::envia(PaqueteDatagrama &p, unsigned char ttl)
{
    unsigned char TTL = ttl;
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&TTL, sizeof(TTL));
    sockaddr_in direccionForanea;
    int len = sizeof(direccionForanea);
    bzero(&direccionForanea, len);
    direccionForanea.sin_family = AF_INET;
    direccionForanea.sin_addr.s_addr = inet_addr(p.obtieneDireccion());
    direccionForanea.sin_port = htons(p.obtienePuerto());
    return sendto(s, p.obtieneDatos(), p.obtieneLongitud(), 0, (sockaddr *)&direccionForanea, sizeof(direccionForanea));
}

/**
 * Envía un paquete de datagrama mediante socket multicast a una determinada cantidad de receptores.
 * Se requiere de un valor TTL para indicar qué tanto se propaga el mensaje.
 * 
 * Retorna la cantidad de bytes enviados o -1 en caso de error.
 */
int SocketMulticast::enviaConfiable(PaqueteDatagrama &p, unsigned char ttl, int totalReceptores)
{
    SocketDatagrama socketUnicast(UNICAST_PORT);

    PaqueteDatagrama request(MAX_LONGITUD_DATOS);

    int receptoresRestantes = totalReceptores;
    int receptoresActivos = 0;
    int intentos = INTENTOS;
    int enviados;

    while (intentos > 0 && receptoresRestantes > 0)
    {
        for (int i = 0; i < receptoresRestantes; ++i)
        {
            enviados = envia(p, ttl);
            if (socketUnicast.recibeTimeout(request, 4, 0) >= 0)
            {
                receptoresActivos++;
            }
        }
        receptoresRestantes -= receptoresActivos;
        receptoresActivos = 0;
        intentos--;
    }
    
    return receptoresRestantes == 0 ? enviados : -1;
}

void SocketMulticast::unirAlGrupo(const char *multicastIP)
{
    ip_mreq multicast;
    multicast.imr_multiaddr.s_addr = inet_addr(multicastIP);
    multicast.imr_interface.s_addr = INADDR_ANY;
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&multicast, sizeof(multicast));
}

void SocketMulticast::salirDelGrupo(const char *multicastIP)
{
    ip_mreq multicast;
    multicast.imr_multiaddr.s_addr = inet_addr(multicastIP);
    multicast.imr_interface.s_addr = INADDR_ANY;
    setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)&multicast, sizeof(multicast));
}

void SocketMulticast::fijarUltimoId(unsigned int id)
{
    ultimoId = id;
}

unsigned int SocketMulticast::obtenerUltimoId()
{
    return ultimoId;
}