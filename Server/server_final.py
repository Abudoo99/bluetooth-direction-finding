from websocket_server import WebsocketServer

# Called when a new client connects
def new_client(client, server):
    print(f"New client connected: {client['id']}")
    # server.send_message_to_all("New client has joined")

# Called when a client disconnects
def client_left(client, server):
    print(f"Client disconnected: {client['id']}")

# Called when a client sends a message
def message_received(client, server, message):
    # print(f"Client({client['id']}) said: {message}")

    device_id, instance_id, rssi, azimuth, elevation, channel = get_tag_angles(message)
    print(f"{device_id} : Azimuth angle - {azimuth}\u00B0, Elevation angle - {elevation}\u00B0")



def get_tag_angles(buffer):
    # Split the buffer by commas
    components = buffer.split(',')

    # Extract each component
    device_id_str = components[0].strip()
    instance_id_str = components[1].strip()
    rssi_str = components[2].strip()
    azimuth_str = components[3].strip()
    elevation_str = components[4].strip()
    channel_str = components[5].strip()

    # Filter out non-digit characters except for '-' where appropriate
    device_id_str = ''.join(c for c in device_id_str if c.isdigit() or c == '-')
    instance_id_str = ''.join(c for c in instance_id_str if c.isdigit() or c == '-')
    rssi_str = ''.join(c for c in rssi_str if c.isdigit() or c == '-')
    azimuth_str = ''.join(c for c in azimuth_str if c.isdigit() or c == '-')
    elevation_str = ''.join(c for c in elevation_str if c.isdigit() or c == '-')
    channel_str = ''.join(c for c in channel_str if c.isdigit() or c == '-')

    # Convert the cleaned strings to integers
    rssi = int(rssi_str)
    azimuth = int(azimuth_str)
    elevation = int(elevation_str)
    channel = int(channel_str)

    # Return all the extracted values
    return device_id_str, instance_id_str, rssi, azimuth, elevation, channel

def main():
    # Create and start the server
    server = WebsocketServer(host='0.0.0.0', port=8080)
    server.set_fn_new_client(new_client)
    server.set_fn_client_left(client_left)
    server.set_fn_message_received(message_received)
    server.run_forever()

if __name__ == "__main__":
    main()


   
