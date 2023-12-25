#!/bin/bash

# dcc-update-plugin.ts
lupdate ./src/dcc-update-plugin -ts -no-obsolete translations/dcc-update-plugin.ts

# dde-abrecovery.ts
lupdate ./src/dde-abrecovery -ts -no-obsolete translations/dde-abrecovery.ts

# dde-update
lupdate ./src/dde-update -ts -no-obsolete translations/dde-update.ts

tx push -s -f --branch m20
