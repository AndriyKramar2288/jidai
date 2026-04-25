import os
import asyncio
import json
from telethon import TelegramClient, events
from telethon.sessions import StringSession
from telethon.tl.types import (
    UserStatusOnline, 
    UserStatusOffline
)

# --- ЗМІННІ СЕРЕДОВИЩА TELEGRAM ---
TG_API_ID = int(os.environ.get("TG_API_ID", 0))
TG_API_HASH = os.environ.get("TG_API_HASH", "")
TG_SESSION_STRING = os.environ.get("TG_SESSION_STRING", "")

TOPIC_IN = "jidai/global/tgbridge/in/telemetry"
TOPIC_STATUS = "jidai/global/tgbridge/status/telemetry" 

client = TelegramClient(StringSession(TG_SESSION_STRING), TG_API_ID, TG_API_HASH)

# Локальне посилання на MQTT клієнт
_mqtt_client = None

# Функція для експорту в main.py
async def send_tg_message(target, text):
    try:
        await client.send_message(target, text)
    except Exception as e:
        print(f"[Telegram] Помилка відправки: {e}")

@client.on(events.NewMessage())
async def forward_to_mqtt(event):
    if not _mqtt_client: return
    if event.is_channel and not event.is_group: return

    chat = await event.get_chat()
    sender = await event.get_sender()

    chat_title = getattr(chat, 'title', None) or getattr(chat, 'first_name', 'Unknown')
    sender_name = getattr(sender, 'first_name', 'Unknown') if sender else "Анонім/Система"
    
    text = event.message.text
    if not text:
        text = f"[Медіа: {type(event.message.media).__name__}]" if event.message.media else "[Порожньо]"

    payload_json = json.dumps({
        "chat_id": event.chat_id,
        "chat_title": chat_title,
        "sender_id": event.sender_id,
        "sender_name": sender_name,
        "sender_username": getattr(sender, 'username', None),
        "is_group": event.is_group,
        "is_private": event.is_private,
        "text": text
    }, ensure_ascii=False)
    
    _mqtt_client.publish(TOPIC_IN, payload_json, qos=1)
    print(f"[Telegram] Вхідне повідомлення відправлено в MQTT")

async def status_monitor():
    while True:
        try:
            me = await client.get_me()
            status = me.status

            status_data = {
                "username": me.username,
                "is_online": False,
                "status_type": type(status).__name__,
                "last_seen_ts": None,
                "expires_ts": None
            }

            if isinstance(status, UserStatusOnline):
                status_data["is_online"] = True
                if status.expires:
                    status_data["expires_ts"] = int(status.expires.timestamp() * 1000)
                    
            elif isinstance(status, UserStatusOffline):
                if status.was_online:
                    status_data["last_seen_ts"] = int(status.was_online.timestamp() * 1000)

            if _mqtt_client:
                payload_json = json.dumps(status_data, ensure_ascii=False)
                _mqtt_client.publish(TOPIC_STATUS, payload_json, qos=1)
                
        except Exception as e:
            print(f"[Telegram] Помилка в моніторі статусу: {e}")

        await asyncio.sleep(30)

async def start_telegram(mqtt_client_ref):
    global _mqtt_client
    _mqtt_client = mqtt_client_ref
    
    print("[Telegram] Ініціалізація клієнта...")
    await client.start()
    
    # Запускаємо фоновий моніторинг статусу
    asyncio.create_task(status_monitor())
    
    # Тримаємо підключення активним
    await client.run_until_disconnected()