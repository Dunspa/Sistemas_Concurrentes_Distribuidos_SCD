// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Práctica 1. Sincronización de hebras con semáforos.
//
// El problema de los fumadores (fumadores.cpp)
//
// Jose Luis Gallego Peña - A1
// -----------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>    // dispositivos, generadores y distribuciones aleatorias
#include <chrono>    // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std;
using namespace SEM;

//**********************************************************************
// variables compartidas
const int NUM_FUMADORES = 3;

// Semáforos
Semaphore ingr_disp[NUM_FUMADORES] = {0, 0, 0}; // Indica si el ingrediente está disponible (1) o no (0)
Semaphore mostr_vacio = 1;          // Indica si el mostrador está vacío (1) o no (0)
Semaphore mut = 1;                  // Semáforo para la exclusión mutua en los fumadores

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

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra
void fumar(int num_fumador){

   // calcular milisegundos aleatorios de duración de la acción de fumar
   chrono::milliseconds duracion_fumar(aleatorio<20,200>());

   // informa de que comienza a fumar
   cout << "Fumador " << num_fumador << "  :"
        << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" 
        << endl;

   // espera bloqueada un tiempo igual a 'duracion_fumar' milisegundos
   this_thread::sleep_for(duracion_fumar);

   // informa de que ha terminado de fumar
   cout << "Fumador " << num_fumador 
        << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
void funcion_hebra_estanquero(){
   while(true){

      // Retraso aleatorio y producción del ingrediente
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
      int i = int (aleatorio<0, 2>());

      sem_wait(mostr_vacio);
      cout << "Estanquero produce ingrediente " << i << endl;
      sem_signal(ingr_disp[i]);

   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador(int num_fumador){
   while(true){
      
      sem_wait(mut);
      // Sección crítica
      cout << "Fumador " << num_fumador << " espera para fumar. "
           << "Necesita el ingrediente " << num_fumador << endl;
      sem_signal(mut);

      sem_wait(ingr_disp[num_fumador]);
      cout << "El fumador " << num_fumador << " retira su ingrediente" << endl;
      sem_signal(mostr_vacio);

      fumar(num_fumador);
      
   }
}

//----------------------------------------------------------------------

int main(){

   cout << "--------------------------------------------------------" << endl
        << "             Problema de los fumadores.                 " << endl
        << "--------------------------------------------------------" << endl
        << flush;

   // declarar hebras y ponerlas en marcha
   thread hebra_estanquero(funcion_hebra_estanquero);

   thread fumadores[NUM_FUMADORES];

   for (int i = 0 ; i < NUM_FUMADORES ; i++)
      fumadores[i] = thread(funcion_hebra_fumador, i);
   
   hebra_estanquero.join();

   for (int i = 0 ; i < NUM_FUMADORES ; i++)
      fumadores[i].join();

}