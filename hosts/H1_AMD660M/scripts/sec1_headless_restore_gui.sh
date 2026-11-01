#!/usr/bin/env bash
set +e

echo "Restoring graphical.target..."
sync

sudo systemctl set-default graphical.target

echo
echo "Default target:"
systemctl get-default

echo
echo "Rebooting back into KDE..."
sync
sudo reboot
