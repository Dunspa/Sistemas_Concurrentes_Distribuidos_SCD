// Jose Luis Gallego Peña - A1

#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int
   num_filosofos = 5,
   num_procesos  = 2 * num_filosofos + 1,
   id_camarero = 10,
   etiq_coger = 0,
   etiq_soltar = 1,
   etiq_sentarse = 2,
   etiq_levantarse = 3;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template<int min, int max> int aleatorio(){
   static default_random_engine generador((random_device())());
   static uniform_int_distribution<int> distribucion_uniforme(min, max) ;
   return distribucion_uniforme(generador);
}

// ---------------------------------------------------------------------

void funcion_camarero(){
   int valor, flag, id_filosofo, s = 0;
   MPI_Status estado;

   while (true){
      if (s < 4){
         MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_sentarse, MPI_COMM_WORLD, &estado);
         id_filosofo = estado.MPI_SOURCE;
         cout << "Camarero: Filósofo " << id_filosofo << " se sienta en la mesa"
              << endl;
         s++;
         // Si el filósofo está rellenito ocupa una silla más
         if (id_filosofo == 6){	
         	s++;
         }
         cout << "Camarero: Hay " << s << " sillas ocupadas" << endl;
      }
      

      MPI_Iprobe(MPI_ANY_SOURCE, etiq_levantarse, MPI_COMM_WORLD, &flag, &estado);
      if (flag > 0){
         MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_levantarse, MPI_COMM_WORLD, &estado);
         id_filosofo = estado.MPI_SOURCE;
         if (s == 4){
		   	cout << "Estaban cuatro filósofos sentados y se ha levantado el filósofo "
		   		  << id_filosofo << endl;
      	}
         else{
         	cout << "Camarero: Filósofo " << id_filosofo << " se levanta de la mesa"
              	  << endl;
         }
         s--;
         // Si el filósofo está rellenito se libera una silla más
         if (id_filosofo == 6){	
         	s--;
         }
         cout << "Camarero: Hay " << s << " sillas ocupadas" << endl;
      }
   }
}

// ---------------------------------------------------------------------

void funcion_filosofos(int id){
   int id_ten_izq = (id + 1) % num_procesos,                //id. tenedor izq.
       id_ten_der = (id + num_procesos - 1) % num_procesos; //id. tenedor der.
   int peticion;

   while (true){
      // 1. Sentarse
      MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);

      // 2. Tomar tenedores (El primer filósofo coge los tenedores al revés)
      if (id == 0){
         cout << "Filósofo " << id << " solicita ten. der." << id_ten_der << endl;
         MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_coger, MPI_COMM_WORLD);

         cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
         MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_coger, MPI_COMM_WORLD);
      }
      else{
         cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
         MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_coger, MPI_COMM_WORLD);

         cout << "Filósofo " << id << " solicita ten. der." << id_ten_der << endl;
         MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_coger, MPI_COMM_WORLD);
      }

      // 3. Comer
      cout << "Filósofo " << id << " comienza a comer" << endl;
      sleep_for(milliseconds(aleatorio<10,100>()));

      // 4. Soltar tenedores
      cout << "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl;
      MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_soltar, MPI_COMM_WORLD);

      cout << "Filósofo " << id << " suelta ten. der. " <<id_ten_der << endl;
      MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_soltar, MPI_COMM_WORLD);

      // 5. Levantarse
      MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);

      // 6. Pensar
      cout << "Filosofo " << id << " comienza a pensar" << endl;
      sleep_for(milliseconds(aleatorio<10,100>()));
   }
}

// ---------------------------------------------------------------------

void funcion_tenedores(int id){
   int valor, id_filosofo;  // valor recibido, identificador del filósofo
   MPI_Status estado;       // metadatos de las dos recepciones

   while (true){
      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_coger, MPI_COMM_WORLD, &estado);
      id_filosofo = estado.MPI_SOURCE;
      cout << "Ten. " << id << " ha sido cogido por filo. " << id_filosofo << endl;

      MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_soltar, MPI_COMM_WORLD, &estado);
      cout << "Ten. " << id << " ha sido liberado por filo. " << id_filosofo << endl;
   }
}

// ---------------------------------------------------------------------

int main(int argc, char * argv[]){
   int id_propio, num_procesos_actual;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

   if (num_procesos == num_procesos_actual){
      // ejecutar la función correspondiente a 'id_propio'
      if (id_propio % 2 == 0){   // si es par
         if (id_propio == id_camarero){   // si el id es 10 (id del camarero)
            funcion_camarero();  // es el camarero
         }
         else{ // si es cualquier otro id par
            funcion_filosofos(id_propio); // es un filósofo
         }
      }
      else{ // si es impar
         funcion_tenedores(id_propio); // es un tenedor
      }
   }
   else{
      if (id_propio == 0){ // solo el primero escribe error, indep. del rol
         cout << "el número de procesos esperados es:    " << num_procesos << endl
              << "el número de procesos en ejecución es: " << num_procesos_actual << endl
              << "(programa abortado)" << endl;
      }
   }

   MPI_Finalize();
   return 0;
}
