import os
import asyncio
import json
import paho.mqtt.client as mqtt

# Імпортуємо наші модулі
from tg_bridge import start_telegram, send_tg_message
from anki_bridge import start_anki

# --- ЗМІННІ СЕРЕДОВИЩА MQTT ---
MQTT_BROKER = os.environ.get("MQTT_BROKER", "mosquitto")
MQTT_PORT = int(os.environ.get("MQTT_PORT", 1883))
MQTT_USER = os.environ.get("MQTT_USER", "")
MQTT_PASSWORD = os.environ.get("MQTT_PASSWORD", "")

TOPIC_OUT = "jidai/global/tgbridge/out/cmd"

# Фіксуємо головний цикл подій
loop = asyncio.get_event_loop()

# --- ЛОГІКА MQTT ---
def on_connect(mqtt_client, userdata, flags, rc):
    print(f"[Main] З'єднано з MQTT (код {rc})")
    mqtt_client.subscribe(TOPIC_OUT)

def on_message(mqtt_client, userdata, msg):
    raw_payload = msg.payload.decode('utf-8')
    try:
        data = json.loads(raw_payload)
        
        # Обробка команд для Telegram
        if msg.topic == TOPIC_OUT:
            target = data.get("target")
            text = data.get("text")
            
            if not target or not text:
                print("[Main] Помилка: JSON для Telegram не містить 'target' або 'text'")
                return
                
            print(f"[Main] Node-RED -> Telegram (кому: {target}): {text}")
            
            # Безпечно передаємо задачу відправки в асинхронний цикл
            asyncio.run_coroutine_threadsafe(send_tg_message(target, text), loop)
            
    except json.JSONDecodeError:
        print(f"[Main] Помилка: Отримано не JSON! Текст: {raw_payload}")
    except Exception as e:
        print(f"[Main] Критична помилка при парсингу MQTT: {e}")

async def main():
    print("[Main] Запуск python-automation-worker...")
    
    # Ініціалізація MQTT
    mqtt_client = mqtt.Client(client_id="python-automation-worker")
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start() # Запускає фоновий потік для MQTT

    # Запуск модулів як окремих фонових задач у спільному event loop
    tg_task = loop.create_task(start_telegram(mqtt_client))
    anki_task = loop.create_task(start_anki(mqtt_client))
    
    # Чекаємо на їх виконання (вони працюватимуть нескінченно)
    await asyncio.gather(tg_task, anki_task)

if __name__ == '__main__':
    loop.run_until_complete(main())