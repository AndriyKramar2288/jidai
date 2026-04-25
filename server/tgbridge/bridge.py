import os
import asyncio
import json
from telethon import TelegramClient, events
from telethon.sessions import StringSession
from telethon.tl.types import (
    UserStatusOnline, 
    UserStatusOffline, 
    UserStatusRecently, 
    UserStatusLastWeek, 
    UserStatusLastMonth
)
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
TOPIC_STATUS = "jidai/global/tgbridge/status/telemetry" 

# --- ІНІЦІАЛІЗАЦІЯ TELEGRAM ---
client = TelegramClient(StringSession(TG_SESSION_STRING), TG_API_ID, TG_API_HASH)

# Фіксуємо головний асинхронний цикл подій
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
        
        async def send_task():
            try:
                await client.send_message(target, text)
            except Exception as e:
                print(f"Помилка Telethon під час відправки: {e}")
        
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

# --- ЛОГІКА TELEGRAM (ВХІДНІ ПОВІДОМЛЕННЯ) ---
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

# --- ФОНОВИЙ МОНІТОР СТАТУСУ ---
async def status_monitor():
    while True:
        try:
            # Отримуємо свій власний профіль
            me = await client.get_me()
            status = me.status

            # Базовий шаблон JSON
            status_data = {
                "username": me.username,
                "is_online": False,
                "status_type": type(status).__name__,
                "last_seen_ts": None,
                "expires_ts": None
            }

            # Аналізуємо тип статусу
            if isinstance(status, UserStatusOnline):
                status_data["is_online"] = True
                if status.expires:
                    status_data["expires_ts"] = int(status.expires.timestamp() * 1000)
                    
            elif isinstance(status, UserStatusOffline):
                # Коли був востаннє
                if status.was_online:
                    status_data["last_seen_ts"] = int(status.was_online.timestamp() * 1000)
                    
            # Якщо статус прихований (UserStatusRecently, UserStatusLastWeek тощо),
            # is_online залишається False, а type передасть Node-RED, що саме приховано.

            payload_json = json.dumps(status_data, ensure_ascii=False)
            mqtt_client.publish(TOPIC_STATUS, payload_json, qos=1)
            
        except Exception as e:
            print(f"Помилка в моніторі статусу: {e}")

        # Перевіряємо статус кожні 30 секунд
        await asyncio.sleep(30) 

# --- СТАРТ ---
async def main():
    print("Бот стартує без консолі...")
    await client.start()
    loop.create_task(status_monitor())
    await client.run_until_disconnected()

if __name__ == '__main__':
    loop.run_until_complete(main())