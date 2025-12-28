/*
    Sistema de archivos jerárquico dinámico (árbol primer-hijo/siguiente-hermano).
    Sin contenedores STL, solo punteros crudos y buffers char.
*/
#ifndef FS_H
#define FS_H

#include <iostream>
using namespace std;

// Tipos de nodo
enum TipoNodo { NODO_DIR = 0, NODO_ARCHIVO = 1 };

// Línea de archivo
struct Linea {
    char* texto;
    Linea* siguiente;
};

// Nodo del árbol
struct Nodo {
    TipoNodo tipo;
    char* nombre;
    Nodo* padre;
    Nodo* primerHijo;
    Nodo* siguienteHermano;
    // Solo para archivos
    Linea* primeraLinea;
};

// ---- Utilidades de cadenas (sin <cstring>) ----
int str_longitud(const char* s);
bool str_igual(const char* a, const char* b);
int str_comparar(const char* a, const char* b); // compara lexicográficamente, devuelve -1/0/1
char* str_duplicar(const char* s);
bool nombre_valido(const char* s); // no vacío, sin '/'

// ---- Ayudas para nodos ----
Nodo* crear_nodo(TipoNodo t, const char* nombre, Nodo* padre);
void liberar_lineas(Linea* l);
void liberar_arbol(Nodo* raiz);
Nodo* buscar_hijo(Nodo* dir, const char* nombre);
bool tiene_hijo_llamado(Nodo* dir, const char* nombre);
void enlazar_hijo_al_frente(Nodo* padre, Nodo* hijo);
void desvincular_de_padre(Nodo* n); // quita n de la lista de hijos de su padre

// ---- Operaciones del sistema de archivos ----
Nodo* crear_directorio(Nodo* cwd, const char* nombre, ostream& out);
Nodo* crear_archivo(Nodo* cwd, const char* nombre, ostream& out);
void listar(Nodo* cwd, ostream& out);

// Mover/renombrar
bool es_ancestro(Nodo* ancestro, Nodo* n);
bool mover_nodo(Nodo* item, Nodo* nuevoPadre, const char* nuevoNombre, ostream& out);

// ---- Resolución de rutas ----
// Resuelve ruta absoluta o relativa. Devuelve Nodo* o nullptr si hay error.
Nodo* resolver_ruta(Nodo* raiz, Nodo* cwd, const char* ruta, ostream& out);

// Construye la ruta absoluta de un nodo (devuelve char* nuevo)
char* construir_ruta_absoluta(Nodo* n);

// ---- Editor de archivos ----
void imprimir_archivo(Nodo* f, ostream& out);
bool editar_archivo(Nodo* f, istream& in, ostream& out);

// ---- Persistencia ----
// Formato:
// D /ruta/absoluta
// F /ruta/absoluta N\n
// <N líneas de contenido>
bool serializar_arbol(Nodo* raiz, ostream& out);
bool deserializar_arbol(Nodo* raiz, istream& in, ostream& out);

// ---- Helper de IO ----
// getline seguro en un buffer nuevo (límite maxLen). Devuelve nullptr en EOF.
char* leer_linea_alloc(istream& in, int maxLen);

#endif // FS_H

// ========================= IMPLEMENTACIÓN =========================

int str_longitud(const char* s) {
    if (!s) return 0;
    int n = 0; while (s[n] != '\0') ++n; return n;
}

bool str_igual(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    int i = 0;
    while (a[i] && b[i]) { if (a[i] != b[i]) return false; ++i; }
    return a[i] == b[i];
}

int str_comparar(const char* a, const char* b) {
    if (a == b) return 0;
    if (!a) return -1; if (!b) return 1;
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        ++i;
    }
    if (a[i] == b[i]) return 0;
    return a[i] ? 1 : -1;
}

char* str_duplicar(const char* s) {
    int n = str_longitud(s);
    char* r = new char[n + 1];
    for (int i = 0; i < n; ++i) r[i] = s[i];
    r[n] = '\0';
    return r;
}

bool nombre_valido(const char* s) {
    if (!s) return false;
    int n = str_longitud(s);
    if (n == 0) return false;
    for (int i = 0; i < n; ++i) if (s[i] == '/') return false;
    return true;
}

