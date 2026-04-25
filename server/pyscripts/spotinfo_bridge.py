import os
import asyncio
import json
import io
import traceback
import aiohttp
from colorthief import ColorThief

# --- ЗМІННІ СЕРЕДОВИЩА LAST.FM ---
LASTFM_API_KEY = os.environ.get("LASTFM_API_KEY", "")
LASTFM_USER = os.environ.get("LASTFM_USER", "")

TOPIC_SPOTIFY_STATS = "jidai/global/spotify/telemetry"

_last_track_id = None
_last_color = {"hex": "#000000", "rgb": [0, 0, 0]}

async def get_dominant_color(session, image_url):
    """Завантажує картинку в пам'ять і витягує домінантний колір"""
    try:
        if not image_url: 
            return {"hex": "#ffffff", "rgb": [255, 255, 255]}
            
        async with session.get(image_url) as resp:
            if resp.status != 200:
                return {"hex": "#ffffff", "rgb": [255, 255, 255]}
            
            img_bytes = await resp.read()
            
            # ColorThief блокує потік, запускаємо асинхронно
            def extract_color():
                with io.BytesIO(img_bytes) as f:
                    cf = ColorThief(f)
                    return cf.get_color(quality=1) # RGB tuple
                    
            r, g, b = await asyncio.to_thread(extract_color)
            return {
                "hex": f"#{r:02x}{g:02x}{b:02x}",
                "rgb": [r, g, b]
            }
    except Exception as e:
        print(f"[LastFM] Помилка отримання кольору: {e}")
        return {"hex": "#ffffff", "rgb": [255, 255, 255]}

async def fetch_lastfm_state(session):
    """Тягне інфу про поточний трек через відкрите API Last.fm"""
    global _last_track_id, _last_color
    
    if not LASTFM_API_KEY or not LASTFM_USER:
        raise ValueError("Бракує LASTFM_API_KEY або LASTFM_USER")

    # API запит на 1 останній трек
    url = f"http://ws.audioscrobbler.com/2.0/?method=user.getrecenttracks&user={LASTFM_USER}&api_key={LASTFM_API_KEY}&format=json&limit=1"
    
    async with session.get(url) as resp:
        if resp.status != 200:
            raise Exception(f"HTTP {resp.status}: {await resp.text()}")
            
        data = await resp.json()
        tracks = data.get("recenttracks", {}).get("track", [])
        
        if not tracks:
            return {"is_playing": False, "status": "no_tracks"}
            
        track = tracks[0]
        
        # Last.fm додає атрибут nowplaying="true" тільки якщо трек грає ПРЯМО ЗАРАЗ
        is_playing = track.get("@attr", {}).get("nowplaying") == "true"
        
        if not is_playing:
            return {"is_playing": False, "status": "idle"}
            
        track_name = track.get("name")
        artist = track.get("artist", {}).get("#text")
        album = track.get("album", {}).get("#text")
        
        # У Last.fm не завжди є mbid, тому робимо унікальний ID з імені та артиста
        track_id = track.get("mbid") or f"{artist}-{track_name}"
        
        # Витягуємо найбільшу картинку (extralarge)
        images = track.get("image", [])
        image_url = ""
        for img in images:
            if img.get("size") == "extralarge" and img.get("#text"):
                image_url = img.get("#text")
                break
        
        # Розраховуємо колір ТІЛЬКИ якщо трек змінився
        if is_playing and image_url and track_id != _last_track_id:
            _last_color = await get_dominant_color(session, image_url)
            _last_track_id = track_id

        return {
            "is_playing": is_playing,
            "track_id": track_id,
            "track_name": track_name,
            "artists": artist,
            "album": album,
            "cover_url": image_url,
            "color": _last_color
        }

async def spotinfo_task(mqtt_client):
    print("[LastFM] Bridge запущено. Очікування треків зі Spotify...")
    await asyncio.sleep(2)
    
    async with aiohttp.ClientSession() as session:
        while True:
            try:
                state = await fetch_lastfm_state(session)
                
                payload_json = json.dumps(state, ensure_ascii=False)
                if mqtt_client:
                    mqtt_client.publish(TOPIC_SPOTIFY_STATS, payload_json, qos=0)
                
            except Exception as e:
                print(f"\n[LastFM ERROR] {e}")
                traceback.print_exc()

            # Стукаємо кожні 3 секунди (Last.fm дозволяє до 5 запитів на секунду, тому це абсолютно безпечно)
            await asyncio.sleep(3)

async def start_spotinfo(mqtt_client_ref):
    await spotinfo_task(mqtt_client_ref)