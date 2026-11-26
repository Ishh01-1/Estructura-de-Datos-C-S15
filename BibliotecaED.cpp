/*
 * ============================================================
 *  SISTEMA DE CONTROL DE PRESTAMOS DE UNA BIBLIOTECA ESCOLAR
 *  Curso: Estructura de Datos - Universidad Continental
 * ------------------------------------------------------------
 *  Estructuras de datos dinamicas lineales (hechas desde cero):
 *   1. LISTA ENLAZADA SIMPLE -> catalogo de libros
 *   2. COLA (FIFO)           -> lista de espera de cada libro
 *   3. PILA (LIFO)           -> historial de movimientos
 *  Persistencia: libros.txt, espera.txt, historial.txt
 * ============================================================
 */

#include <iostream>
#include <windows.h>
#include <iomanip>
#include <fstream>
#include <string>
#include <ctime>
using namespace std;

/* ------------------------------------------------------------
 *  1. DEFINICION DE ESTRUCTURAS (NODOS)
 * ------------------------------------------------------------ */

// Datos de un estudiante
struct Estudiante
{
    string codigo;
    string nombre;
};

// Nodo de la COLA de espera
struct NodoCola
{
    Estudiante est;
    NodoCola *sig;
};

// COLA: se inserta por el final y se atiende por el frente
struct ColaEspera
{
    NodoCola *frente;
    NodoCola *final;
    int cantidad;
};

// Nodo de la LISTA ENLAZADA de libros
struct Libro
{
    string codigo;
    string titulo;
    string autor;
    bool disponible;
    Estudiante prestadoA; // a quien esta prestado (si no esta disponible)
    ColaEspera espera;    // cola de espera propia de este libro
    Libro *sig;
};

// Nodo de la PILA de historial
struct Movimiento
{
    string tipo; // REGISTRO, PRESTAMO, DEVOLUCION, ESPERA, ELIMINACION
    string codLibro;
    string codEst;
    string nomEst;
    string fecha;
    Movimiento *sig;
};

/* ------------------------------------------------------------
 *  2. VARIABLES GLOBALES (cabezas de las estructuras)
 * ------------------------------------------------------------ */
Libro *cabeza = NULL;    // inicio de la lista de libros
Movimiento *tope = NULL; // tope de la pila de historial
int totalLibros = 0;
int totalMovimientos = 0;

// ==================================
// FUNCIONES DE APOYO
// ==================================

// COLORES
void color(int c)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void titulo(string t)
{
    color(11);
    cout << "\n  ==================================================\n";
    cout << "     " << t << "\n";
    cout << "  ==================================================\n";
    color(7);
}

void ok(string m)
{
    color(10);
    cout << "  [OK] " << m << "\n";
    color(7);
}

void error(string m)
{
    color(12);
    cout << "  [ERROR] " << m << "\n";
    color(7);
}

void aviso(string m)
{
    color(14);
    cout << "  [AVISO] " << m << "\n";
    color(7);
}

void pausa()
{
    cout << "\n  Presione ENTER para continuar...";
    cin.get();
}

// Lee un numero entre min y max (si escriben letras no se cuelga)
int leerEntero(string mensaje, int min, int max)
{
    int n;

    while (true)
    {
        cout << mensaje;

        if (cin >> n && n >= min && n <= max)
        {
            cin.ignore(10000, '\n');
            return n;
        }

        cin.clear();
        cin.ignore(10000, '\n');
        color(12);
        cout << "  [ERROR] Ingrese un numero entre " << min << " y " << max << ".\n";
        color(7);
    }
}

// Lee un texto que no este vacio
string leerTexto(string mensaje)
{
    string s;

    do
    {
        cout << mensaje;
        getline(cin, s);

        if (s == "")
            error("El campo no puede estar vacio.");

    } while (s == "");

    return s;
}

// Convierte un texto a mayusculas
string mayus(string s)
{
    for (int i = 0; i < (int)s.size(); i++)
        s[i] = toupper(s[i]);

    return s;
}

// Fecha y hora actual
string fechaActual()
{
    time_t ahora = time(0);
    char texto[20];
    strftime(texto, 20, "%d/%m/%Y %H:%M", localtime(&ahora));
    return texto;
}

