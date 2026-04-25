import os
import asyncio
import json
import aiohttp
import traceback

ANKI_COOKIE = os.environ.get("ANKI_COOKIE", "")
TOPIC_ANKI_STATS = "jidai/global/anki/telemetry"
DECKS_API_URL = "https://ankiweb.net/svc/decks/deck-list-info"

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
}

# --- МІКРО-ПАРСЕР PROTOBUF (Магія чистого Python) ---
def decode_varint(buffer, offset):
    result = 0
    shift = 0
    while True:
        if offset >= len(buffer): return result, offset
        byte = buffer[offset]
        offset += 1
        result |= (byte & 0x7f) << shift
        if not (byte & 0x80): break
        shift += 7
    return result, offset

def parse_protobuf(buffer):
    offset = 0
    length = len(buffer)
    fields = {}
    while offset < length:
        key, offset = decode_varint(buffer, offset)
        field_num = key >> 3
        wire_type = key & 0x07

        if wire_type == 0:
            val, offset = decode_varint(buffer, offset)
            fields.setdefault(field_num, []).append(val)
        elif wire_type == 1:
            offset += 8
        elif wire_type == 2:
            vlen, offset = decode_varint(buffer, offset)
            val = buffer[offset:offset+vlen]
            offset += vlen
            fields.setdefault(field_num, []).append(val)
        elif wire_type == 5:
            offset += 4
        else:
            break # Захист від битих байтів
    return fields
# ----------------------------------------------------

async def fetch_anki_stats():
    if not ANKI_COOKIE:
        raise ValueError("Не задано ANKI_COOKIE!")

    cookie_string = f"ankiweb={ANKI_COOKIE}; has_auth=1"
    request_headers = HEADERS.copy()
    request_headers["Cookie"] = cookie_string

    async with aiohttp.ClientSession(headers=request_headers) as session:
        async with session.post(DECKS_API_URL, data=b"") as resp:
            if resp.status != 200:
                raise Exception(f"HTTP Помилка {resp.status}")
            
            raw_bytes = await resp.read()

            # 1. Розбираємо головний об'єкт
            root_fields = parse_protobuf(raw_bytes)
            
            # 2. Розбираємо список колод
            decks_data = []
            total_due = 0
            total_new = 0
            
            decks_wrapper = root_fields.get(1, [b''])[0]
            wrapper_fields = parse_protobuf(decks_wrapper)
            
            # Кожна колода лежить в масиві поля 3
            for deck_buf in wrapper_fields.get(3, []):
                deck_fields = parse_protobuf(deck_buf)
                
                name = deck_fields.get(2, [b'Unknown'])[0].decode('utf-8', errors='ignore')
                review = deck_fields.get(6, [0])[0]
                learn = deck_fields.get(7, [0])[0]
                new = deck_fields.get(8, [0])[0]
                
                deck_due = review + learn
                deck_new = new
                
                total_due += deck_due
                total_new += deck_new
                
                decks_data.append({
                    "name": name,
                    "due": deck_due,
                    "new": deck_new
                })

            return {
                "totalDue": total_due,
                "totalNew": total_new,
                "totalRemaining": total_due + total_new,
                "decks": decks_data
            }

async def anki_scraper_task(mqtt_client):
    print("[Anki] Бінарний парсер Protobuf запущено!")
    await asyncio.sleep(5)
    
    while True:
        try:
            stats = await fetch_anki_stats()
            payload_json = json.dumps(stats, ensure_ascii=False)
            
            if mqtt_client:
                mqtt_client.publish(TOPIC_ANKI_STATS, payload_json, qos=1)
                print(f"[Anki] Статистика успішно відправлена: Залишилось {stats['totalRemaining']} карток!")
            
        except Exception as e:
            print(f"\n[Anki ERROR] {e}")
            traceback.print_exc()

        await asyncio.sleep(1800) # Раз на 30 хв

async def start_anki(mqtt_client_ref):
    await anki_scraper_task(mqtt_client_ref)