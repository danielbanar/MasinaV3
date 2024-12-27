#!/bin/bash

# Configuration
CONFIG_FILE="/etc/wireguard.conf"
CONFIG_TXT_FILE="/root/config.txt"

# Read the hostname from /root/config.txt
ENDPOINT_HOSTNAME=$(grep -E '^hostname=' "$CONFIG_TXT_FILE" | cut -d'=' -f2)
ENDPOINT_HOST=$(grep -E '^host=' "$CONFIG_TXT_FILE" | cut -d'=' -f2)

if [ -z "$ENDPOINT_HOSTNAME" ]; then
    echo "Error: No hostname found in $CONFIG_TXT_FILE."
    exit 1
fi

if [ -z "$ENDPOINT_HOST" ]; then
    echo "Error: No host found in $CONFIG_TXT_FILE."
    exit 1
fi

# Resolve hostname to IP using nslookup
RESOLVED_IP=$(nslookup "$ENDPOINT_HOSTNAME" | awk '/^Address: / { print $2 }' | tail -n1)
if [ -z "$RESOLVED_IP" ]; then
    echo "Error: Failed to resolve hostname $ENDPOINT_HOSTNAME to an IP address using nslookup."
    exit 1
fi

# Update WireGuard configuration
if [ -f "$CONFIG_FILE" ]; then
    sed -i -E "s|^(Endpoint = )[0-9.]+(:[0-9]+)$|\1$RESOLVED_IP\2|" "$CONFIG_FILE"
    echo "Updated $CONFIG_FILE with IP $RESOLVED_IP."
else
    echo "Error: WireGuard configuration file $CONFIG_FILE does not exist."
    exit 1
fi

# Run yaml-cli command
yaml-cli -s .outgoing.server udp://"$ENDPOINT_HOST":2222
if [ $? -eq 0 ]; then
    echo "yaml-cli command executed successfully."
else
    echo "Error: Failed to execute yaml-cli command."
    exit 1
fi
