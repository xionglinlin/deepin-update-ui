#!/bin/bash

ab_recovery_mark_file="/usr/share/deepin-ab-recovery/.deepin-ab-recovery-backup"
ab_recovery_mark_file_old="/.deepin-ab-recovery-backup"
if [[ ! -e $ab_recovery_mark_file && ! -e $ab_recovery_mark_file_old ]]; then
  echo "$ab_recovery_mark_file and $ab_recovery_mark_file_old not exist, don't need process rollback"
  exit 1
fi


can_restore=$(qdbus --literal --system com.deepin.ABRecovery /com/deepin/ABRecovery com.deepin.ABRecovery.CanRestore)
if [ "$can_restore" = "false" ];then
  echo "can not restore"
  exit 1
fi
echo "ab_recovery_mark_file exist and can restore, rollback can process"
exit 0