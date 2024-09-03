# Projekt i implementacja inteligentnego domu przy użyciu mikrokontrolerów

Repozytorium zawiera kod źródłowy modułów komunikujących się z serwerem przy pomocy protokołu MQTT.
- BME680 jest modułem zbierającym rozszerzone informacje o środowisku (temperatura, wilgotność, ciśnienie, poziom zanieczyszczenia powietrza)
- SHTC3 jest modułem zbierającym podstawowe informacje o środowisku (temperatura, wilgotność)
- Plant Overseer kontroluje wilgotność gleby rośliny doniczkowej
- Relay Module ma za zadanie włączać i wyłączać podłączone urządzenia w zależności od panujących warunków i/lub poleceń odbieranych z serwera
