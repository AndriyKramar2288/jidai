from telethon.sync import TelegramClient
from telethon.sessions import StringSession

print("Сайтік https://my.telegram.org/apps, звідти береш")
api_id = input("App api_id: ")
api_hash = input("App api_hash: ")

with TelegramClient(StringSession(), api_id, api_hash) as client:
    print("\n--- ОСЬ ТВІЙ SESSION STRING (СКОПІЮЙ ЙОГО) ---")
    print(client.session.save())
    print("----------------------------------------------\n")