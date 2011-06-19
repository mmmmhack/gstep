old_server_logs=`grep -l gserver.c log/*.log`
for f in $old_server_logs; do rm $f; done
./gserver
server_log=`grep -l gserver log/*.log`
#echo mv .$server_log. log/gserver.log
mv $server_log log/gserver.log
