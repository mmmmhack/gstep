rm log/*.log
./gserver
server_log=`grep -l gserver log/*.log`
echo mv .$server_log. log/gserver.log
mv $server_log log/gserver.log
