get_site() {
    site=$1;
    curl --proxy localhost:${PROXY_PORT} --silent --output downloads/${site//\//_} http://${site};
    if ! diff <(cat downloads/${site//\//_}) <(curl --silent http://${site}); then
        echo -e "\u001b[31mFailed $site.\u001b[0m";
        exit 1
    fi
}

# Start new proxy
make run &

PROXY_PORT=$(cs24-port)

sleep 1

pids=""

for site in $(cat $1); do
    get_site $site &
    pids="$pids $!"
done

ERROR=false

for pid in $pids; do
    wait $pid || ERROR=true;
done

if ! $ERROR; then
    echo -e "\u001b[32;1mSuccess.\u001b[0m"
fi