// ==================================
// COLA: LISTA DE ESPERA DE UN LIBRO
// ==================================

// ENQUEUE: inserta al final
void encolar(Libro *libro, Estudiante e)
{
    NodoCola *nuevo = new NodoCola;
    nuevo->est = e;
    nuevo->sig = NULL;

    if (libro->espera.frente == NULL)
    {
        libro->espera.frente = nuevo;
        libro->espera.final = nuevo;
    }
    else
    {
        libro->espera.final->sig = nuevo;
        libro->espera.final = nuevo;
    }

    libro->espera.cantidad++;
}

// DEQUEUE: saca al primero de la cola
Estudiante desencolar(Libro *libro)
{
    NodoCola *aux = libro->espera.frente;
    Estudiante e = aux->est;

    libro->espera.frente = libro->espera.frente->sig;

    // Si la cola queda vacia
    if (libro->espera.frente == NULL)
        libro->espera.final = NULL;

    delete aux;
    libro->espera.cantidad--;
    return e;
}

// Busca un estudiante en la cola, devuelve su posicion o 0
int buscarEnCola(Libro *libro, string codEst)
{
    NodoCola *aux = libro->espera.frente;
    int pos = 1;

    while (aux != NULL)
    {
        if (aux->est.codigo == codEst)
            return pos;

        aux = aux->sig;
        pos++;
    }

    return 0;
}

void mostrarCola(Libro *libro)
{
    if (libro->espera.frente == NULL)
    {
        aviso("No hay estudiantes en espera.");
        return;
    }

    NodoCola *aux = libro->espera.frente;
    int pos = 1;

    while (aux != NULL)
    {
        cout << "    " << pos << ". [" << aux->est.codigo << "] " << aux->est.nombre;

        if (pos == 1)
            cout << "   <- FRENTE (siguiente en recibir el libro)";

        cout << "\n";
        aux = aux->sig;
        pos++;
    }
}

void liberarCola(Libro *libro)
{
    while (libro->espera.frente != NULL)
    {
        NodoCola *aux = libro->espera.frente;
        libro->espera.frente = libro->espera.frente->sig;
        delete aux;
    }

    libro->espera.final = NULL;
    libro->espera.cantidad = 0;
}

// ==================================
// PILA: HISTORIAL DE MOVIMIENTOS
// ==================================

// PUSH: inserta en el tope
void apilar(string tipo, string codLibro, string codEst, string nomEst, string fecha)
{
    Movimiento *nuevo = new Movimiento;
    nuevo->tipo = tipo;
    nuevo->codLibro = codLibro;
    nuevo->codEst = codEst;
    nuevo->nomEst = nomEst;
    nuevo->fecha = fecha;

    nuevo->sig = tope;
    tope = nuevo;
    totalMovimientos++;
}

// POP: elimina el tope
void desapilar()
{
    if (tope == NULL)
        return;

    Movimiento *aux = tope;
    tope = tope->sig;
    delete aux;
    totalMovimientos--;
}

void imprimirMovimiento(Movimiento *m)
{
    cout << left;
    cout << "    " << m->fecha << " | " << setw(11) << m->tipo
         << " | Libro: " << m->codLibro;

    if (m->codEst != "")
        cout << " | Est: " << m->codEst << " - " << m->nomEst;

    cout << "\n";
}

void mostrarPila()
{
    if (tope == NULL)
    {
        aviso("El historial esta vacio.");
        return;
    }

    cout << "  (del mas reciente al mas antiguo)\n";

    Movimiento *aux = tope;

    while (aux != NULL)
    {
        imprimirMovimiento(aux);
        aux = aux->sig;
    }

    cout << "  Total de movimientos: " << totalMovimientos << "\n";
}

// Busca en la pila los movimientos de un estudiante
void buscarEnPila(string codEst)
{
    Movimiento *aux = tope;
    int encontrados = 0;

    while (aux != NULL)
    {
        if (aux->codEst == codEst)
        {
            imprimirMovimiento(aux);
            encontrados++;
        }

        aux = aux->sig;
    }

    if (encontrados == 0)
        aviso("No hay movimientos de ese estudiante.");
    else
        cout << "  Movimientos encontrados: " << encontrados << "\n";
}

void liberarPila()
{
    while (tope != NULL)
        desapilar();
}

