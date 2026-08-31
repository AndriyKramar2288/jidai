import os
import json
import asyncio
import traceback
import decimal
from datetime import datetime, timezone
from aiohttp import web
from actual import Actual
from actual.queries import reconcile_transaction

MONO_WEBHOOK_SECRET = os.environ.get("MONO_WEBHOOK_SECRET", "")
MONO_ACC_MAP = json.loads(os.environ.get("MONO_ACC_MAP", "{}"))

ACTUAL_URL = os.environ.get("ACTUAL_URL", "http://actual:5006")
ACTUAL_PASSWORD = os.environ.get("ACTUAL_PASSWORD", "")
ACTUAL_SYNC_ID = os.environ.get("ACTUAL_SYNC_ID", "")


def push_to_actual(item: dict, actual_acc_id: str):
    """Синхронний виклик actualpy — виконується в окремому потоці."""
    try:
        with Actual(base_url=ACTUAL_URL, password=ACTUAL_PASSWORD, file=ACTUAL_SYNC_ID) as act:
            act.download_budget()

            date = datetime.fromtimestamp(item["time"], tz=timezone.utc).date()
            amount = decimal.Decimal(item["amount"]) / 100  # копійки -> гривні

            # reconcile_transaction сам знайде дубль по imported_id (financial_id),
            # тож повторний виклик вебхука (ретрай від Монобанку) не задвоїть запис
            reconcile_transaction(
                act.session,
                date,
                actual_acc_id,
                payee=item.get("description") or "Monobank",
                amount=amount,
                imported_id=item["id"],
                cleared=True,
            )
            act.commit()
            print(f"[Mono] Додано транзакцію: {item.get('description')} ({amount})")
    except Exception:
        print("[Mono ERROR] Помилка запису в Actual:")
        traceback.print_exc()


async def handle_mono_webhook(request: web.Request):
    if MONO_WEBHOOK_SECRET and request.match_info.get("secret") != MONO_WEBHOOK_SECRET:
        return web.Response(status=404, text="Not Found")

    try:
        data = await request.json()
    except Exception:
        # монобанк при підключенні вебхука шле тестовий запит — не валимо його
        return web.Response(text="OK")

    if data.get("type") != "StatementItem":
        return web.Response(text="OK")

    item = data.get("data", {}).get("statementItem")
    mono_acc = data.get("data", {}).get("account")
    if not item or not mono_acc:
        return web.Response(text="OK")

    actual_acc = MONO_ACC_MAP.get(mono_acc)
    if not actual_acc:
        print(f"[Mono] Ігнорую картку: {mono_acc}")
        return web.Response(text="Account ignored")

    await asyncio.to_thread(push_to_actual, item, actual_acc)
    return web.Response(text="OK")


async def start_mono_server(mqtt_client):
    app = web.Application()
    path = f"/mono/{MONO_WEBHOOK_SECRET}" if MONO_WEBHOOK_SECRET else "/mono"
    app.router.add_post(path, handle_mono_webhook)

    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, "0.0.0.0", 8080)
    await site.start()
    print(f"[Mono] Webhook міст слухає на порту 8080, шлях {path}")

    await asyncio.Event().wait()  # тримаємо задачу живою, як і решту мостів