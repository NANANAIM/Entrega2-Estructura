// Simulación de terminal tipo Unix usando árbol jerárquico dinámico.
// Usa <iostream> y, para persistencia, <fstream>.

#include <iostream>
#include <fstream>
#include "fs.h"
using namespace std;

static void imprimir_prompt(Nodo* cwd) {
	char* p = construir_ruta_absoluta(cwd);
	cout << p << " $ " << flush;  // Añadido flush para forzar salida inmediata
	delete[] p;
}

static void guardado_automatico(Nodo* raiz, const char* archivoAbierto, const char* rutaPorDefecto) {
	const char* ruta = archivoAbierto ? archivoAbierto : rutaPorDefecto;
	ofstream ofs(ruta, ios::out | ios::trunc);
	if (!ofs.is_open()) {
		cout << "Error: no se puede guardar en '" << ruta << "'\n";
		return;
	}
	serializar_arbol(raiz, ofs);
	ofs.close();
	// Guardado silencioso
}

static bool contieneBarra(const char* s) {
	if (!s) return false;
	for (int i = 0; s[i] != '\0'; ++i) if (s[i] == '/') return true;
	return false;
}

static Nodo* resolver_padre_para_nuevo(Nodo* raiz, Nodo* cwd, const char* ruta, char* nombreSalida, ostream& out) {
	// Devuelve el nodo padre y llena nombreSalida con el último componente; soporta abs/rel
	// Parsear hasta la última '/'
	int L = str_longitud(ruta);
	int i = L - 1; while (i >= 0 && ruta[i] == '/') --i; // recortar barras finales
	int fin = i;
	while (i >= 0 && ruta[i] != '/') --i;
	int inicioNombre = i + 1;
	int lenNombre = (fin - inicioNombre + 1);
	for (int k = 0; k < lenNombre && k < 255; ++k) nombreSalida[k] = ruta[inicioNombre + k];
	nombreSalida[(lenNombre < 256) ? lenNombre : 255] = '\0';
	// ruta padre
	char parentBuf[512];
	int pbLen = 0;
	if (i <= 0) { parentBuf[pbLen++] = '/'; }
	else {
		for (int k = 0; k < i && pbLen < 511; ++k) parentBuf[pbLen++] = ruta[k];
	}
	parentBuf[pbLen] = '\0';
	Nodo* padre = resolver_ruta(raiz, cwd, parentBuf, out);
	return padre;
}

