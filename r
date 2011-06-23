old_server_logs=`grep -l gserver.c log/*.log`
echo removing old server logs: [$old_server_logs]
for f in $old_server_logs; do rm $f; done

echo finding gdb logs
old_gdb_logs=`grep -l gdb.c log/*.log`
echo removing gdb logs: [$old_gdb_logs]
for f in $old_gdb_logs; do rm $f; done

./gserver

server_log=`grep -l gserver.c log/*.log`
echo renaming server_log: [$server_log]
mv $server_log log/gserver.log

gdb_log=`grep -l "bef stdio pipe switch" log/*.log`
echo renaming gdb_log: [$gdb_log]
mv $gdb_log log/gdb.log
