nginx -p nginx/ -c nginx.conf -s stop

while read pid; do
  kill $pid
done <backend_pids
rm backend_pids
