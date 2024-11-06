#!/bin/bash

 cd /app
 if [ -d "/app/wwwroot/" ];then
   rm ./wwwroot/* -rf
  else
   mkdir /app/wwwroot
fi

 tftp -gr wwwroot.tar.gz 192.168.137.1

 tar xzvf ./wwwroot.tar.gz -C ./wwwroot/
 rm ./wwwroot.tar.gz
 tftp -gr pixiu 192.168.137.1
 chmod 777 ./pixiu
 ./pixiu &
 
sleep 5
 
wget --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer kissme' \
  --post-data '{
 "server":"host.iotsharp.net",
 "accessToken":"c1de9594cf294cd1b41f2c87a5bfa417",
 "entranceOrExit":1,
 "tollbooth":6501234,
  "gpio_door_id":1
}
' \
   '127.0.0.1:10000/api/iot_config'
rm ./iot_config   
 
wget --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer kissme' \
  --post-data '{
 "ipaddress":"192.168.137.10",
 "gateway":"192.168.137.1",
 "netmask":"255.255.255.0",
 "broadcast":"192.168.137.0"
}
' \
   '127.0.0.1:10000/api/ip_config'
rm ./ip_config   
wget --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer kissme' \
  --post-data '{
    "in1":1,
    "in2":37
}'\
   '127.0.0.1:10000/api/env_config'
rm ./env_config   
wget --header 'Authorization: Bearer kissme' \
   '127.0.0.1:10000/api/config'
rm ./config
rm setup.sh 

   