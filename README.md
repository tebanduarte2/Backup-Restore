# Proyecto de Respaldo en C++
Este proyecto es una aplicación de respaldo de archivos desarrollada en C++ que ofrece funcionalidades para respaldar y restaurar carpetas en diversas ubicaciones de almacenamiento.

## Características Principales
* Respaldo de Archivos: Permite seleccionar carpetas y guardarlas en:

  * Almacenamiento Local/Disco Duro (ZIP).

  * Almacenamiento en la Nube (vía API Flask a AWS S3).

  * Almacenamiento USB.

* Restauración de Archivos: Permite seleccionar respaldos existentes y descomprimirlos en un directorio local elegido por el usuario, desde:

  * Almacenamiento Local/Disco Duro (archivos ZIP).

  * Almacenamiento en la Nube (vía API Flask desde AWS S3).

  * Almacenamiento USB.

* Interfaz Gráfica Sencilla: Utiliza zenity para diálogos de selección de archivos/carpetas y mensajes al usuario.

* Paralelización: Aprovecha los algoritmos paralelos de C++17 para acelerar operaciones intensivas como la copia de archivos y la compresión.

## Estructura del Proyecto
El diseño del proyecto se basa en los patrones Estrategia y Fábrica para gestionar los diferentes tipos de almacenamiento de manera extensible.

### main.cpp:
* Punto de entrada de la aplicación.
* Maneja la interacción inicial con el usuario (elegir Respaldo o Restauración).
* Crea dinámicamente el manejador de almacenamiento (StorageHandler) apropiado.
* Inicializa y desinicializa la librería cURL globalmente.

### StorageHandler.h / StorageHandler.cpp:
* StorageHandler.h: Define la interfaz abstracta (StorageHandler) para cualquier tipo de almacenamiento, con métodos virtuales puros como validate(), backup(), restore() y getDescription().
* StorageHandler.cpp: Contiene la función fábrica createStorageHandler() que devuelve una instancia del manejador de almacenamiento concreto (Local, Nube, USB) según la elección del usuario.

### Clases de Almacenamiento (Implementaciones de StorageHandler):
* LocalStorage.h / LocalStorage.cpp: Implementa las operaciones de respaldo y restauración en el sistema de archivos local.

* CloudStorage.h / CloudStorage.cpp: Gestiona las operaciones de respaldo y restauración con un servicio en la nube (actualmente, a través de una API Flask que interactúa con AWS S3).

* UsbStorage.h / UsbStorage.cpp: Clases placeholder para implementaciones de respaldo y restauración en dispositivos USB.

### utils.h / utils.cpp:

* Contiene funciones de utilidad compartidas por los manejadores de almacenamiento.

* Incluye funciones para la interfaz gráfica (zenity), copia de directorios, compresión/descompresión ZIP, y selección de archivos/carpetas.

## Librerías Importantes
Las siguientes librerías son cruciales para el funcionamiento del proyecto:

* libzip: Realiza la compresión y descompresión de archivos en formato ZIP. Es esencial para empaquetar y desempaquetar los respaldos.
* libcurl: Actúa como un cliente HTTP para realizar peticiones web. CloudStorage lo utiliza para comunicarse con la API Flask (subir archivos ZIP y descargar respaldos).
* nlohmann/json: Librería "header-only" para parsear y generar datos JSON. Es utilizada por CloudStorage para interpretar las respuestas JSON recibidas de la API Flask (ej., la lista de archivos disponibles).
* std::filesystem (C++17): Proporciona funcionalidades para manipular el sistema de archivos (crear/eliminar directorios, trabajar con rutas de archivos, copiar archivos/directorios) de forma portable.
* std::execution (C++17 Parallel Algorithms): Permite la ejecución paralela de algoritmos de la librería estándar de C++ (como std::transform y std::for_each), lo que mejora el rendimiento en operaciones intensivas al aprovechar múltiples núcleos de CPU.
* zenity (Herramienta externa): Una herramienta de línea de comandos que facilita la creación de diálogos gráficos simples (selección de archivos/carpetas, mensajes informativos) para una interacción amigable con el usuario.

## Paralelización Implementada
La paralelización se ha utilizado en puntos clave para optimizar el rendimiento:
* LocalStorage::backup(): La copia de múltiples carpetas seleccionadas se ejecuta en paralelo. Esto se logra usando std::transform con la política de ejecución std::execution::par_unseq.
* utils::compress_folder(): La adición de archivos individuales al archivo ZIP durante la compresión se realiza en paralelo. Aquí se utiliza std::for_each con std::execution::par_unseq.

Para que la paralelización funcione, el compilador debe ser invocado con la bandera -fopenmp (para GCC/Clang), lo que activa el soporte para OpenMP, una de las tecnologías que subyacen a las políticas de ejecución paralela de C++17.