// ==================================
// LISTA ENLAZADA: LIBROS
// ==================================

// Buscar libro por codigo
Libro *buscarPorCodigo(string cod)
{
    Libro *aux = cabeza;

    while (aux != NULL)
    {
        if (aux->codigo == cod)
            return aux;

        aux = aux->sig;
    }

    return NULL;
}

// Insertar al final de la lista
Libro *insertarLibro(string cod, string tit, string aut)
{
    Libro *nuevo = new Libro;
    nuevo->codigo = cod;
    nuevo->titulo = tit;
    nuevo->autor = aut;
    nuevo->disponible = true;
    nuevo->prestadoA.codigo = "";
    nuevo->prestadoA.nombre = "";
    nuevo->espera.frente = NULL;
    nuevo->espera.final = NULL;
    nuevo->espera.cantidad = 0;
    nuevo->sig = NULL;

    if (cabeza == NULL)
    {
        cabeza = nuevo;
    }
    else
    {
        Libro *aux = cabeza;

        while (aux->sig != NULL)
            aux = aux->sig;

        aux->sig = nuevo;
    }

    totalLibros++;
    return nuevo;
}

// Eliminar libro por codigo
void eliminarLibro(string cod)
{
    Libro *actual = cabeza;
    Libro *anterior = NULL;

    while (actual != NULL && actual->codigo != cod)
    {
        anterior = actual;
        actual = actual->sig;
    }

    if (actual == NULL)
        return;

    if (anterior == NULL)
        cabeza = actual->sig;
    else
        anterior->sig = actual->sig;

    liberarCola(actual);
    delete actual;
    totalLibros--;
}

void encabezadoTabla()
{
    color(14);
    cout << "  CODIGO   | TITULO                       | AUTOR                | ESTADO      | ESPERA\n";
    cout << "  ---------+------------------------------+----------------------+-------------+-------\n";
    color(7);
}

void filaLibro(Libro *l)
{
    cout << left;
    cout << "  " << setw(8) << l->codigo.substr(0, 8)
         << " | " << setw(28) << l->titulo.substr(0, 28)
         << " | " << setw(20) << l->autor.substr(0, 20) << " | ";

    if (l->disponible)
    {
        color(10);
        cout << "Disponible ";
    }
    else
    {
        color(12);
        cout << "Prestado   ";
    }

    color(7);
    cout << " | " << l->espera.cantidad << "\n";
}

void mostrarLibros(bool soloDisponibles)
{
    if (cabeza == NULL)
    {
        aviso("No hay libros registrados.");
        return;
    }

    encabezadoTabla();

    Libro *aux = cabeza;
    int n = 0;

    while (aux != NULL)
    {
        if (soloDisponibles == false || aux->disponible)
        {
            filaLibro(aux);
            n++;
        }

        aux = aux->sig;
    }

    if (n == 0)
        aviso("No hay libros disponibles en este momento.");

    cout << "\n  Libros mostrados: " << n << " de " << totalLibros << "\n";
}

void detalleLibro(Libro *l)
{
    cout << "\n  Codigo : " << l->codigo << "\n";
    cout << "  Titulo : " << l->titulo << "\n";
    cout << "  Autor  : " << l->autor << "\n";

    if (l->disponible)
    {
        cout << "  Estado : Disponible\n";
    }
    else
    {
        cout << "  Estado : Prestado\n";
        cout << "  Prestado a: [" << l->prestadoA.codigo << "] " << l->prestadoA.nombre << "\n";
    }

    cout << "  Estudiantes en espera: " << l->espera.cantidad << "\n";
}

void liberarLista()
{
    while (cabeza != NULL)
    {
        Libro *aux = cabeza;
        cabeza = cabeza->sig;
        liberarCola(aux);
        delete aux;
    }

    totalLibros = 0;
}

// ==================================
// OPCIONES: GESTION DE LIBROS
// ==================================

void opRegistrarLibro()
{
    titulo("REGISTRAR LIBRO");
    string cod = mayus(leerTexto("  Codigo del libro: "));

    if (buscarPorCodigo(cod) != NULL)
    {
        error("Ya existe un libro con ese codigo.");
        return;
    }

    string tit = leerTexto("  Titulo: ");
    string aut = leerTexto("  Autor : ");

    insertarLibro(cod, tit, aut);
    apilar("REGISTRO", cod, "", "", fechaActual());
    ok("Libro registrado correctamente.");
}

