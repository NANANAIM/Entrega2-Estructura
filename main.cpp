#include <iostream>
#include <string>

int main() {
    std::string entrada;
    int elegir;
    
    std::cout << "\n-PC:~$ ";
    std::cin >> entrada;
    
    //(case-sensitive)
    if (entrada == "1" || entrada == "cd") elegir = 1;
    else if (entrada == "2" || entrada == "ls") elegir = 2;
    else if (entrada == "3" || entrada == "mkdir") elegir = 3;
    else if (entrada == "4" || entrada == "mv") elegir = 4;
    else if (entrada == "5" || entrada == "touch") elegir = 5;
    else if (entrada == "6" || entrada == "exit:") elegir = 6;
    else if (entrada == "help") {
        std::cout << "Comands: cd, ls, mkdir, mv, touch, exit" << std::endl;
        main;
    }
    else if (entrada == "cow say") {
        std::cout << "moo" << std::endl;
        return 0;
    }
    else {
        std::cout << "Comando no valido." << std::endl;
        return 0;
    }
    

    switch (elegir) {
        case 1:
            // ... añadir codigo
            break;
        case 2:
            // ... añadir codigo
            break;
        case 3:
            // ... añadir codigo
            break;
       case 4:
            // ... añadir codigo
            break;
       case 5:
            // ... añadir codigo
            break;
        case 6:
            std::cout << "Saliendo..." << std::endl;
            return -1;
    }
    
    return elegir;
}


