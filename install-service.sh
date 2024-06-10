#!/bin/bash

# Create a systemd service file
cat <<EOF > /etc/systemd/system/studer232-to-mqtt.service
[Unit]
Description=Studer232-to-MQTT Service
After=network.target

[Service]
ExecStart=$(pwd)/bin/studer232-to-mqtt
WorkingDirectory=$(pwd)/bin
Restart=always
User=${SUDO_USER}
StandardOutput=null
StandardError=null


[Install]
WantedBy=multi-user.target
EOF

# Reload systemd daemon
systemctl daemon-reload

# Enable and start the service
systemctl enable studer232-to-mqtt.service
systemctl start studer232-to-mqtt.service