#!/bin/bash

pre_greeter_upgrade_option_temp_path="/tmp/deepin_update_option.json"
if [ ! -f $pre_greeter_upgrade_option_temp_path ]; then
  echo "$pre_greeter_upgrade_option_temp_path does not exist, don't need process system upgrade"
  exit 1
fi

needPreGreeterUpgrade=$(jq -r .DoUpgrade /tmp/deepin_update_option.json)
if test $needPreGreeterUpgrade == true;then
  echo "DoUpgrade is true, system update need proceed"
  exit 0
fi

needPreGreeterCheck=$(jq -r .PreGreeterCheck /tmp/deepin_update_option.json)
if test $needPreGreeterCheck != true;then
  echo "PreGreeterCheck is false ,don't need preGreeter check"
  exit 1
fi

echo "$pre_greeter_upgrade_option_temp_path exist, system update can proceed"
exit 0