void opBuscarCodigo()
{
    titulo("BUSCAR LIBRO POR CODIGO");
    string cod = mayus(leerTexto("  Codigo: "));
    Libro *l = buscarPorCodigo(cod);

    if (l == NULL)
        error("No se encontro el libro.");
    else
        detalleLibro(l);
}

void opBuscarTitulo()
{
    titulo("BUSCAR LIBRO POR TITULO");
    string txt = mayus(leerTexto("  Texto a buscar en el titulo: "));

    Libro *aux = cabeza;
    int n = 0;

    while (aux != NULL)
    {
        // find busca el texto dentro del titulo
        if (mayus(aux->titulo).find(txt) != string::npos)
        {
            if (n == 0)
                encabezadoTabla();

            filaLibro(aux);
            n++;
        }

        aux = aux->sig;
    }

    if (n == 0)
        error("No hay libros con ese titulo.");
    else
        cout << "\n  Coincidencias: " << n << "\n";
}

void opModificarLibro()
{
    titulo("MODIFICAR DATOS DE UN LIBRO");
    string cod = mayus(leerTexto("  Codigo del libro: "));
    Libro *l = buscarPorCodigo(cod);

    if (l == NULL)
    {
        error("No se encontro el libro.");
        return;
    }

    detalleLibro(l);
    cout << "\n  1. Cambiar titulo\n  2. Cambiar autor\n  0. Cancelar\n";
    int op = leerEntero("  Opcion: ", 0, 2);

    if (op == 1)
    {
        l->titulo = leerTexto("  Nuevo titulo: ");
        ok("Titulo actualizado.");
    }
    else if (op == 2)
    {
        l->autor = leerTexto("  Nuevo autor: ");
        ok("Autor actualizado.");
    }
}

void opEliminarLibro()
{
    titulo("ELIMINAR LIBRO");
    string cod = mayus(leerTexto("  Codigo del libro: "));
    Libro *l = buscarPorCodigo(cod);

    if (l == NULL)
    {
        error("No se encontro el libro.");
        return;
    }

    if (l->disponible == false)
    {
        error("No se puede eliminar: el libro esta prestado.");
        return;
    }

    if (l->espera.frente != NULL)
    {
        error("No se puede eliminar: hay estudiantes en espera.");
        return;
    }

    detalleLibro(l);
    int conf = leerEntero("  Confirmar eliminacion (1 = Si, 0 = No): ", 0, 1);

    if (conf == 1)
    {
        eliminarLibro(cod);
        apilar("ELIMINACION", cod, "", "", fechaActual());
        ok("Libro eliminado.");
    }
    else
    {
        aviso("Operacion cancelada.");
    }
}

// ==================================
// OPCIONES: PRESTAMOS Y ESPERA
// ==================================

void opPrestar()
{
    titulo("PRESTAR LIBRO");
    string cod = mayus(leerTexto("  Codigo del libro: "));
    Libro *l = buscarPorCodigo(cod);

    if (l == NULL)
    {
        error("No se encontro el libro.");
        return;
    }

    detalleLibro(l);
    cout << "\n";

    Estudiante e;
    e.codigo = mayus(leerTexto("  Codigo del estudiante: "));
    e.nombre = leerTexto("  Nombre del estudiante: ");

    // Si esta disponible se presta directo
    if (l->disponible)
    {
        l->disponible = false;
        l->prestadoA = e;
        apilar("PRESTAMO", l->codigo, e.codigo, e.nombre, fechaActual());
        ok("Libro prestado a " + e.nombre + ".");
        return;
    }

    // Si no esta disponible se ofrece la cola de espera
    if (l->prestadoA.codigo == e.codigo)
    {
        error("Ese estudiante ya tiene este libro.");
        return;
    }

    if (buscarEnCola(l, e.codigo) > 0)
    {
        aviso("El estudiante ya esta en la lista de espera.");
        return;
    }

    aviso("El libro esta prestado.");
    int r = leerEntero("  Agregar al estudiante a la lista de espera? (1 = Si, 0 = No): ", 0, 1);

    if (r == 1)
    {
        encolar(l, e);
        apilar("ESPERA", l->codigo, e.codigo, e.nombre, fechaActual());

        color(10);
        cout << "  [OK] Agregado a la lista de espera en la posicion "
             << l->espera.cantidad << ".\n";
        color(7);
    }
}

