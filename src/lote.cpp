#include "../define/lote.h"
#include "../define/cursor.h"
#include "../define/proceso.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <thread>

unsigned long Lote::tiempoTotal = 0;
std::map<std::string, bool> Lote::idsUsed;

Lote::Lote()
{ this->procActual = nullptr; }

// Constructor copy
Lote::Lote(const Lote& lote)
{
    this->id = lote.id;
    this->procPend = lote.procPend;
    this->procTerm = lote.procTerm;
    this->procActual = lote.procActual != nullptr
        ? new Proceso(lote.procActual->getId(),
                      lote.procActual->getNombre(),
                      lote.procActual->getOperacion(),
                      lote.procActual->getTiempoMax())
        : nullptr;
}

const Lote& Lote::operator=(const Lote &lote)
{
    this->id = lote.id;
    this->procPend = lote.procPend;
    this->procTerm = lote.procTerm;
    this->procActual = lote.procActual != nullptr
        ? new Proceso(lote.procActual->getId(),
                      lote.procActual->getNombre(),
                      lote.procActual->getOperacion(),
                      lote.procActual->getTiempoMax())
        : nullptr;

    return *this;
}

Lote::~Lote()
{ 
    if (this->procActual)
        delete this->procActual;
    this->procActual = nullptr;
}

std::vector<Proceso>& Lote::getProcesosPendientes() const
{ return this->getProcesosPendientes(); }

void Lote::printProcesosTerminados()
{
    for (size_t i = 0; i < this->procTerm.size(); ++i) {
        std::cout << "Proceso #" << i + 1 << std::endl
                  << "ID: " << this->procTerm[i].getId() << std::endl
                  << "Nombre: " << this->procTerm[i].getNombre() << std::endl
                  << "Operación: " << this->procTerm[i].getOperacion()
                  << "Resultado: " << this->procTerm[i].getResultado()
                  << std::endl
                  << "Tiempo: " << this->procTerm[i].getTiempoMax()
                  << std::endl << std::endl;
    }
}

Proceso* Lote::getProcesoActual() const
{ return this->procActual; }

const unsigned long& Lote::getId() const
{ return this->id; }

void Lote::setId(const unsigned long &id)
{ this->id = id; }

void Lote::iniciar()
{
    unsigned long proc = 0;
    unsigned long cont = 0;
    
    std::cout << Cursor::colorText(VERDE, "Lotes a capturar: ");
    std::cin >> proc;
    while (proc--)
        capturarProceso(++cont);
    ejecutarProcesos();
}

///////////////////////////////////////////////
/*          CREACIÓN DE PROCESO              */
///////////////////////////////////////////////
void Lote::capturarProceso(const unsigned long& cont)
{
    Proceso aux;
    // Captura de ID
    aux.setId(std::to_string(cont));
    // Captura de operación
    aux.setOperacion(generarOperacionProceso());
    // Captura de tiempo máximo
    aux.setTiempoMax(std::to_string(generarTiempoProceso()));
    this->procPend.push_back(aux);
}

///////////////////////////////////////////////
/*      GENERACIÓN DE DATOS ALEATORIOS       */
///////////////////////////////////////////////
std::string Lote::generarOperacionProceso()
{   
    const short numOp = 5;
    char op[numOp] = { '+', '-', '*', '/', '%' };
    std::string tmp = "";
    srand(time(NULL));

    tmp = std::to_string((rand() % 99) + 1);
    tmp += op[rand() % numOp];
    tmp += std::to_string((rand() % 99) + 1);
    return tmp;
}

unsigned long Lote::generarTiempoProceso()
{
    const short maxTmp = 5;
    unsigned long tmp = 1;
    srand(time(NULL));
    tmp = (rand() % maxTmp) + 1;
    return tmp;
}

///////////////////////////////////////////////
/*           CAPTURA DE CAMPOS               */
///////////////////////////////////////////////
void Lote::capturarCampo(std::string msj, std::string msjError,
                         Proceso& proc,
                         bool(Proceso::*metodo)(const std::string&,
                                                std::map<std::string,bool>*),
                         std::map<std::string, bool>* idsUsed)
{
    std::string aux;
    bool unaVez = false;
    std::cout << Cursor::Cursor::colorText(VERDE, msj);
    while(1) {
        std::getline(std::cin, aux);
        // Si el input es correcto rompemos el búcle infinito
        if ((proc.*metodo)(aux, idsUsed))
            break;
        if (!unaVez) {
            unaVez = true;
            Cursor::rmLine();
        }
        else
            Cursor::rmLine(2);
        std::cout << Cursor::Cursor::colorText(ROJO, msjError, true)
                  << std::endl;
        std::cout << Cursor::Cursor::colorText(VERDE, msj);
    }
}

