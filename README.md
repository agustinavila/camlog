# CamLog

Programa en arduino para marcar lugares de interes con ubicaciones en gps.

## Descripción

El objetivo es recopilar datos para luego procesarlos y representarlos en un mapa, utilizando un dispositivo lo suficientemente pequeño para poder llevarlo sin inconvenientes. 
El programa fue diseñado para funcionar con un arduino pro micro, obtiene la ubicacion de un módulo GPS, y al pulsar diferentes combinaciones de botón (click, doble click, hold), guardar la ubicación con distintos flags.
La información se guarda en un módulo lector de SD por SPI.

## Requisitos

### Hardware

- Arduino Pro Micro
- Módulo GPS Serial
- Módulo lector de tarjetas SD
- led RGB
- Boton DIP

### Librerías

- Libreria para leer memorias SD
- TinyGPS++

## Conexión

Descripcion de las conexiones físicas.