void opDevolver()
{
    titulo("DEVOLVER LIBRO");
    string cod = mayus(leerTexto("  Codigo del libro: "));
    Libro *l = buscarPorCodigo(cod);

    if (l == NULL)
    {
        error("No se encontro el libro.");
        return;
    }

    if (l->disponible)
    {
        error("Ese libro no esta prestado.");
        return;
    }

    apilar("DEVOLUCION", l->codigo, l->prestadoA.codigo, l->prestadoA.nombre, fechaActual());
    ok("Devuelto por " + l->prestadoA.nombre + ".");

    // Si hay estudiantes esperando, se presta al primero de la cola
    if (l->espera.frente != NULL)
    {
        Estudiante sig = desencolar(l);
        l->prestadoA = sig;
        apilar("PRESTAMO", l->codigo, sig.codigo, sig.nombre, fechaActual());
        ok("Prestado automaticamente al primero en espera: " + sig.nombre + ".");

        if (l->espera.frente != NULL)
        {
            color(14);
            cout << "  [AVISO] Quedan " << l->espera.cantidad << " estudiante(s) en espera.\n";
            color(7);
        }
    }
    else
    {
        l->disponible = true;
        l->prestadoA.codigo = "";
        l->prestadoA.nombre = "";
        ok("El libro vuelve a estar disponible.");
    }
}

void opVerEspera()
{
    titulo("LISTA DE ESPERA DE UN LIBRO");
    string cod = mayus(leerTexto("  Codigo del libro: "));
    Libro *l = buscarPorCodigo(cod);

    if (l == NULL)
    {
        error("No se encontro el libro.");
        return;
    }

    cout << "  " << l->codigo << " - " << l->titulo << "\n";
    mostrarCola(l);
}

void opBuscarEnEspera()
{
    titulo("BUSCAR ESTUDIANTE EN LISTAS DE ESPERA");
    string codEst = mayus(leerTexto("  Codigo del estudiante: "));

    Libro *aux = cabeza;
    int n = 0;

    while (aux != NULL)
    {
        int pos = buscarEnCola(aux, codEst);

        if (pos > 0)
        {
            cout << "    Libro " << aux->codigo << " (" << aux->titulo << "): posicion "
                 << pos << " de " << aux->espera.cantidad << "\n";
            n++;
        }

        aux = aux->sig;
    }

    if (n == 0)
        aviso("El estudiante no esta en ninguna lista de espera.");
}

void opPrestamosActivos()
{
    titulo("PRESTAMOS ACTIVOS");

    Libro *aux = cabeza;
    int n = 0;

    while (aux != NULL)
    {
        if (aux->disponible == false)
        {
            cout << left;
            cout << "    " << setw(8) << aux->codigo << " " << setw(28) << aux->titulo
                 << " -> [" << aux->prestadoA.codigo << "] " << aux->prestadoA.nombre
                 << "  (espera: " << aux->espera.cantidad << ")\n";
            n++;
        }

        aux = aux->sig;
    }

    if (n == 0)
        aviso("No hay prestamos activos.");
}

// ==================================
// GUARDAR Y CARGAR ARCHIVOS
// ==================================

bool guardarDatos()
{
    ofstream archLibros("libros.txt");
    ofstream archEspera("espera.txt");
    ofstream archHistorial("historial.txt");

    if (!archLibros || !archEspera || !archHistorial)
        return false;

    // Libros (un dato por linea) y su cola de espera
    Libro *aux = cabeza;

    while (aux != NULL)
    {
        archLibros << aux->codigo << "\n";
        archLibros << aux->titulo << "\n";
        archLibros << aux->autor << "\n";
        archLibros << aux->disponible << "\n";
        archLibros << aux->prestadoA.codigo << "\n";
        archLibros << aux->prestadoA.nombre << "\n";

        NodoCola *c = aux->espera.frente;

        while (c != NULL)
        {
            archEspera << aux->codigo << "\n";
            archEspera << c->est.codigo << "\n";
            archEspera << c->est.nombre << "\n";
            c = c->sig;
        }

        aux = aux->sig;
    }

    // Historial: se guarda desde el tope
    Movimiento *m = tope;

    while (m != NULL)
    {
        archHistorial << m->tipo << "\n";
        archHistorial << m->codLibro << "\n";
        archHistorial << m->codEst << "\n";
        archHistorial << m->nomEst << "\n";
        archHistorial << m->fecha << "\n";
        m = m->sig;
    }

    archLibros.close();
    archEspera.close();
    archHistorial.close();
    return true;
}

