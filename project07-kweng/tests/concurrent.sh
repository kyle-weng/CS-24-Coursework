# Start new proxy
make run &

sleep 1

# Start HTTP servers
#kill -9 $(ps ax | grep simple-http | grep python | sed -e "s/\([0-9]\+\) .*/\1/") > /dev/null
./tests/simple-http.py &

sleep 1

PROXY_PORT=$(cs24-port)

slow=compute-cpu2.cms.caltech.edu:8080
curl --proxy localhost:$PROXY_PORT  --silent --output downloads/${slow//\//_} http://${slow} &

sleep 1

fast=compute-cpu2.cms.caltech.edu:8081
curl --proxy localhost:$PROXY_PORT --silent --output downloads/${fast//\//_} http://${fast}

kill -9 $(ps ax | grep simple-http | grep python | sed -e "s/\([0-9]\+\) .*/\1/") > /dev/null
killall -9 proxy > /dev/null

echo -e "\u001b[32;1mSuccess.\u001b[0m"