int main() {
	// Desactivar el buffering de cout para que todo se muestre inmediatamente
	cout << unitbuf;
	
	// Crear directorio raíz '/'
	Nodo* raiz = crear_nodo(NODO_DIR, "", nullptr);
	Nodo* cwd = raiz;
	char* archivoAbierto = nullptr; // si se establece con 'open', se guarda al salir
	const char* rutaPorDefecto = "fs.txt"; // archivo de auto-persistencia en el directorio actual

	// Auto-cargar archivo por defecto si existe
	{
		ifstream ifs(rutaPorDefecto, ios::in);
		if (ifs.is_open()) {
			// Redirigir errores a cerr para no interferir con cout
			deserializar_arbol(raiz, ifs, cerr);
			ifs.close();
			archivoAbierto = str_duplicar(rutaPorDefecto);
		}
	}

	// Preparar entrada
	cin.clear();
	
	// Mostrar el primer prompt inmediatamente
	imprimir_prompt(cwd);

	char cmdline[1024];
	while (true) {
		if (!cin.getline(cmdline, 1024)) break;
		// recortar CR
		int len = str_longitud(cmdline); if (len > 0 && cmdline[len-1] == '\r') cmdline[len-1] = '\0';
		// saltar vacío
		if (cmdline[0] == '\0') {
			imprimir_prompt(cwd);
			continue;
		}
		// parsear comando y argumentos
		char cmd[32]; int ci = 0; int i = 0;
		while (cmdline[i] == ' ') ++i;
		while (cmdline[i] != '\0' && cmdline[i] != ' ' && ci < 31) { cmd[ci++] = cmdline[i++]; }
		cmd[ci] = '\0';
		while (cmdline[i] == ' ') ++i;
		char arg1[512]; int a1 = 0;
		while (cmdline[i] != '\0' && cmdline[i] != ' ' && a1 < 511) { arg1[a1++] = cmdline[i++]; }
		arg1[a1] = '\0';
		while (cmdline[i] == ' ') ++i;
		char arg2[512]; int a2 = 0;
		while (cmdline[i] != '\0' && a2 < 511) { arg2[a2++] = cmdline[i++]; }
		arg2[a2] = '\0';

		if (str_igual(cmd, "exit")) {
			// Si hay archivo abierto, guardar allí; si no, volcar a stdout
			if (archivoAbierto) {
				ofstream ofs(archivoAbierto, ios::out | ios::trunc);
				if (!ofs.is_open()) {
					cout << "Error: no se puede guardar en '" << archivoAbierto << "'\n";
				} else {
					serializar_arbol(raiz, ofs);
					ofs.close();
				}
			} else {
				serializar_arbol(raiz, cout);
			}
			break;
		} else if (str_igual(cmd, "ls")) {
			listar(cwd, cout);
		} else if (str_igual(cmd, "cd")) {
			if (arg1[0] == '\0') { cout << "Uso: cd <ruta>\n"; continue; }
			Nodo* dest = resolver_ruta(raiz, cwd, arg1, cout);
			if (!dest) continue;
			if (dest->tipo != NODO_DIR) { cout << "Error: no es directorio\n"; continue; }
			cwd = dest;
		} else if (str_igual(cmd, "mkdir")) {
			if (arg1[0] == '\0') { cout << "Uso: mkdir <nombre>\n"; continue; }
			// Si arg1 contiene '/', tratar como ruta y crear bajo su padre
			if (arg1[0] == '/' || contieneBarra(arg1)) {
				char nombre[256];
				Nodo* padre = resolver_padre_para_nuevo(raiz, cwd, arg1, nombre, cout);
				if (!padre) { cout << "Error: ruta inválida\n"; continue; }
				Nodo* creado = crear_directorio(padre, nombre, cout);
				if (creado) guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
			} else {
				Nodo* creado = crear_directorio(cwd, arg1, cout);
				if (creado) guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
			}
		} else if (str_igual(cmd, "touch")) {
			if (arg1[0] == '\0') { cout << "Uso: touch <nombre>\n"; continue; }
			if (arg1[0] == '/' || contieneBarra(arg1)) {
				char nombre[256];
				Nodo* padre = resolver_padre_para_nuevo(raiz, cwd, arg1, nombre, cout);
				if (!padre) { cout << "Error: ruta inválida\n"; continue; }
				Nodo* creado = crear_archivo(padre, nombre, cout);
				if (creado) guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
			} else {
				Nodo* creado = crear_archivo(cwd, arg1, cout);
				if (creado) guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
			}
		} else if (str_igual(cmd, "mv")) {
			if (arg1[0] == '\0' || arg2[0] == '\0') { cout << "Uso: mv <origen> <destino>\n"; continue; }
			Nodo* src = resolver_ruta(raiz, cwd, arg1, cout);
			if (!src) continue;
			Nodo* dst = resolver_ruta(raiz, cwd, arg2, cout);
			if (dst) {
				if (dst->tipo != NODO_DIR) { cout << "Error: destino no es directorio\n"; continue; }
				if (!mover_nodo(src, dst, nullptr, cout)) continue;
				guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
			} else {
				// Si el destino no existe, intentar como renombrado bajo su padre
				char nombre[256];
				Nodo* padre = resolver_padre_para_nuevo(raiz, cwd, arg2, nombre, cout);
				if (!padre) { cout << "Error: destino inválido\n"; continue; }
				if (!mover_nodo(src, padre, nombre, cout)) continue;
				guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
			}
		} else if (str_igual(cmd, "rename")) {
			if (arg1[0] == '\0' || arg2[0] == '\0') { cout << "Uso: rename <ruta> <nuevo_nombre>\n"; continue; }
			Nodo* tgt = resolver_ruta(raiz, cwd, arg1, cout);
			if (!tgt) continue;
			if (!nombre_valido(arg2)) { cout << "Nombre inválido\n"; continue; }
			if (tiene_hijo_llamado(tgt->padre ? tgt->padre : raiz, arg2)) { cout << "Colisión de nombre\n"; continue; }
			delete[] tgt->nombre; tgt->nombre = str_duplicar(arg2);
			guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
		} else if (str_igual(cmd, "edit")) {
			if (arg1[0] == '\0') { cout << "Uso: edit <ruta-archivo>\n"; continue; }
			Nodo* f = resolver_ruta(raiz, cwd, arg1, cout);
			if (!f) continue;
			if (f->tipo != NODO_ARCHIVO) { cout << "Error: no es archivo\n"; continue; }
			bool guardado = editar_archivo(f, cin, cout);
			// Independiente de :wq o :q!, guardar para minimizar pérdidas
			guardado_automatico(raiz, archivoAbierto, rutaPorDefecto);
		} else if (str_igual(cmd, "load")) {
			deserializar_arbol(raiz, cin, cout);
		} else if (str_igual(cmd, "open")) {
			if (arg1[0] == '\0') { cout << "Uso: open <ruta-archivo>\n"; continue; }
			ifstream ifs(arg1, ios::in);
			// Reiniciar árbol actual
			liberar_arbol(raiz);
			raiz = crear_nodo(NODO_DIR, "", nullptr);
			cwd = raiz;
			if (archivoAbierto) { delete[] archivoAbierto; archivoAbierto = nullptr; }
			archivoAbierto = str_duplicar(arg1);
			if (!ifs.is_open()) {
				// Si no existe, iniciar árbol vacío; se creará al salir
				cout << "Nuevo archivo: " << arg1 << "\n";
			} else {
				deserializar_arbol(raiz, ifs, cout);
				ifs.close();
				cout << "Abierto: " << arg1 << "\n";
			}
		} else {
			cout << "Comando desconocido: " << cmd << "\n"; //Muestrado mensaje para comandos no encontrados 
		}
		
		// Mostrar el prompt para el siguiente comando
		imprimir_prompt(cwd);
	}

	if (archivoAbierto) { delete[] archivoAbierto; }
	liberar_arbol(raiz);
	return 0;
}