void cargarDatos(bool mostrarMensajes)
{
    ifstream archLibros("libros.txt");

    if (!archLibros)
    {
        if (mostrarMensajes)
            aviso("No hay datos guardados todavia.");
        return;
    }

    // Se vacian las estructuras antes de cargar
    liberarLista();
    liberarPila();

    string codigo, titulo, autor, disponible, codEst, nomEst;

    // 1. Libros
    while (getline(archLibros, codigo))
    {
        getline(archLibros, titulo);
        getline(archLibros, autor);
        getline(archLibros, disponible);
        getline(archLibros, codEst);
        getline(archLibros, nomEst);

        Libro *l = insertarLibro(codigo, titulo, autor);
        l->disponible = (disponible == "1");
        l->prestadoA.codigo = codEst;
        l->prestadoA.nombre = nomEst;
    }

    archLibros.close();

    // 2. Colas de espera
    ifstream archEspera("espera.txt");

    while (getline(archEspera, codigo))
    {
        Estudiante e;
        getline(archEspera, e.codigo);
        getline(archEspera, e.nombre);

        Libro *l = buscarPorCodigo(codigo);

        if (l != NULL)
            encolar(l, e);
    }

    archEspera.close();

    // 3. Historial
    // El archivo empieza por el tope, por eso cada movimiento
    // se agrega al fondo para que la pila no quede al reves
    ifstream archHistorial("historial.txt");
    string tipo;
    Movimiento *ultimo = NULL;

    while (getline(archHistorial, tipo))
    {
        Movimiento *nuevo = new Movimiento;
        nuevo->tipo = tipo;
        getline(archHistorial, nuevo->codLibro);
        getline(archHistorial, nuevo->codEst);
        getline(archHistorial, nuevo->nomEst);
        getline(archHistorial, nuevo->fecha);
        nuevo->sig = NULL;

        if (tope == NULL)
            tope = nuevo;
        else
            ultimo->sig = nuevo;

        ultimo = nuevo;
        totalMovimientos++;
    }

    archHistorial.close();

    if (mostrarMensajes)
    {
        color(10);
        cout << "  [OK] Datos cargados: " << totalLibros << " libro(s) y "
             << totalMovimientos << " movimiento(s).\n";
        color(7);
    }
}

// ==================================
// MENUS
// ==================================

void menuLibros()
{
    int op;

    do
    {
        system("cls");
        titulo("GESTION DE LIBROS  (Lista enlazada)");
        cout << "   1. Registrar libro\n";
        cout << "   2. Buscar libro por codigo\n";
        cout << "   3. Buscar libro por titulo\n";
        cout << "   4. Modificar datos de un libro\n";
        cout << "   5. Eliminar libro\n";
        cout << "   6. Mostrar todos los libros\n";
        cout << "   7. Mostrar libros disponibles\n";
        cout << "   0. Volver\n";
        op = leerEntero("\n  Opcion: ", 0, 7);

        switch (op)
        {
        case 1:
            opRegistrarLibro();
            break;
        case 2:
            opBuscarCodigo();
            break;
        case 3:
            opBuscarTitulo();
            break;
        case 4:
            opModificarLibro();
            break;
        case 5:
            opEliminarLibro();
            break;
        case 6:
            titulo("TODOS LOS LIBROS");
            mostrarLibros(false);
            break;
        case 7:
            titulo("LIBROS DISPONIBLES");
            mostrarLibros(true);
            break;
        }

        if (op != 0)
            pausa();

    } while (op != 0);
}

