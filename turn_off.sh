PID=$(ps -a | grep "fire" | awk '{print $1}')
echo $PID
sudo kill -10 $PID