Nodo* crear_nodo(TipoNodo t, const char* nombre, Nodo* padre) {
    Nodo* n = new Nodo();
    n->tipo = t;
    n->nombre = str_duplicar(nombre ? nombre : "");
    n->padre = padre;
    n->primerHijo = nullptr;
    n->siguienteHermano = nullptr;
    n->primeraLinea = nullptr;
    return n;
}

void liberar_lineas(Linea* l) {
    while (l) { Linea* nx = l->siguiente; if (l->texto) delete[] l->texto; delete l; l = nx; }
}

void liberar_arbol(Nodo* raiz) {
    if (!raiz) return;
    // Liberación postorden
    Nodo* ch = raiz->primerHijo;
    while (ch) { Nodo* nx = ch->siguienteHermano; liberar_arbol(ch); ch = nx; }
    if (raiz->primeraLinea) liberar_lineas(raiz->primeraLinea);
    if (raiz->nombre) delete[] raiz->nombre;
    delete raiz;
}

Nodo* buscar_hijo(Nodo* dir, const char* nombre) {
    if (!dir || dir->tipo != NODO_DIR) return nullptr;
    Nodo* c = dir->primerHijo;
    while (c) { if (str_igual(c->nombre, nombre)) return c; c = c->siguienteHermano; }
    return nullptr;
}

bool tiene_hijo_llamado(Nodo* dir, const char* nombre) { return buscar_hijo(dir, nombre) != nullptr; }

void enlazar_hijo_al_frente(Nodo* padre, Nodo* hijo) {
    hijo->padre = padre;
    hijo->siguienteHermano = padre->primerHijo;
    padre->primerHijo = hijo;
}

void desvincular_de_padre(Nodo* n) {
    if (!n || !n->padre) return;
    Nodo* p = n->padre;
    Nodo* c = p->primerHijo;
    Nodo* prev = nullptr;
    while (c) {
        if (c == n) {
            if (prev) prev->siguienteHermano = c->siguienteHermano; else p->primerHijo = c->siguienteHermano;
            c->siguienteHermano = nullptr;
            return;
        }
        prev = c; c = c->siguienteHermano;
    }
}

Nodo* crear_directorio(Nodo* cwd, const char* nombre, ostream& out) {
    if (!cwd || cwd->tipo != NODO_DIR) { out << "Error: directorio actual inválido\n"; return nullptr; }
    if (!nombre_valido(nombre)) { out << "Error: nombre inválido\n"; return nullptr; }
    if (tiene_hijo_llamado(cwd, nombre)) { out << "Error: ya existe en el directorio\n"; return nullptr; }
    Nodo* n = crear_nodo(NODO_DIR, nombre, cwd);
    enlazar_hijo_al_frente(cwd, n);
    return n;
}

Nodo* crear_archivo(Nodo* cwd, const char* nombre, ostream& out) {
    if (!cwd || cwd->tipo != NODO_DIR) { out << "Error: directorio actual inválido\n"; return nullptr; }
    if (!nombre_valido(nombre)) { out << "Error: nombre inválido\n"; return nullptr; }
    Nodo* existente = buscar_hijo(cwd, nombre);
    if (existente) {
        if (existente->tipo == NODO_ARCHIVO) return existente;
        out << "Error: existe una carpeta con ese nombre\n";
        return nullptr;
    }
    Nodo* n = crear_nodo(NODO_ARCHIVO, nombre, cwd);
    enlazar_hijo_al_frente(cwd, n);
    return n;
}

void listar(Nodo* cwd, ostream& out) {
    if (!cwd || cwd->tipo != NODO_DIR) { out << "Error: directorio actual inválido\n"; return; }
    Nodo* c = cwd->primerHijo;
    while (c) {
        out << c->nombre;
        if (c->tipo == NODO_DIR) out << "/";
        out << "\n";
        c = c->siguienteHermano;
    }
}

bool es_ancestro(Nodo* ancestro, Nodo* n) {
    Nodo* cur = n;
    while (cur) { if (cur == ancestro) return true; cur = cur->padre; }
    return false;
}

