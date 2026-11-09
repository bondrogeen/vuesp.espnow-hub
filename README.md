# Vuesp ESP-NOW Hub

ESP32-хаб для ESP-NOW-клиентов Vuesp. Всегда включён, принимает события от клиентов,
хранит очередь и передаёт их локальному `vuesp-hub` серверу по HTTP.

## Документация

Подробное описание протокола, очереди, настройки и ограничений — в [DOCS.md](./DOCS.md).

## Схема

```text
client (ESP32-C3) --ESP-NOW/VLP--> hub (ESP32) --HTTP/JSON--> vuesp-hub server
```

## Что делает хаб

- поднимает ESP-NOW на канале 6;
- принимает `HELLO`, `EVT`, `HB` и `ACK`;
- запоминает MAC и ID клиентов;
- отвечает на `HELLO` кадром `HELLO_ACK`;
- складывает события в очередь и отправляет JSON на hub-сервер;
- повторяет отправку до 3 раз, затем отбрасывает событие;
- раз в 5 секунд рассылает `HB`, чтобы клиенты находили хаб.

## Настройка

В `src/espnow/espnow.cpp`:

```cpp
#define ESPNOW_CHANNEL 6
#define HUB_SERVER_URL "http://192.168.1.10:3005/api/v1/espnow/event"
```

Канал должен совпадать у хаба и всех клиентов. Сервер `vuesp-hub` должен быть доступен
по HTTP и принимать `POST /api/v1/espnow/event`.

## Сборка

```bash
pio run -e esp32
pio run -e esp32 -t upload
pio device monitor -b 115200
```

## VLP/1

Хаб принимает текстовые кадры:

```text
VLP1|HELLO|src=8A1B2C3D|fw=0.1.0|board=c3|caps=btn,temp|ch=6
VLP1|EVT|src=8A1B2C3D|seq=42|up=123456|k=btn|gpio=4|val=1
VLP1|EVT|src=8A1B2C3D|seq=43|up=123456|k=temp|val=23.40|unit=C
```

Наружу хаб отправляет JSON:

```json
{"v":1,"hub":"A1B2C3D4","src":"8A1B2C3D","seq":42,"up":123456,"name":"btn","gpio":4,"value":1}
```

## Ограничения

- ESP-NOW не работает через роутер и интернет.
- Все устройства должны быть на одном Wi-Fi-канале.
- Для отправки в `vuesp-hub` хаб должен быть подключён к локальной сети по STA.
- Адрес сервера задаётся вручную в `HUB_SERVER_URL`.
