#!/bin/sh

# test existance of the /opt/login_alert directory
if [ ! -d /opt/login_alert ]; then
    echo "Directory /opt/login_alert does not exist. Creating it..."
    mkdir /opt/login_alert
fi

echo "PAM_USER: $PAM_USER"
echo "PAM_TYPE: $PAM_TYPE"
echo "PAM_RHOST: $PAM_RHOST"
echo "PAM_SERVICE: $PAM_SERVICE"
echo "PAM_TTY: $PAM_TTY"

# create a file with the login details
echo "PAM_USER: $PAM_USER" > /opt/login_alert/login_details.txt
echo "PAM_TYPE: $PAM_TYPE" >> /opt/login_alert/login_details.txt
echo "PAM_RHOST: $PAM_RHOST" >> /opt/login_alert/login_details.txt
echo "PAM_SERVICE: $PAM_SERVICE" >> /opt/login_alert/login_details.txt
echo "PAM_TTY: $PAM_TTY" >> /opt/login_alert/login_details.txt