bool mover_nodo(Nodo* item, Nodo* nuevoPadre, const char* nuevoNombre, ostream& out) {
    if (!item || !nuevoPadre || nuevoPadre->tipo != NODO_DIR) { out << "Error: destino inválido\n"; return false; }
    if (es_ancestro(item, nuevoPadre)) { out << "Error: no se puede mover dentro de su subárbol\n"; return false; }
    const char* nombreFinal = nuevoNombre && nombre_valido(nuevoNombre) ? nuevoNombre : item->nombre;
    if (!nombre_valido(nombreFinal)) { out << "Error: nombre destino inválido\n"; return false; }
    if (tiene_hijo_llamado(nuevoPadre, nombreFinal)) { out << "Error: colisión de nombre en destino\n"; return false; }
    desvincular_de_padre(item);
    if (nuevoNombre && !str_igual(nuevoNombre, item->nombre)) { delete[] item->nombre; item->nombre = str_duplicar(nuevoNombre); }
    enlazar_hijo_al_frente(nuevoPadre, item);
    return true;
}

static Nodo* resolver_inicio(Nodo* raiz, Nodo* cwd, const char* ruta) {
    if (!ruta || ruta[0] == '\0') return cwd;
    if (ruta[0] == '/') return raiz;
    return cwd;
}

Nodo* resolver_ruta(Nodo* raiz, Nodo* cwd, const char* ruta, ostream& out) {
    Nodo* cur = resolver_inicio(raiz, cwd, ruta);
    if (!cur) { out << "Error: punto de inicio inválido\n"; return nullptr; }
    int i = 0;
    if (ruta && ruta[0] == '/') i = 1; // saltar '/'
    char token[256];
    while (ruta && ruta[i] != '\0') {
        // saltar '/' repetidos
        while (ruta[i] == '/') ++i;
        if (ruta[i] == '\0') break;
        int tlen = 0;
        while (ruta[i] != '\0' && ruta[i] != '/' && tlen < 255) { token[tlen++] = ruta[i++]; }
        token[tlen] = '\0';
        if (tlen == 0) continue;
        if (str_igual(token, ".")) {
            // quedarse
        } else if (str_igual(token, "..")) {
            if (cur->padre) cur = cur->padre; // root permanece
        } else {
            if (cur->tipo != NODO_DIR) { out << "Error: ruta atraviesa archivo\n"; return nullptr; }
            Nodo* nxt = buscar_hijo(cur, token);
            if (!nxt) { out << "Error: elemento no encontrado: " << token << "\n"; return nullptr; }
            cur = nxt;
        }
    }
    return cur;
}

char* construir_ruta_absoluta(Nodo* n) {
    if (!n) { char* r = str_duplicar("/"); return r; }
    // Contar profundidad y longitud total
    int profundidad = 0; int len = 1; // '/'
    Nodo* cur = n;
    while (cur && cur->padre) { len += str_longitud(cur->nombre) + 1; ++profundidad; cur = cur->padre; }
    if (profundidad == 0) { char* r = str_duplicar("/"); return r; }
    char* out = new char[len + 1];
    out[len] = '\0';
    // recolectar nodos hasta raíz
    Nodo** arr = new Nodo*[profundidad];
    cur = n; int idx = profundidad - 1;
    while (cur && cur->padre) { arr[idx--] = cur; cur = cur->padre; }
    int pos = 0; out[pos++] = '/';
    for (int k = 0; k < profundidad; ++k) {
        char* nm = arr[k]->nombre;
        int L = str_longitud(nm);
        for (int j = 0; j < L; ++j) out[pos++] = nm[j];
        if (k != profundidad - 1) out[pos++] = '/';
    }
    delete[] arr;
    out[pos] = '\0';
    return out;
}

void imprimir_archivo(Nodo* f, ostream& out) {
    if (!f || f->tipo != NODO_ARCHIVO) { out << "Error: no es archivo\n"; return; }
    int i = 1;
    Linea* l = f->primeraLinea;
    while (l) { out << i << ": " << l->texto << "\n"; l = l->siguiente; ++i; }
}

static Linea* obtener_linea_en(Linea* cabeza, int idx) {
    int i = 1; Linea* cur = cabeza; while (cur && i < idx) { cur = cur->siguiente; ++i; } return (i == idx) ? cur : nullptr;
}

