//******************************************************************
// File:   ServidorMulticliente.cpp
// Author: PSCD-Unizar
// Date:   Noviembre 2015
// Coms:   Ejemplo de servidor multicliente con comunicación síncrona mediante sockets
//         Compilar el fichero "Makefile" asociado, mediante "make".
//*****************************************************************

#include "Socket.hpp"
#include <iostream>
#include <thread>
#include <cstring> //manejo de cadenas tipo C
#include <stdlib.h>     /* atoi */
#include <string>
#include <list>
#include "vuelo.hpp"
#include "codificador.h"

using namespace std;

void avisarFin(int socket_fd, Socket& socket, Vuelo& vuelo);
void handler(int n);
void servCliente(Socket& soc, int client_fd, Vuelo& vuelo);

//-------------------------------------------------------------
int main(int argc, char* argv[]){
	const int N = 2;

	if (argc != 2) {
		cerr << "Parámetros mal introducidos: <puerto Servidor>" << endl;
		exit(1);
	}

	// se captura la señal de cierre para evitar que termine con ctrl + c.
	signal(SIGINT, handler);

  // Creo el monitor que gestionará el vuelo
	Vuelo vuelo;

	// Dirección y número donde escucha el proceso servidor
	string SERVER_ADDRESS = "localhost";
  int SERVER_PORT = atoi(argv[1]);

  // Threads para atender clientes
  thread cliente[N];
  int client_fd[N];



	// Creación del socket con el que se llevará a cabo
	// la comunicación con el servidor.
	Socket socket(SERVER_PORT);

	// Bind
	int socket_fd =socket.Bind();
	if (socket_fd == -1) {
		string mensError(strerror(errno));
    cerr << "Error en el bind: " + mensError + "\n";
		exit(1);
	}

  // Thread para gestionar finalización
  thread fin(&avisarFin, socket_fd, ref(socket), ref(vuelo));
  fin.detach();

	// Listen
  int max_connections = N;
	int error_code = socket.Listen(max_connections);
	if (error_code == -1) {
		string mensError(strerror(errno));
    cerr << "Error en el listen: " + mensError + "\n";
		// Cerramos el socket
		socket.Close(socket_fd);
		exit(1);
	}

	for (int i=0; i<max_connections; i++) {
		// Accept
		client_fd[i] = socket.Accept();

    if(vuelo.getVuelo_cerrado())
      break;

		if(client_fd[i] == -1 ) {
			string mensError(strerror(errno));
    	cerr << "Error en el accept: " + mensError + "\n";
			// Cerramos el socket
			socket.Close(socket_fd);
			exit(1);
		}

		cout << "Lanzo thread nuevo cliente " + to_string(i) + "\n";
		cliente[i] = thread(&servCliente, ref(socket), client_fd[i], ref(vuelo));
		cout << "Nuevo cliente " + to_string(i) + " aceptado" + "\n";
	}

	for (int i=0; i<max_connections; i++) {
    if(cliente[i].joinable())
		  cliente[i].join();
	}
  // Cerramos el socket del servidor
  error_code = socket.Close(socket_fd);
  if (error_code == -1) {
		string mensError(strerror(errno));
    cerr << "Error cerrando el socket del servidor: " + mensError + "\n";
	}

	// Mensaje de despedida
	cout << "Bye bye" << endl;

  return error_code;
}


//------------------------------------------------------------------------------
//-------------------------- Otras funciones -----------------------------------
//------------------------------------------------------------------------------


//capturo señal de cierre para evitar que se termine on ctrl + c.
void handler(int n){
  signal(SIGINT, handler);
  cout << "Para salir escribe 'END OF SERVICE'\n";
}

// Atiende peticion de un cliente
void servCliente(Socket& soc, int client_fd, Vuelo& vuelo) {

  // Si el vuelo esta cerrado por parte del servidor cerramos el socket
  if(vuelo.getVuelo_cerrado()){
    soc.Close(client_fd);
  }

	char MENS_FIN[]="END OF SERVICE";
	// Buffer para recibir el mensaje
	int length = 100;
	char buffer[length];
	bool out = false; // Inicialmente no salir del bucle
  vuelo.nuevoCliente();
	while(!out) {
		// Recibimos el mensaje del cliente
		int rcv_bytes = soc.Recv(client_fd,buffer,length);
		if (rcv_bytes == -1) {
			string mensError(strerror(errno));
    		cerr << "Error al recibir datos: " + mensError + "\n";
			// Cerramos los sockets
			soc.Close(client_fd);
		}

		cout << "\nMensaje recibido: " << buffer << endl;

		// Si recibimos "END OF SERVICE" --> Fin de la comunicación
		if (0 == strcmp(buffer, MENS_FIN)) {
			out = true; // Salir del bucle
		} else {
      string mensaje;

			if(vuelo.es_completo()){
				// VUELO COMPLETO
				mensaje = "COMPLETO";
			}
			else{
        // respuesta = [fila, asiento]
        list<string> respuesta = decodificar(buffer, '#');
				int asiento = vuelo.reservar((atoi(respuesta.front().c_str()) - 1),
																     (atoi(respuesta.back().c_str())  - 1));
				if(asiento != -1){
					// Reserva hecha correctamente
					mensaje = "RESERVADO#" + to_string(asiento);
				}else{
					// Asiento ocupado, devuelvo libres (OCUPADO#L1#L2#...#Ln)
					mensaje = "OCUPADO";
					list<int> libres = vuelo.get_libres();
					for(int asiento : libres){
						mensaje+= "#" + to_string(asiento);
					}
				}
			}

			int send_bytes = soc.Send(client_fd, mensaje);
			if(send_bytes == -1) {
				string mensError(strerror(errno));
    			cerr << "Error al enviar datos: " + mensError + "\n";
				// Cerramos los sockets
				soc.Close(client_fd);
				exit(1);
			}
		}
	}
  vuelo.cerrarCliente();
	soc.Close(client_fd);
}

// Espera a recibir el mensaje de finalización para cerrar el servidor
void avisarFin(int socket_fd, Socket& socket, Vuelo& vuelo){
  string mensaje;
  while(1){
    getline(cin,mensaje);
    if (mensaje == "END OF SERVICE"){

      cout << "Esperando a la finalización de los clientes....\n";
      vuelo.cerrarVuelo();
      cout << "Clientes finalizados\n";

      int error_code = socket.Close(socket_fd);
      if(error_code == -1)
        cerr << "Error cerrando el socket: " << strerror(errno) << endl;

      // Mensaje de despedida
      cout << "Bye bye" << endl;
      exit(1);
    }
  }
}