void menuPrestamos()
{
    int op;

    do
    {
        system("cls");
        titulo("PRESTAMOS Y LISTAS DE ESPERA  (Cola)");
        cout << "   1. Prestar libro\n";
        cout << "   2. Devolver libro\n";
        cout << "   3. Ver lista de espera de un libro\n";
        cout << "   4. Buscar estudiante en listas de espera\n";
        cout << "   5. Ver prestamos activos\n";
        cout << "   0. Volver\n";
        op = leerEntero("\n  Opcion: ", 0, 5);

        switch (op)
        {
        case 1:
            opPrestar();
            break;
        case 2:
            opDevolver();
            break;
        case 3:
            opVerEspera();
            break;
        case 4:
            opBuscarEnEspera();
            break;
        case 5:
            opPrestamosActivos();
            break;
        }

        if (op != 0)
            pausa();

    } while (op != 0);
}

void menuHistorial()
{
    int op;

    do
    {
        system("cls");
        titulo("HISTORIAL DE MOVIMIENTOS  (Pila)");
        cout << "   1. Ver historial completo\n";
        cout << "   2. Ver ultimo movimiento (tope)\n";
        cout << "   3. Buscar movimientos de un estudiante\n";
        cout << "   4. Eliminar ultimo registro del historial (pop)\n";
        cout << "   5. Vaciar historial\n";
        cout << "   0. Volver\n";
        op = leerEntero("\n  Opcion: ", 0, 5);

        switch (op)
        {
        case 1:
            titulo("HISTORIAL COMPLETO");
            mostrarPila();
            break;

        case 2:
            titulo("ULTIMO MOVIMIENTO");
            if (tope == NULL)
                aviso("El historial esta vacio.");
            else
                imprimirMovimiento(tope);
            break;

        case 3:
            titulo("MOVIMIENTOS DE UN ESTUDIANTE");
            buscarEnPila(mayus(leerTexto("  Codigo del estudiante: ")));
            break;

        case 4:
            titulo("ELIMINAR ULTIMO REGISTRO");
            if (tope == NULL)
            {
                aviso("El historial esta vacio.");
                break;
            }
            imprimirMovimiento(tope);
            if (leerEntero("  Confirmar (1 = Si, 0 = No): ", 0, 1) == 1)
            {
                desapilar();
                ok("Registro eliminado del historial.");
            }
            break;

        case 5:
            titulo("VACIAR HISTORIAL");
            if (leerEntero("  Se borraran todos los registros. Confirmar (1 = Si, 0 = No): ", 0, 1) == 1)
            {
                liberarPila();
                ok("Historial vaciado.");
            }
            break;
        }

        if (op != 0)
            pausa();

    } while (op != 0);
}

// ==================================
// PROGRAMA PRINCIPAL
// ==================================
int main()
{
    SetConsoleTitleA("Biblioteca - Control de Prestamos");
    cargarDatos(false); // carga automatica al iniciar

    int op;

    do
    {
        system("cls");
        color(11);
        cout << "\n  ##################################################\n";
        cout << "  #   SISTEMA DE CONTROL DE PRESTAMOS - BIBLIOTECA  #\n";
        cout << "  ##################################################\n";
        color(7);
        cout << "   Libros registrados: " << totalLibros
             << "   |   Movimientos: " << totalMovimientos << "\n\n";
        cout << "   1. Gestion de libros          (Lista enlazada)\n";
        cout << "   2. Prestamos y lista de espera (Cola)\n";
        cout << "   3. Historial de movimientos   (Pila)\n";
        cout << "   4. Guardar datos\n";
        cout << "   5. Cargar datos\n";
        cout << "   0. Guardar y salir\n";
        op = leerEntero("\n  Opcion: ", 0, 5);

        switch (op)
        {
        case 1:
            menuLibros();
            break;
        case 2:
            menuPrestamos();
            break;
        case 3:
            menuHistorial();
            break;
        case 4:
            if (guardarDatos())
                ok("Datos guardados en libros.txt, espera.txt e historial.txt.");
            else
                error("No se pudo guardar.");
            pausa();
            break;
        case 5:
            cargarDatos(true);
            pausa();
            break;
        }

    } while (op != 0);

    if (guardarDatos())
        ok("Datos guardados. Hasta pronto.");

    liberarLista(); // se libera toda la memoria antes de salir
    liberarPila();
    return 0;
}
