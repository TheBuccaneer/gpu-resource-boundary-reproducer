#!/usr/bin/env bash
set +e

echo "SEC1 GUI-LESS CONTROL"
echo "Switching next boot to multi-user.target."
echo "Save all work before continuing."
echo
read -r -p "Press ENTER to reboot without graphical desktop..."

sync
sudo systemctl set-default multi-user.target

echo
echo "Default target:"
systemctl get-default

sync
sudo reboot
