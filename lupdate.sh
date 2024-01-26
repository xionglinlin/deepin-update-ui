#!/bin/bash
export QT_SELECT=5
for dir in src/* ;do
    if [ "$dir" != "src/common" ]; then
        tag=${dir:4}
        TS_FILE=./translations/${tag}.ts
        lupdate $dir -ts -no-ui-lines -locations absolute -no-obsolete $TS_FILE
        sed -e 's/DCC_NAMESPACE/dccV20/g' $TS_FILE > tmp.ts
        mv tmp.ts $TS_FILE
    fi
done

tx push -s -f --branch m20
