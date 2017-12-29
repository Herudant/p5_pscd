//============================================================================
// Name        : Vuelo.cpp
// Author      : Eduardo Alonso Monge
// Description : practica 5 de pscd
//============================================================================

#include "vuelo.hpp"

Vuelo::Vuelo(){
	this -> num_asientos = 0;
	this -> num_clientes = 0;
	this -> vuelo_cerrado = false;
	for (int i = 0; i < NUM_FILAS; i++) {
		for (int j = 0; j < NUM_ASIENTOS; j++)
			this -> asiento_libre[i][j] = true;
	}
}

/* Cada asiento esta representado por un número de tal forma que
 * la primera cifra del número indica la fila (0-9) y la segunda
 * cifra indica el asiento (0-3). Rango válido (0-39)
*/
int Vuelo::reservar(const int fila, const int columna){
  unique_lock<mutex> lck(mtx);
	int ret = -1;
	if((fila < NUM_FILAS && columna < NUM_ASIENTOS) and !vuelo_cerrado) {
	  if(asiento_libre[fila][columna]){
			this -> asiento_libre[fila][columna] = false;
			this -> num_asientos++;
		  ret = (fila * 10) + columna;
		}
	}
  return ret;
}

/* Devuelve una lista de asientos libres codificados */
list<int> Vuelo::get_libres(){
	list<int> ret;
	for (int i = 0; i < NUM_FILAS; i++) {
		for (int j = 0; j < NUM_ASIENTOS; j++){
			if(asiento_libre[i][j]){
				ret.push_back((j*10) + i);
			}
		}
	}
	return ret;
}

bool Vuelo::es_completo(){
	return (num_asientos == NUM_FILAS * NUM_ASIENTOS) ? true : false;
}

/* Libera el asiento */
void Vuelo::liberar(const int fila, const int columna){
	unique_lock<mutex> lck(mtx);
	this -> asiento_libre[fila][columna] = 1;
	this -> num_asientos--;
}

void Vuelo::nuevoCliente(){
	unique_lock<mutex> lck(mtx);
	this -> num_clientes++;
}

void Vuelo::cerrarCliente(){
	unique_lock<mutex> lck(mtx);
	this -> num_clientes--;
	if(vuelo_cerrado and num_clientes == 0){
		this -> espera.notify_one();
	}
}

void Vuelo::cerrarVuelo(){
	unique_lock<mutex> lck(mtx);
	this -> vuelo_cerrado = true;
	if(num_clientes != 0){
		this -> espera.wait(lck);
	}
}

bool Vuelo::getVuelo_cerrado(){
	return this -> vuelo_cerrado;
}
