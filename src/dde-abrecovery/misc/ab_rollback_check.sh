#!/bin/bash

ab_recovery_mark_file="/.deepin-ab-recovery-backup"
if [ ! -f $ab_recovery_mark_file ]; then
  echo "$ab_recovery_mark_file does not exist, don't need process rollback"
  exit 1
fi

can_restore=$(qdbus --literal --system com.deepin.ABRecovery /com/deepin/ABRecovery com.deepin.ABRecovery.CanRestore)
if [ "$can_restore" = "false" ];then
  echo "can not restore"
  exit 1
fi
echo "ab_recovery_mark_file exist and can restore, rollback can process"
exit 0