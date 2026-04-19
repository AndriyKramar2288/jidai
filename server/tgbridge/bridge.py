import os
import asyncio
import json
from telethon import TelegramClient, events
from telethon.sessions import StringSession
import paho.mqtt.client as mqtt

# --- ЗМІННІ СЕРЕДОВИЩА ---
TG_API_ID = int(os.environ.get("TG_API_ID", 0))
TG_API_HASH = os.environ.get("TG_API_HASH", "")
TG_SESSION_STRING = os.environ.get("TG_SESSION_STRING", "")

MQTT_BROKER = os.environ.get("MQTT_BROKER", "mosquitto")
MQTT_PORT = int(os.environ.get("MQTT_PORT", 1883))
MQTT_USER = os.environ.get("MQTT_USER", "")
MQTT_PASSWORD = os.environ.get("MQTT_PASSWORD", "")

TOPIC_IN = "jidai/global/tgbridge/in/telemetry"
TOPIC_OUT = "jidai/global/tgbridge/out/cmd"

# --- ІНІЦІАЛІЗАЦІЯ TELEGRAM ---
client = TelegramClient(StringSession(TG_SESSION_STRING), TG_API_ID, TG_API_HASH)

# Фіксуємо головний асинхронний цикл подій для взаємодії між потоками
loop = asyncio.get_event_loop()

# --- ЛОГІКА MQTT ---
def on_connect(mqtt_client, userdata, flags, rc):
    print(f"З'єднано з MQTT (код {rc})")
    mqtt_client.subscribe(TOPIC_OUT)

def on_message(mqtt_client, userdata, msg):
    raw_payload = msg.payload.decode('utf-8')
    
    try:
        data = json.loads(raw_payload)
        
        target = data.get("target")
        text = data.get("text")
        
        if not target or not text:
            print("Помилка: JSON з Node-RED не містить 'target' або 'text'")
            return
            
        print(f"Node-RED -> Telegram (кому: {target}): {text}")
        
        # Створюємо асинхронну обгортку, яка виконається ТІЛЬКИ в головному потоці
        async def send_task():
            try:
                await client.send_message(target, text)
            except Exception as e:
                print(f"Помилка Telethon під час відправки: {e}")
        
        # Безпечно закидаємо цю задачу в головний loop
        asyncio.run_coroutine_threadsafe(send_task(), loop)
        
    except json.JSONDecodeError:
        print(f"Помилка: Отримано не JSON з Node-RED! Текст: {raw_payload}")
    except Exception as e:
        print(f"Критична помилка при парсингу: {e}")

mqtt_client = mqtt.Client(client_id="telegram-bridge")
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

# --- ЛОГІКА TELEGRAM ---
@client.on(events.NewMessage())
async def forward_to_mqtt(event):
    if event.is_channel and not event.is_group:
        return

    chat = await event.get_chat()
    sender = await event.get_sender()

    chat_id = event.chat_id
    chat_title = getattr(chat, 'title', None) or getattr(chat, 'first_name', 'Unknown')
    
    sender_id = event.sender_id
    sender_name = getattr(sender, 'first_name', 'Unknown') if sender else "Анонім/Система"
    sender_username = getattr(sender, 'username', None)

    text = event.message.text
    if not text:
        if event.message.media:
            text = f"[Медіа: {type(event.message.media).__name__}]"
        else:
            text = "[Порожньо]"

    payload_dict = {
        "chat_id": chat_id,
        "chat_title": chat_title,
        "sender_id": sender_id,
        "sender_name": sender_name,
        "sender_username": sender_username,
        "is_group": event.is_group,
        "is_private": event.is_private,
        "text": text
    }

    payload_json = json.dumps(payload_dict, ensure_ascii=False)
    
    mqtt_client.publish(TOPIC_IN, payload_json, qos=1)
    print(f"Telegram -> Node-RED: {payload_json}")

async def main():
    print("Бот стартує без консолі...")
    await client.start()
    await client.run_until_disconnected()

if __name__ == '__main__':
    # Використовуємо той самий зафіксований loop
    loop.run_until_complete(main())