import asyncio
from bleak import BleakClient, BleakError

address = "08:F9:E0:F6:46:7E"  # Replace with your ESP32's MAC address
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

async def run(address):
    async with BleakClient(address, timeout=60.0) as client:
        try:
            x = await client.is_connected()
            print("Connected: {0}".format(x))

            def notification_handler(sender, data):
                print("Received data: {0}".format(data.decode('utf-8')))

            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
            await asyncio.sleep(60.0)
            await client.stop_notify(CHARACTERISTIC_UUID)

        except BleakError as e:
            print("Failed to connect: {}".format(e))

loop = asyncio.get_event_loop()
loop.run_until_complete(run(address))