bool editar_archivo(Nodo* f, istream& in, ostream& out) {
    if (!f || f->tipo != NODO_ARCHIVO) { out << "Error: no es archivo\n"; return false; }
    out << "Editor (:p mostrar, :a append, :i N, :r N, :d N, :wq guardar, :q! salir)\n";
    char buf[1024];
    while (true) {
        out << "> ";
        if (!in.getline(buf, 1024)) return false;
        if (buf[0] == ':' ) {
            if (str_igual(buf, ":p")) { imprimir_archivo(f, out); continue; }
            if (str_igual(buf, ":wq")) { return true; }
            if (str_igual(buf, ":q!")) { return false; }
            if (buf[1] == 'a') { // :a luego la siguiente línea para anexar
                out << "texto: ";
                char* t = leer_linea_alloc(in, 1024);
                if (!t) { out << "EOF\n"; continue; }
                Linea* nl = new Linea(); nl->texto = t; nl->siguiente = nullptr;
                if (!f->primeraLinea) f->primeraLinea = nl; else {
                    Linea* c = f->primeraLinea; while (c->siguiente) c = c->siguiente; c->siguiente = nl;
                }
                continue;
            }
            if (buf[1] == 'i') { // :i N luego la siguiente línea para insertar antes de N
                int N = 0; for (int i = 3; buf[i]; ++i) if (buf[i] >= '0' && buf[i] <= '9') { N = N*10 + (buf[i]-'0'); }
                if (N <= 0) { out << "N inválido\n"; continue; }
                out << "texto: "; char* t = leer_linea_alloc(in, 1024); if (!t) continue;
                Linea* nl = new Linea(); nl->texto = t; nl->siguiente = nullptr;
                if (N == 1) { nl->siguiente = f->primeraLinea; f->primeraLinea = nl; }
                else {
                    int i = 1; Linea* prev = f->primeraLinea; while (prev && i < N-1) { prev = prev->siguiente; ++i; }
                    if (!prev) { out << "línea fuera de rango\n"; delete[] t; delete nl; continue; }
                    nl->siguiente = prev->siguiente; prev->siguiente = nl;
                }
                continue;
            }
            if (buf[1] == 'r') { // :r N reemplaza N con la siguiente línea
                int N = 0; for (int i = 3; buf[i]; ++i) if (buf[i] >= '0' && buf[i] <= '9') { N = N*10 + (buf[i]-'0'); }
                Linea* tgt = obtener_linea_en(f->primeraLinea, N);
                if (!tgt) { out << "línea no existe\n"; continue; }
                out << "texto: "; char* t = leer_linea_alloc(in, 1024); if (!t) continue;
                if (tgt->texto) delete[] tgt->texto; tgt->texto = t; continue;
            }
            if (buf[1] == 'd') { // :d N eliminar
                int N = 0; for (int i = 3; buf[i]; ++i) if (buf[i] >= '0' && buf[i] <= '9') { N = N*10 + (buf[i]-'0'); }
                if (N <= 0) { out << "N inválido\n"; continue; }
                if (N == 1) { Linea* del = f->primeraLinea; if (del) { f->primeraLinea = del->siguiente; if (del->texto) delete[] del->texto; delete del; } continue; }
                int i2 = 1; Linea* prev = f->primeraLinea; while (prev && i2 < N-1) { prev = prev->siguiente; ++i2; }
                if (!prev || !prev->siguiente) { out << "línea no existe\n"; continue; }
                Linea* del = prev->siguiente; prev->siguiente = del->siguiente; if (del->texto) delete[] del->texto; delete del; continue;
            }
            out << "Comando desconocido\n"; continue;
        } else {
            out << "Use comandos ':'\n";
        }
    }
}

static Nodo* asegurar_directorio_absoluto(Nodo* raiz, const char* rutaAbs, ostream& out) {
    if (!raiz || !rutaAbs || rutaAbs[0] != '/') return nullptr;
    int i = 1; Nodo* cur = raiz;
    char token[256];
    while (rutaAbs[i] != '\0') {
        while (rutaAbs[i] == '/') ++i;
        if (rutaAbs[i] == '\0') break;
        int tlen = 0; while (rutaAbs[i] != '\0' && rutaAbs[i] != '/' && tlen < 255) { token[tlen++] = rutaAbs[i++]; }
        token[tlen] = '\0';
        if (tlen == 0) continue;
        Nodo* nxt = buscar_hijo(cur, token);
        if (!nxt) { nxt = crear_nodo(NODO_DIR, token, cur); enlazar_hijo_al_frente(cur, nxt); }
        cur = nxt;
    }
    return cur;
}

