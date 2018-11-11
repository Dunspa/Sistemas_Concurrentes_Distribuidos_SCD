// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// Productor/Consumidor múltiples (prodcons_fifo_su.cpp)
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Precondición: num_productores y num_consumidores divisores de num_items
// Opcion FIFO (queue)
//
// Jose Luis Gallego Peña - A1
// -----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

constexpr int num_items = 40;          // número de items a producir/consumir
constexpr int num_productores = 5;    // número de hebras productoras
constexpr int num_consumidores = 4;   // número de hebras consumidoras
mutex mtx;                             // mutex de escritura en pantalla
unsigned cont_prod[num_items],         // contadores de verificación: producidos
         cont_cons[num_items];         // contadores de verificación: consumidos

// indica para cada hebra productora cuantos items ha producido 
unsigned producidos[num_productores] = {0};  
// indica para cada hebra consumidora cuantos items ha consumido
unsigned consumidos[num_consumidores] = {0};  

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme(min, max);
  return distribucion_uniforme(generador);
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_hebra){
   static int contador = 0;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   producidos[num_hebra]++;
   mtx.lock();
   cout << "(hebra " << num_hebra << ")producido: " << contador << "(total: " 
        << producidos[num_hebra] << ")" << endl << flush;
   mtx.unlock();
   cont_prod[contador]++;
   return contador++;
}

void consumir_dato(unsigned dato, int num_hebra){
   if (num_items <= dato){
      cout << " dato === " << dato << ", num_items == " << num_items << endl;
      assert(dato < num_items);
   }

   cont_cons[dato]++;
   consumidos[num_hebra]++;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << "(hebra " << num_hebra 
        << ")" << endl;
   mtx.unlock();
}

void ini_contadores(){
   for(unsigned i = 0 ; i < num_items ; i++){  
      cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

void test_contadores(){
   bool ok = true;
   cout << "comprobando contadores ...." << flush;

   for(unsigned i = 0 ; i < num_items ; i++){
      if (cont_prod[i] != 1){
         cout << "error: valor " << i << " producido " << cont_prod[i] 
              << " veces." << endl;
         ok = false;
      }
      if (cont_cons[i] != 1){
         cout << "error: valor " << i << " consumido " << cont_cons[i] 
         << " veces" << endl;
         ok = false;
      }
   }

   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons2SU : public HoareMonitor{
private:
   // constantes
   static const int num_celdas_total = 10;   // núm. de entradas del buffer
   // variables permanentes
   int buffer[num_celdas_total]; // buffer de tamaño fijo, con los datos
   int primera_libre;            // indice de celda de la próxima inserción
   int primera_ocupada;          // indice de celda de la próxima extracción
   int num_celdas_ocupadas;          // indica el número de celdas ocupadas
   // colas condicion
   CondVar ocupadas;  // cola donde espera el consumidor (n>0)
   CondVar libres;    // cola donde espera el productor (n<num_celdas_total)

public:                    
   // constructor y métodos públicos
   ProdCons2SU();             // constructor
   int leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir(int valor);  // insertar un valor (sentencia E) (productor)
};

ProdCons2SU::ProdCons2SU(){
   primera_libre = 0;
   primera_ocupada = 0;
   num_celdas_ocupadas = 0;
   ocupadas = newCondVar();
   libres = newCondVar();
}

// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons2SU::leer(){
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   while (num_celdas_ocupadas == 0){
      ocupadas.wait();
   }

   // hacer la operación de lectura, actualizando estado del monitor
   assert(0 < num_celdas_ocupadas);
   if (primera_ocupada == num_celdas_total)
      primera_ocupada = 0;
   const int valor = buffer[primera_ocupada];
   primera_ocupada++;
   num_celdas_ocupadas--;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor;
}

void ProdCons2SU::escribir(int valor){
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   while (num_celdas_ocupadas == num_celdas_total){
      libres.wait();
   }

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert(num_celdas_ocupadas < num_celdas_total);

   // hacer la operación de inserción, actualizando estado del monitor
   if (primera_libre == num_celdas_total)
      primera_libre = 0;
   buffer[primera_libre] = valor;
   primera_libre++;
   num_celdas_ocupadas++;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdCons2SU> monitor, int num_hebra){
   for(unsigned i = 0 ; i < num_items/num_productores ; i++){
      int valor = producir_dato(num_hebra);
      monitor->escribir(valor);
   }
}

void funcion_hebra_consumidora(MRef<ProdCons2SU> monitor, int num_hebra){
   for(unsigned i = 0 ; i < num_items/num_consumidores ; i++){
      int valor = monitor->leer();
      consumir_dato(valor, num_hebra);
   }
}

int main(){
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Múltiples prod/cons, Monitor SU, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush;

   ini_contadores();
   
   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdCons2SU> monitor = Create<ProdCons2SU>();

   thread productores[num_productores];
   thread consumidores[num_consumidores];
   for (int i = 0 ; i < num_productores ; i++)
      productores[i] = thread(funcion_hebra_productora, monitor, i);
   for (int i = 0 ; i < num_consumidores ; i++)
      consumidores[i] = thread(funcion_hebra_consumidora, monitor, i);

   for (int i = 0 ; i < num_productores ; i++)
      productores[i].join();
   for (int i = 0 ; i < num_consumidores ; i++)
      consumidores[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}