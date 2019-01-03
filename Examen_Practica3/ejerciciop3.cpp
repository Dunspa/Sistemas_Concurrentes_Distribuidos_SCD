// Jose Luis Gallego Peña - A1

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int
	num_procesos_esperado = 10,
	num_clientes = 8,
	id_dependiente = 8,
	id_reponedor = 9,
	etiq_clientes = 3,
	etiq_par = 2,
	etiq_impar = 1,
	etiq_reponedor = 0;
	
//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}	

// ---------------------------------------------------------------------

void proceso_cliente(int id){
	int peticion_botellas, etiq;
	MPI_Status estado;

	while (true){
		cout << "Cliente " << id << " está de camino a la tienda" << endl;
		sleep_for(milliseconds(aleatorio<10,100>()));
		
		if (id % 2 == 0){
			etiq = etiq_par;
		}
		else{
			etiq = etiq_impar;
		}
		
		cout << "Cliente " << id << " llega a la tienda y pide " << etiq 
		  	<< " botellas" << endl;
		MPI_Ssend(&peticion_botellas, 1, MPI_INT, id_dependiente, etiq, MPI_COMM_WORLD);
		
		MPI_Recv(&peticion_botellas, 1, MPI_INT, id_dependiente, etiq, MPI_COMM_WORLD, &estado);
		cout << "Cliente " << id << " se va de la tienda" << endl;
	}
}

// ---------------------------------------------------------------------

void proceso_reponedor(){
	int peticion_botellas;
	MPI_Status estado;

	while (true){
		cout << "Reponedor preparado para reponer" << endl;
		
		MPI_Recv(&peticion_botellas, 1, MPI_INT, id_dependiente, etiq_reponedor, MPI_COMM_WORLD, &estado);
		cout << "Reponedor transporta botellas a la tienda" << endl;
		sleep_for(milliseconds(aleatorio<10,100>()));
		
		cout << "Reponedor deja botellas en la tienda" << endl;
		MPI_Ssend(&peticion_botellas, 1, MPI_INT, id_dependiente, etiq_reponedor, MPI_COMM_WORLD);
	}
}

// ---------------------------------------------------------------------

void proceso_dependiente(){
	int peticion_botellas, etiq, botellas = 12, emisor;
	MPI_Status estado;

	while (true){
		if (botellas > 4){	// Si quedan más de 4 botellas
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
		}
		else if (botellas >= 1){	// Si queda al menos una botella
			MPI_Probe(MPI_ANY_SOURCE, etiq_impar, MPI_COMM_WORLD, &estado);
		}
		else if (botellas == 0){	// Si no quedan botellas
			MPI_Ssend(&peticion_botellas, 1, MPI_INT, id_reponedor, etiq_reponedor, MPI_COMM_WORLD);
			MPI_Probe(id_reponedor, etiq_reponedor, MPI_COMM_WORLD, &estado);
			cout << "Dependiente recibe las nuevas botellas" << endl;
		}
		
		emisor = estado.MPI_SOURCE;
		MPI_Recv(&peticion_botellas, 1, MPI_INT, emisor, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
		if (emisor < 8){
			if (emisor % 2 == 0){
				botellas -= 2;
				etiq = etiq_par;
			}
			else{
				botellas--;
				etiq = etiq_impar;
			}
			
			cout << "Cliente " << emisor << " recibe " << etiq << " botellas ";
			cout << "(actualmente quedan " << botellas << " botellas en la tienda)" 
			     << endl;
			MPI_Ssend(&peticion_botellas, 1, MPI_INT, emisor, etiq, MPI_COMM_WORLD);
		}
		else if (emisor == id_reponedor){
			botellas += 12;
		}
	}
}

// ---------------------------------------------------------------------

int main(int argc, char * argv[]){
	int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);
	
	if (num_procesos_esperado == num_procesos_actual){
		if (id_propio < num_clientes){
			proceso_cliente(id_propio);
		}
		else if (id_propio == id_dependiente){
			proceso_dependiente();
		}
		else if (id_propio == id_reponedor){
			proceso_reponedor();
		}
   }
   else{
      if (id_propio == 0){ // solo el primero escribe error, indep. del rol
         cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
              << "el número de procesos en ejecución es: " << num_procesos_actual << endl
              << "(programa abortado)" << endl;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize();
   return 0;
}
