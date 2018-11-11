// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Práctica 2. Casos prácticos de monitores en C++11.
//
// El problema de los fumadores (fumadores_su.cpp)
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
const int NUM_FUMADORES = 3;

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

class Estanco : public HoareMonitor{
private:
   // Variables permanentes
   bool mostr_vacio;                   // Indica si el mostrador está vacío
   int ingrediente_mostrador;          // Ingrediente colocado en el mostrador
   mutex cerrojo_monitor;              // Cerrojo del monitor
   // Colas condición
   CondVar mostrador;                        // El estanquero espera aquí cuando el mostrador tiene un ingrediente
   CondVar sin_ingrediente[NUM_FUMADORES];   // Esperan los fumadores a que se les de un ingrediente 
public:
   Estanco();                          // Constructor
   void obtenerIngrediente(int i);     // El fumador espera su ingrediente y lo retira
   void ponerIngrediente(int ingre);   // Pone ingrediente en el mostrador
   void esperarRecogidaIngrediente();  // Espera bloqueada hasta que el mostrador esté libre
};

Estanco::Estanco(){
   ingrediente_mostrador = -1;
   mostr_vacio = true;

   mostrador = newCondVar();
   for (int i = 0 ; i < NUM_FUMADORES ; i++){
      sin_ingrediente[i] = newCondVar();
   }
}

void Estanco::obtenerIngrediente(int i){
   // Comprueba si hay un ingrediente en el mostrador
   if (ingrediente_mostrador != i){
      sin_ingrediente[i].wait();
   }

   // Se recoge el ingrediente
   mostr_vacio = true;   
   cout << "El fumador " << i << " retira su ingrediente" << endl;
   mostrador.signal(); 
}

void Estanco::ponerIngrediente(int ingre){
   ingrediente_mostrador = ingre;
   // Se indica al fumador que puede obtener su ingrediente
   mostr_vacio = false;
   cout << "Estanquero produce ingrediente " << ingre << endl;
   sin_ingrediente[ingrediente_mostrador].signal();
}

void Estanco::esperarRecogidaIngrediente(){
   if (!mostr_vacio)
      mostrador.wait(); // El estanquero espera hasta que se consuma el ingrediente
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra
void Fumar(int num_fumador){

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

//-------------------------------------------------------------------------
// Función que produce un ingrediente aleatorio
int ProducirIngrediente(){
   int i = int (aleatorio<0, 2>());

   return i;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
void funcion_hebra_estanquero(MRef<Estanco> monitor){
   while(true){
      int ingre = ProducirIngrediente();
      monitor->ponerIngrediente(ingre);
      monitor->esperarRecogidaIngrediente();
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador){
   while(true){
      monitor->obtenerIngrediente(num_fumador);
      Fumar(num_fumador);    
   }
}

//----------------------------------------------------------------------

int main(){
   cout << "--------------------------------------------------------" << endl
        << "             Problema de los fumadores.                 " << endl
        << "--------------------------------------------------------" << endl
        << flush;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Estanco> monitor = Create<Estanco>();

   // declarar hebras y ponerlas en marcha
   thread hebra_estanquero(funcion_hebra_estanquero, monitor);
   thread fumadores[NUM_FUMADORES];
   for (int i = 0 ; i < NUM_FUMADORES ; i++)
      fumadores[i] = thread(funcion_hebra_fumador, monitor, i);
   
   hebra_estanquero.join();
   for (int i = 0 ; i < NUM_FUMADORES ; i++)
      fumadores[i].join();
}