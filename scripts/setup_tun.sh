#!/bin/bash
# setup_tun.sh - Configure l'IP et le routage pour une interface TUN Linux virtuelle
# Usage : sudo ./scripts/setup_tun.sh [nom_interface] [IP_locale]
# Exemple : sudo ./scripts/setup_tun.sh tun0 10.0.0.1

DEV="${1:-tun0}"
IP="${2:-10.0.0.1}"

# Si l'interface TUN n'est pas encore créée, lancez d'abord ipo_sound une fois
# ou créez-la avec ip tuntap.
if ! ip link show "$DEV" &>/dev/null; then
    echo "Device $DEV not found. Start ipo_sound first to create it, or run:"
    echo "  sudo ip tuntap add dev $DEV mode tun"
    exit 1
fi

sudo ip addr add "${IP}/24" dev "$DEV"
sudo ip link set "$DEV" up
echo "TUN $DEV up with $IP/24"
echo "Route: traffic to 10.0.0.0/24 goes via $DEV. Other machine can use 10.0.0.2/24."