void Lote::capturarCampo(std::string msj, std::string msjError,
                         Proceso& proc,
                         bool(Proceso::*metodo)(const std::string&))
{
    std::string aux;
    bool unaVez = false;
    std::cout << Cursor::colorText(VERDE, msj);
    while(1) {
        std::getline(std::cin, aux);
        // Si el input es correcto rompemos el búcle infinito
        if ((proc.*metodo)(aux))
            break;
        if (!unaVez) {
            unaVez = true;
            Cursor::rmLine();
        }
        else
            Cursor::rmLine(2);
        std::cout << Cursor::colorText(ROJO, msjError, true) << std::endl;
        std::cout << Cursor::colorText(VERDE, msj);
    }
}

void Lote::ejecutarProcesos()
{
    float lotes = (float)this->procPend.size() / BATCH_MAX_CAPACITY;
    unsigned long lotesRes = (lotes - (int)(lotes)) ? lotes : lotes + 1;
    unsigned long lotesTerm = 0;
    unsigned long cont = 0;
    Frame pendientes(1, 5, FIELD_WIDTH * 2, 15, AMARILLO);
    Frame actual(FIELD_WIDTH * 2 + 2, 5, FIELD_WIDTH * 5, 15, VERDE);
    Frame terminados(FIELD_WIDTH * 8 + 4, 5, FIELD_WIDTH * 4, 15, CYAN);

    Cursor::clrscr();
    imprimirVentanas(&pendientes, &actual, &terminados);
    terminados.print("-----------(lote ");
    terminados.print(std::to_string(++lotesTerm));
    terminados.print(")-----------");
    while (this->procPend.size()) {
        this->procActual = new Proceso(this->procPend.front());
        this->procPend.erase(this->procPend.begin());

        pendientes.rmContent();
        imprimirVentanas(&pendientes);
        for (size_t i = 0; i < this->procPend.size(); ++i)
            llenarMarco(pendientes, this->procPend[i], false, false);
        
        if (!(this->procTerm.size() % BATCH_MAX_CAPACITY)
                && this->procTerm.size()) {
            terminados.print("-----------(lote ");
            terminados.print(std::to_string(++lotesTerm));
            terminados.print(")-----------");
        }

        cont = this->procActual->getTiempoMax();
        while (cont--) {
            actual.rmContent();
            imprimirVentanas(nullptr, &actual);
            llenarMarco(actual, *this->procActual, true, false);

            Cursor::gotoxy(1, 2);
            Cursor::rmLine(2);
            std::cout << "Lotes pendientes: " << lotesRes << std::endl
                      << "Tiempo transcurrido: " << Lote::tiempoTotal;
          
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ++Lote::tiempoTotal;
            this->procActual->setTiempoRes(cont);
            this->procActual->setTiempoTrans(
                                       this->procActual->getTiempoTrans() + 1);
        }

        this->procActual->calculate();
        llenarMarco(terminados, *this->procActual, false, true);
        this->procTerm.push_back(*this->procActual);
        delete this->procActual; this->procActual = nullptr;
        if (!(this->procTerm.size() % BATCH_MAX_CAPACITY))
            lotesRes--;
    }
    pendientes.rmContent(); actual.rmContent();
    imprimirVentanas(&pendientes, &actual);
}

void Lote::llenarMarco(Frame& marco, Proceso& proc, bool actual, bool term)
{
    if (!actual && !term)
        marco.print(std::to_string(proc.getTiempoMax()), BLANCO, true,
                    FIELD_WIDTH);
    else if (actual || term) {
        marco.print(std::to_string(proc.getId()), BLANCO, false, FIELD_WIDTH);
        marco.print(proc.getOperacion(), BLANCO, false, FIELD_WIDTH);
        marco.print(std::to_string(proc.getTiempoMax()), BLANCO, false,
                    FIELD_WIDTH);
    }
    if (actual) {
        marco.print(std::to_string(proc.getTiempoRes()), BLANCO, false,
                    FIELD_WIDTH);
        marco.print(std::to_string(proc.getTiempoTrans()), BLANCO, true,
                    FIELD_WIDTH);
    }
    else if (term)
        marco.print(std::to_string(proc.getResultado()), BLANCO, true,
                    FIELD_WIDTH);
}

void Lote::imprimirVentanas(Frame* pend, Frame* act, Frame* term)
{
    if (pend) {
        pend->print("procesos      pendientes:", BLANCO, true);
        pend->print("TMPM    ", BLANCO, true);
    }
    if (act) {
        act->print("procesos actual:", BLANCO, true);
        act->print("ID      OP      TMPM    TMPR    TMPT    ",
                   BLANCO, true);
    }
    if (term){
        term->print("procesos terminados:", BLANCO, true);
        term->print("ID      OP      TMPM    RES     ", BLANCO, true);
    }
}
