// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Práctica 2. Casos prácticos de monitores en C++11.
//
// El problema del barbero (barbero_su.cpp)
// Semántica SU
//
// Jose Luis Gallego Peña - A1
// -----------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>    // dispositivos, generadores y distribuciones aleatorias
#include <chrono>    // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

//**********************************************************************
// variables compartidas
const int NUM_CLIENTES = 3;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------
template< int min, int max > int aleatorio(){
   static default_random_engine generador( (random_device())() );
   static uniform_int_distribution<int> distribucion_uniforme( min, max );
   return distribucion_uniforme( generador );
}

class Barberia : public HoareMonitor{
private:
   // Variables permanentes
   int cliente_actual;              // Cliente al que se le está cortando el pelo
   // Colas condición
   CondVar barbero;                 // Cola donde duerme el barberoa
   CondVar sala_espera;             // Cola donde esperan los clientes
   CondVar silla;                   // Cola donde espera el cliente mientras se le pela
public:
   Barberia();
   void cortarPelo(int i);    // El cliente despierta al barbero o lo espera
   void siguienteCliente();   // Barbero espera la llegada de un nuevo cliente
   void finCliente();         // Indica y espera que el cliente salga de la barbería
};

Barberia::Barberia(){
   barbero = newCondVar();
   sala_espera = newCondVar();
   silla = newCondVar();
}

void Barberia::cortarPelo(int i){
   cout << "Cliente " << i << " entra a la barbería" << endl;

   // Barbero ocupado cortando el pelo a otro cliente
   if (barbero.empty()){
      cout << "Cliente " << i << " espera en la sala de espera" << endl;
      sala_espera.wait();
   }
   else{
      cout << "El barbero se despierta" << endl;
   }

   cliente_actual = i;
   cout << "Se le corta el pelo al cliente " << i << endl;
   barbero.signal();
   silla.wait();

   cout << "Cliente " << i << " sale de la barbería y espera un tiempo" 
        << endl;
}

void Barberia::siguienteCliente(){
   if (sala_espera.empty()){
      cout << "No hay clientes en la sala de espera. El barbero se duerme" 
           << endl;
      barbero.wait();
   }

   sala_espera.signal();
}

void Barberia::finCliente(){
   cout << "Se termina de pelar al cliente " << cliente_actual << endl;
   silla.signal();
}

//----------------------------------------------------------------------
// funciones que ejecutan esperas bloqueadas de duración aleatoria
void EsperarFueraBarberia(int i){
   this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
}

void CortarPeloACliente(){
   this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del cliente
void funcion_hebra_cliente(MRef<Barberia> monitor, int cliente){
   while(true){
      // 1. Cliente entra a la barbería y espera hasta que el barbero lo llame
      // 2. Cliente espera mientras el barbero lo pela
      monitor->cortarPelo(cliente);
      // 3. Cliente espera fuera de la barbería un tiempo
      EsperarFueraBarberia(cliente);
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del barbero
void funcion_hebra_barbero(MRef<Barberia> monitor){
   while(true){
      // 1. No hay clientes: Barbero espera dormido hasta que llegue un cliente
      monitor->siguienteCliente();
      // 2. Barbero pela al cliente durante un tiempo
      CortarPeloACliente();
      // 3. Se termina de pelar al cliente
      monitor->finCliente();
   }
}

int main(){
   cout << "--------------------------------------------------------" << endl
        << "             Problema del barbero.                      " << endl
        << "--------------------------------------------------------" << endl
        << flush;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Barberia> monitor = Create<Barberia>();

   // declarar hebras y ponerlas en marcha
   thread hebra_barbero(funcion_hebra_barbero, monitor);
   thread clientes[NUM_CLIENTES];
   for (int i = 0 ; i < NUM_CLIENTES; i++)
      clientes[i] = thread(funcion_hebra_cliente, monitor, i);
   
   hebra_barbero.join();
   for (int i = 0 ; i < NUM_CLIENTES ; i++)
      clientes[i].join();
}