bool serializar_arbol(Nodo* raiz, ostream& out) {
    if (!raiz) return false;
    // DFS en preorden
    // Emitir directorios excepto la raíz, luego archivos
    struct Pila { Nodo* n; Pila* sig; };
    Pila* st = nullptr;
    auto push = [&](Nodo* x){ Pila* s = new Pila{ x, st }; st = s; };
    auto pop = [&](){ if (!st) return (Nodo*)nullptr; Pila* s = st; Nodo* x = s->n; st = s->sig; delete s; return x; };
    push(raiz);
    while (st) {
        Nodo* n = pop();
        if (n != raiz) {
            char* p = construir_ruta_absoluta(n);
            if (n->tipo == NODO_DIR) out << "D " << p << "\n";
            else {
                int cnt = 0; Linea* l = n->primeraLinea; while (l) { ++cnt; l = l->siguiente; }
                out << "F " << p << " " << cnt << "\n";
                l = n->primeraLinea; while (l) { out << l->texto << "\n"; l = l->siguiente; }
            }
            delete[] p;
        }
        // apilar hijos
        Nodo* c = n->primerHijo; while (c) { push(c); c = c->siguienteHermano; }
    }
    return true;
}

bool deserializar_arbol(Nodo* raiz, istream& in, ostream& out) {
    if (!raiz) return false;
    char line[1024];
    while (in.getline(line, 1024)) {
        if (line[0] == '\0') continue;
        char type = line[0];
        // Ignorar líneas que no sean entradas de datos
        if (!(type == 'D' || type == 'F')) continue;
        if (line[1] != ' ') { out << "Formato inválido\n"; return false; }
        // índice de inicio de ruta
        int i = 2; // después de "X "
        // leer ruta
        char path[512]; int plen = 0;
        while (line[i] && line[i] != ' ' && plen < 511) { path[plen++] = line[i++]; }
        path[plen] = '\0';
        if (path[0] != '/') { out << "Ruta inválida\n"; return false; }
        if (type == 'D') {
            asegurar_directorio_absoluto(raiz, path, out);
        } else if (type == 'F') {
            // leer N si está presente
            while (line[i] == ' ') ++i;
            int N = 0; while (line[i] >= '0' && line[i] <= '9') { N = N*10 + (line[i]-'0'); ++i; }
            // asegurar directorio padre
            // obtener ruta padre quitando el último componente
            int lastSlash = -1; for (int k = plen-1; k >=0; --k) { if (path[k] == '/') { lastSlash = k; break; } }
            char parentPath[512];
            if (lastSlash <= 0) { parentPath[0] = '/'; parentPath[1] = '\0'; }
            else {
                for (int k = 0; k < lastSlash; ++k) parentPath[k] = path[k];
                parentPath[lastSlash] = '\0';
            }
            Nodo* padre = asegurar_directorio_absoluto(raiz, parentPath, out);
            // nombre del hijo
            char nombre[256]; int nl = 0; for (int k = lastSlash+1; k < plen && nl < 255; ++k) nombre[nl++] = path[k]; nombre[nl] = '\0';
            if (!padre) { out << "Padre inválido\n"; return false; }
            Nodo* f = buscar_hijo(padre, nombre);
            if (!f) { f = crear_nodo(NODO_ARCHIVO, nombre, padre); enlazar_hijo_al_frente(padre, f); }
            // leer N líneas
            for (int j = 0; j < N; ++j) {
                char* t = leer_linea_alloc(in, 1024);
                if (!t) t = str_duplicar("");
                Linea* nl2 = new Linea(); nl2->texto = t; nl2->siguiente = nullptr;
                if (!f->primeraLinea) f->primeraLinea = nl2; else { Linea* c = f->primeraLinea; while (c->siguiente) c = c->siguiente; c->siguiente = nl2; }
            }
        } else {
            out << "Tipo desconocido\n"; return false;
        }
    }
    return true;
}

char* leer_linea_alloc(istream& in, int maxLen) {
    char* buf = new char[maxLen];
    if (!in.getline(buf, maxLen)) { delete[] buf; return nullptr; }
    // recortar CR final si existe (Windows) antes de duplicar
    int n = str_longitud(buf);
    if (n > 0 && buf[n-1] == '\r') buf[n-1] = '\0';
    char* r = str_duplicar(buf);
    delete[] buf;
    return r;
}
